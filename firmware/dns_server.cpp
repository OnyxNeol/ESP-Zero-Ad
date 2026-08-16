#include "dns_server.h"
#include "config.h"
#include "block_list.h"
#include "storage.h"

DNSSinkholeServer DNSServer;

DNSSinkholeServer::DNSSinkholeServer() {
    _mutex = xSemaphoreCreateMutex();
    _port = DNS_PORT;
    _running = false;
    _stats.totalQueries = 0;
    _stats.blockedQueries = 0;
    _stats.forwardedQueries = 0;
    _stats.uniqueDomains = 0;
}

DNSSinkholeServer::~DNSSinkholeServer() {
    if (_mutex) vSemaphoreDelete(_mutex);
}

bool DNSSinkholeServer::begin(uint16_t port) {
    _port = port;

    _upstream1.fromString(DNS_UPSTREAM_1);
    _upstream2.fromString(DNS_UPSTREAM_2);

    if (!_udp.begin(_port)) {
        DEBUG_PRINTLN(F("[DNS] Failed to start UDP server on port 53!"));
        return false;
    }
    if (!_udpForward.begin(0)) {
        DEBUG_PRINTLN(F("[DNS] Failed to start forwarding UDP socket!"));
        return false;
    }

    _running = true;
    DEBUG_PRINTF("[DNS] Sinkhole server listening on port %d\n", _port);
    DEBUG_PRINTF("[DNS] Upstream: %s, %s\n", DNS_UPSTREAM_1, DNS_UPSTREAM_2);
    return true;
}

String DNSSinkholeServer::parseDomainName(const uint8_t* buf, size_t len, size_t& qOffset) {
    String domain = "";
    size_t pos = 12; // skip 12-byte DNS header
    while (pos < len) {
        uint8_t labelLen = buf[pos];
        if (labelLen == 0) {
            pos++; // skip null terminator
            break;
        }
        // Check for compression pointer (not expected in queries, but handle)
        if ((labelLen & 0xC0) == 0xC0) {
            pos += 2;
            break;
        }
        pos++; // skip length byte
        if (pos + labelLen > len) break;
        for (size_t i = 0; i < labelLen; i++) {
            domain += (char)buf[pos + i];
        }
        pos += labelLen;
        domain += ".";
    }
    qOffset = pos;
    // Remove trailing dot
    if (domain.length() > 0 && domain.charAt(domain.length() - 1) == '.') {
        domain.remove(domain.length() - 1);
    }
    return domain;
}

void DNSSinkholeServer::buildSinkholeResponse(const uint8_t* query, size_t queryLen,
                                                uint8_t* response, size_t& respLen) {
    // Copy the header (12 bytes)
    memcpy(response, query, 12);
    // Set QR = 1 (response), OPCODE = 0 (standard query), AA = 0, TC = 0, RD = 1
    response[2] = 0x81; // QR=1, RD=1
    // Set RA = 1, Z = 0, RCODE = 0 (NOERROR)
    response[3] = 0x80; // RA=1, RCODE=0
    // QDCOUNT = 1 (same as query)
    // ANCOUNT = 1
    response[6] = 0x00; response[7] = 0x01;
    // NSCOUNT = 0, ARCOUNT = 0
    response[8] = 0x00; response[9] = 0x00;
    response[10] = 0x00; response[11] = 0x00;

    // Copy the question section from the query
    // Find the end of the question section
    size_t qEnd = 12;
    // Skip through the QNAME
    while (qEnd < queryLen) {
        uint8_t labelLen = query[qEnd];
        if (labelLen == 0) {
            qEnd++;
            break;
        }
        if ((labelLen & 0xC0) == 0xC0) {
            qEnd += 2;
            break;
        }
        qEnd += 1 + labelLen;
    }
    // Skip QTYPE (2) and QCLASS (2)
    qEnd += 4;

    size_t qSectionLen = qEnd - 12;
    memcpy(response + 12, query + 12, qSectionLen);

    // Build the answer section
    // Use compression pointer to the domain name in the question section (offset 12)
    size_t answerPos = 12 + qSectionLen;
    // Name pointer to offset 12 (0xC00C)
    response[answerPos++] = 0xC0;
    response[answerPos++] = 0x0C;
    // TYPE A = 1
    response[answerPos++] = 0x00;
    response[answerPos++] = 0x01;
    // CLASS IN = 1
    response[answerPos++] = 0x00;
    response[answerPos++] = 0x01;
    // TTL = 60 seconds
    response[answerPos++] = 0x00;
    response[answerPos++] = 0x00;
    response[answerPos++] = 0x00;
    response[answerPos++] = 0x3C;
    // RDLENGTH = 4
    response[answerPos++] = 0x00;
    response[answerPos++] = 0x04;
    // RDATA = 0.0.0.0
    response[answerPos++] = SINKHOLE_IP_0;
    response[answerPos++] = SINKHOLE_IP_1;
    response[answerPos++] = SINKHOLE_IP_2;
    response[answerPos++] = SINKHOLE_IP_3;

    respLen = answerPos;
}

bool DNSSinkholeServer::isSinkholeIP(uint32_t ip) {
    // 0.0.0.0
    if (ip == 0x00000000) return true;
    // 0.0.0.1
    if (ip == 0x00000001) return true;
    // Common AdGuard sinkhole: 0.0.0.0 variants already handled
    return false;
}

void DNSSinkholeServer::forwardToUpstream(const uint8_t* query, size_t queryLen,
                                            const IPAddress& clientIP, uint16_t clientPort) {
    // Try upstream 1 first, then upstream 2
    IPAddress upstreams[2] = {_upstream1, _upstream2};

    for (int i = 0; i < 2; i++) {
        _udpForward.beginPacket(upstreams[i], 53);
        _udpForward.write(query, queryLen);
        _udpForward.endPacket();

        // Wait for response (non-blocking with timeout)
        unsigned long startTime = millis();
        bool gotResponse = false;
        while (millis() - startTime < 2000) {
            int respSize = _udpForward.parsePacket();
            if (respSize > 0) {
                uint8_t response[512];
                if (respSize > 512) respSize = 512;
                _udpForward.read(response, respSize);

                // Send response back to original client
                _udp.beginPacket(clientIP, clientPort);
                _udp.write(response, respSize);
                _udp.endPacket();
                gotResponse = true;
                break;
            }
            delay(1);
        }
        if (gotResponse) return;
        // If no response from upstream 1, try upstream 2
    }

    // If both upstreams failed, send NXDOMAIN
    uint8_t response[512];
    memcpy(response, query, min((size_t)queryLen, (size_t)512));
    response[2] = 0x81; // QR=1, RD=1
    response[3] = 0x83; // RA=1, RCODE=3 (NXDOMAIN)
    // ANCOUNT = 0, NSCOUNT = 0, ARCOUNT = 0
    response[6] = 0x00; response[7] = 0x00;
    response[8] = 0x00; response[9] = 0x00;
    response[10] = 0x00; response[11] = 0x00;

    _udp.beginPacket(clientIP, clientPort);
    _udp.write(response, min((size_t)queryLen, (size_t)512));
    _udp.endPacket();

    DEBUG_PRINTLN(F("[DNS] Both upstreams failed, sent NXDOMAIN"));
}

void DNSSinkholeServer::trackDomain(const String& domain) {
    // Track unique domains
    for (const String& d : _uniqueDomains) {
        if (d == domain) return; // already tracked
    }
    _uniqueDomains.push_back(domain);
    _stats.uniqueDomains = (uint32_t)_uniqueDomains.size();

    // Cap unique domains list to prevent memory issues
    if (_uniqueDomains.size() > 5000) {
        _uniqueDomains.erase(_uniqueDomains.begin());
    }
}

void DNSSinkholeServer::trackBlockedDomain(const String& domain) {
    for (size_t i = 0; i < _blockedDomainCounts.size(); i++) {
        if (_blockedDomainCounts[i].domain == domain) {
            _blockedDomainCounts[i].count++;
            return;
        }
    }
    if ((int)_blockedDomainCounts.size() < MAX_TOP_DOMAINS * 2) {
        DomainCount dc;
        dc.domain = domain;
        dc.count = 1;
        _blockedDomainCounts.push_back(dc);
    }
}

bool DNSSinkholeServer::processNextRequest() {
    if (!_running) return false;

    int packetSize = _udp.parsePacket();
    if (packetSize <= 0) return false;

    uint8_t buffer[512];
    if (packetSize > 512) packetSize = 512;
    _udp.read(buffer, packetSize);

    IPAddress clientIP = _udp.remoteIP();
    uint16_t clientPort = _udp.remotePort();

    // Parse DNS header
    if (packetSize < 12) return false;

    uint16_t flags = (buffer[2] << 8) | buffer[3];
    bool isQuery = ((flags & 0x8000) == 0); // QR=0 means query
    uint16_t qdCount = (buffer[4] << 8) | buffer[5];

    if (!isQuery || qdCount == 0) return false; // ignore responses or empty queries

    // Parse the queried domain
    size_t qOffset = 0;
    String domain = parseDomainName(buffer, packetSize, qOffset);

    if (domain.length() == 0) return false;

    // Parse QTYPE
    uint16_t qType = 0;
    if (qOffset + 2 <= packetSize) {
        qType = (buffer[qOffset] << 8) | buffer[qOffset + 1];
    }

    DEBUG_PRINTF("[DNS] Query: %s (type %d) from %s\n",
                 domain.c_str(), qType, clientIP.toString().c_str());

    // Update stats
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _stats.totalQueries++;
        trackDomain(domain);
        xSemaphoreGive(_mutex);
    }

    // Only handle A record queries (type 1) for blocking
    // For other types, just forward
    if (qType == 1 && Blocklist.isBlocked(domain)) {
        // Blocked: return sinkhole response
        uint8_t response[512];
        size_t respLen = 0;
        buildSinkholeResponse(buffer, packetSize, response, respLen);

        _udp.beginPacket(clientIP, clientPort);
        _udp.write(response, respLen);
        _udp.endPacket();

        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _stats.blockedQueries++;
            trackBlockedDomain(domain);
            xSemaphoreGive(_mutex);
        }

        DEBUG_PRINTF("[DNS] BLOCKED: %s -> 0.0.0.0\n", domain.c_str());
    } else {
        // Not blocked or non-A query: forward to upstream
        forwardToUpstream(buffer, packetSize, clientIP, clientPort);

        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _stats.forwardedQueries++;
            xSemaphoreGive(_mutex);
        }

        DEBUG_PRINTF("[DNS] FORWARDED: %s\n", domain.c_str());
    }

    return true;
}

String DNSSinkholeServer::getStats() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return "{}";

    DNSServerStats s = _stats;
    xSemaphoreGive(_mutex);

    float blockedPercent = 0;
    if (s.totalQueries > 0) {
        blockedPercent = ((float)s.blockedQueries / (float)s.totalQueries) * 100.0;
    }

    JsonDocument doc;
    doc["totalQueries"] = s.totalQueries;
    doc["blockedQueries"] = s.blockedQueries;
    doc["forwardedQueries"] = s.forwardedQueries;
    doc["blockedPercent"] = (float)(round(blockedPercent * 100) / 100.0);
    doc["uniqueDomains"] = s.uniqueDomains;

    String output;
    serializeJson(doc, output);
    return output;
}

DNSServerStats DNSSinkholeServer::getRawStats() {
    DNSServerStats s;
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        s = _stats;
        xSemaphoreGive(_mutex);
    }
    return s;
}

String DNSSinkholeServer::getTopBlockedDomains(int limit) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return "[]";

    // Sort by count descending
    std::vector<DomainCount> sorted = _blockedDomainCounts;
    std::sort(sorted.begin(), sorted.end(), [](const DomainCount& a, const DomainCount& b) {
        return a.count > b.count;
    });

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    int count = min(limit, (int)sorted.size());
    for (int i = 0; i < count; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["domain"] = sorted[i].domain;
        obj["count"] = sorted[i].count;
    }

    String output;
    serializeJson(arr, output);
    xSemaphoreGive(_mutex);
    return output;
}

void DNSSinkholeServer::resetStats() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _stats.totalQueries = 0;
        _stats.blockedQueries = 0;
        _stats.forwardedQueries = 0;
        _stats.uniqueDomains = 0;
        _uniqueDomains.clear();
        _blockedDomainCounts.clear();
        xSemaphoreGive(_mutex);
    }
    DEBUG_PRINTLN(F("[DNS] Stats reset"));
}

void DNSSinkholeServer::saveStats() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return;

    JsonDocument doc;
    doc["totalQueries"] = _stats.totalQueries;
    doc["blockedQueries"] = _stats.blockedQueries;
    doc["forwardedQueries"] = _stats.forwardedQueries;
    doc["uniqueDomains"] = _stats.uniqueDomains;

    xSemaphoreGive(_mutex);

    String output;
    serializeJson(doc, output);
    Storage.writeFile(FS_STATS_PATH, output);
    DEBUG_PRINTLN(F("[DNS] Stats saved"));
}

void DNSSinkholeServer::loadStats() {
    String content = Storage.readFile(FS_STATS_PATH);
    if (content.length() == 0) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) return;

    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _stats.totalQueries = doc["totalQueries"] | 0;
        _stats.blockedQueries = doc["blockedQueries"] | 0;
        _stats.forwardedQueries = doc["forwardedQueries"] | 0;
        _stats.uniqueDomains = doc["uniqueDomains"] | 0;
        xSemaphoreGive(_mutex);
    }
    DEBUG_PRINTF("[DNS] Loaded stats: total=%u blocked=%u forwarded=%u unique=%u\n",
                 _stats.totalQueries, _stats.blockedQueries, _stats.forwardedQueries,
                 _stats.uniqueDomains);
}
