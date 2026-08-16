#include "adguard_tester.h"
#include "config.h"
#include "block_list.h"
#include "storage.h"

AdGuardTester AdGuardTest;

// Known sinkhole IPs that indicate a domain is being blocked
static const char* SINKHOLE_IPS[] = {
    "0.0.0.0",
    "0.0.0.1",
    "127.0.0.1",
    "0.0.0.255",
    "255.255.255.255"
};
static const int SINKHOLE_IP_COUNT = 5;

AdGuardTester::AdGuardTester() {
    _mutex = xSemaphoreCreateMutex();
    _routerIP = ROUTER_DNS_IP;
    _lastTestedCount = 0;
    _lastTotalCount = 0;
    _testRun = false;
}

AdGuardTester::~AdGuardTester() {
    if (_mutex) vSemaphoreDelete(_mutex);
}

void AdGuardTester::setRouterIP(const String& ip) {
    _routerIP = ip;
}

String AdGuardTester::getRouterIP() {
    return _routerIP;
}

size_t AdGuardTester::buildDNSQuery(const String& domain, uint8_t* buf, size_t bufLen) {
    if (bufLen < 512) return 0;

    size_t pos = 0;

    // DNS header (12 bytes)
    // Transaction ID
    buf[pos++] = 0xAB; buf[pos++] = 0xCD;
    // Flags: standard query, recursion desired
    buf[pos++] = 0x01; buf[pos++] = 0x00;
    // QDCOUNT = 1
    buf[pos++] = 0x00; buf[pos++] = 0x01;
    // ANCOUNT = 0
    buf[pos++] = 0x00; buf[pos++] = 0x00;
    // NSCOUNT = 0
    buf[pos++] = 0x00; buf[pos++] = 0x00;
    // ARCOUNT = 0
    buf[pos++] = 0x00; buf[pos++] = 0x00;

    // Encode domain name as labels
    String d = domain;
    d.toLowerCase();
    int start = 0;
    while (start < d.length()) {
        int dotPos = d.indexOf('.', start);
        String label;
        if (dotPos < 0) {
            label = d.substring(start);
            start = d.length();
        } else {
            label = d.substring(start, dotPos);
            start = dotPos + 1;
        }
        uint8_t labelLen = label.length();
        buf[pos++] = labelLen;
        for (int i = 0; i < labelLen; i++) {
            buf[pos++] = (uint8_t)label.charAt(i);
        }
    }
    buf[pos++] = 0x00; // null terminator

    // QTYPE = A (1)
    buf[pos++] = 0x00; buf[pos++] = 0x01;
    // QCLASS = IN (1)
    buf[pos++] = 0x00; buf[pos++] = 0x01;

    return pos;
}

bool AdGuardTester::isKnownSinkholeIP(const String& ip) {
    for (int i = 0; i < SINKHOLE_IP_COUNT; i++) {
        if (ip == SINKHOLE_IPS[i]) return true;
    }
    return false;
}

bool AdGuardTester::parseDNSResponse(const uint8_t* buf, size_t len,
                                      String& outIP, bool& isNXDOMAIN, bool& isSinkhole) {
    outIP = "";
    isNXDOMAIN = false;
    isSinkhole = false;

    if (len < 12) return false;

    // Check RCODE (bits 0-3 of byte 3)
    uint8_t rcode = buf[3] & 0x0F;
    if (rcode == 3) { // NXDOMAIN
        isNXDOMAIN = true;
        outIP = "NXDOMAIN";
        return true; // valid response, domain doesn't exist -> blocked
    }
    if (rcode != 0) {
        // Other error code
        outIP = "ERROR_" + String(rcode);
        isNXDOMAIN = true; // treat as blocked
        return true;
    }

    uint16_t anCount = (buf[6] << 8) | buf[7];
    if (anCount == 0) {
        // No answer records — treat as blocked (no resolution)
        outIP = "0.0.0.0";
        isSinkhole = true;
        return true;
    }

    // Skip the question section
    size_t pos = 12;
    // Skip QNAME
    while (pos < len) {
        uint8_t labelLen = buf[pos];
        if (labelLen == 0) {
            pos++;
            break;
        }
        if ((labelLen & 0xC0) == 0xC0) {
            pos += 2;
            break;
        }
        pos += 1 + labelLen;
    }
    pos += 4; // QTYPE + QCLASS

    // Parse answer records
    for (int a = 0; a < anCount && pos < len; a++) {
        // Skip name (could be compression pointer)
        uint8_t labelLen = buf[pos];
        if ((labelLen & 0xC0) == 0xC0) {
            pos += 2;
        } else {
            while (pos < len && buf[pos] != 0) {
                pos += 1 + buf[pos];
            }
            pos++; // skip null
        }

        if (pos + 10 > len) break;

        uint16_t rType = (buf[pos] << 8) | buf[pos + 1];
        uint16_t rClass = (buf[pos + 2] << 8) | buf[pos + 3];
        // uint32_t ttl = (buf[pos+4]<<24)|(buf[pos+5]<<16)|(buf[pos+6]<<8)|buf[pos+7];
        uint16_t rdLength = (buf[pos + 8] << 8) | buf[pos + 9];
        pos += 10;

        if (rType == 1 && rClass == 1 && rdLength == 4 && pos + 4 <= len) {
            // A record
            char ipStr[16];
            snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u",
                     buf[pos], buf[pos + 1], buf[pos + 2], buf[pos + 3]);
            outIP = String(ipStr);
            isSinkhole = isKnownSinkholeIP(outIP);
            return true;
        }

        pos += rdLength;
    }

    return false;
}

AdGuardTestResult AdGuardTester::testDomain(const String& domain) {
    AdGuardTestResult result;
    result.domain = domain;
    result.routerBlocks = false;
    result.resolvedIP = "";

    IPAddress routerAddr;
    if (!routerAddr.fromString(_routerIP)) {
        DEBUG_PRINTF("[AdGuard] Invalid router IP: %s\n", _routerIP.c_str());
        result.resolvedIP = "INVALID_ROUTER_IP";
        return result;
    }

    // Build DNS query
    uint8_t query[512];
    size_t queryLen = buildDNSQuery(domain, query, sizeof(query));
    if (queryLen == 0) {
        result.resolvedIP = "QUERY_BUILD_ERROR";
        return result;
    }

    // Send query to router
    _udp.begin(0); // bind to random port
    _udp.beginPacket(routerAddr, 53);
    _udp.write(query, queryLen);
    _udp.endPacket();

    // Wait for response
    unsigned long startTime = millis();
    bool gotResponse = false;
    while (millis() - startTime < 3000) {
        int respSize = _udp.parsePacket();
        if (respSize > 0) {
            uint8_t response[512];
            if (respSize > 512) respSize = 512;
            _udp.read(response, respSize);

            String outIP;
            bool isNXDOMAIN;
            bool isSinkhole;
            bool parsed = parseDNSResponse(response, respSize, outIP, isNXDOMAIN, isSinkhole);

            if (parsed) {
                result.resolvedIP = outIP;
                result.routerBlocks = (isNXDOMAIN || isSinkhole);
            } else {
                result.resolvedIP = "PARSE_ERROR";
                result.routerBlocks = false;
            }
            gotResponse = true;
            break;
        }
        delay(1);
    }

    if (!gotResponse) {
        result.resolvedIP = "TIMEOUT";
        result.routerBlocks = false; // can't determine
    }

    _udp.stop();

    DEBUG_PRINTF("[AdGuard] %s -> routerBlocks=%s resolvedIP=%s\n",
                 domain.c_str(),
                 result.routerBlocks ? "true" : "false",
                 result.resolvedIP.c_str());

    return result;
}

String AdGuardTester::testDomains(const String& domainsJsonArray, bool autoAdd,
                                    std::function<void(int, int)> progressCallback) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, domainsJsonArray);
    if (err) {
        DEBUG_PRINTLN(F("[AdGuard] Invalid domains JSON"));
        return "[]";
    }

    JsonArray arr = doc.as<JsonArray>();
    int total = arr.size();

    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _lastResults.clear();
        _lastTestedCount = 0;
        _lastTotalCount = total;
        xSemaphoreGive(_mutex);
    }

    JsonDocument resultDoc;
    JsonArray resultArr = resultDoc.to<JsonArray>();

    int current = 0;
    for (JsonVariant v : arr) {
        if (!v.is<const char*>()) continue;
        String domain = v.as<String>();

        AdGuardTestResult tr = testDomain(domain);

        JsonObject obj = resultArr.add<JsonObject>();
        obj["domain"] = tr.domain;
        obj["routerBlocks"] = tr.routerBlocks;
        obj["resolvedIP"] = tr.resolvedIP;

        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _lastResults.push_back(tr);
            _lastTestedCount = current + 1;
            xSemaphoreGive(_mutex);
        }

        // Auto-add domains the router CANNOT block to the block list
        if (autoAdd && !tr.routerBlocks &&
            !tr.resolvedIP.equals("TIMEOUT") &&
            !tr.resolvedIP.equals("INVALID_ROUTER_IP") &&
            !tr.resolvedIP.equals("QUERY_BUILD_ERROR")) {
            Blocklist.addDomain(domain);
            DEBUG_PRINTF("[AdGuard] Auto-added to blocklist: %s\n", domain.c_str());
        }

        current++;
        if (progressCallback) {
            progressCallback(current, total);
        }

        // Small delay to avoid overwhelming the router
        delay(50);

        // Feed watchdog
        yield();
    }

    String output;
    serializeJson(resultArr, output);

    // Save results to LittleFS
    Storage.writeFile(FS_TEST_RESULTS_PATH, output);

    DEBUG_PRINTF("[AdGuard] Tested %d/%d domains, autoAdd=%s\n",
                 current, total, autoAdd ? "true" : "false");
    return output;
}

String AdGuardTester::getTestResults() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return "[]";

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const AdGuardTestResult& r : _lastResults) {
        JsonObject obj = arr.add<JsonObject>();
        obj["domain"] = r.domain;
        obj["routerBlocks"] = r.routerBlocks;
        obj["resolvedIP"] = r.resolvedIP;
    }

    String output;
    serializeJson(arr, output);
    xSemaphoreGive(_mutex);
    return output;
}

int AdGuardTester::getLastTestedCount() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        int c = _lastTestedCount;
        xSemaphoreGive(_mutex);
        return c;
    }
    return 0;
}

int AdGuardTester::getLastTotalCount() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        int c = _lastTotalCount;
        xSemaphoreGive(_mutex);
        return c;
    }
    return 0;
}

// ============================================================================
// Built-in AdBlock Test (from adblock.turtlecute.org / d3host.txt)
// ============================================================================
// Runs BEFORE the block list is created. Tests all 129 domains from
// test_domains.h against the router ad blocker service. Only domains the router
// CANNOT block get added to the block list.
// ============================================================================

int AdGuardTester::runBuiltinTest(bool autoAdd,
    std::function<void(int current, int total, const String& domain, bool routerBlocks)> progressCallback) {

    int total = TEST_DOMAINS_COUNT;

    Serial.println();
    Serial.println(F("============================================"));
    Serial.println(F("  Running AdBlock Test (adblock.turtlecute.org)"));
    Serial.printf("  Testing %d domains against router at %s\n", total, _routerIP.c_str());
    Serial.println(F("  Only domains the router CANNOT block will"));
    Serial.println(F("  be added to the ESP32-S3 block list."));
    Serial.println(F("============================================"));
    Serial.println();

    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _lastResults.clear();
        _lastTestedCount = 0;
        _lastTotalCount = total;
        xSemaphoreGive(_mutex);
    }

    int addedCount = 0;
    int routerBlocksCount = 0;
    int routerDoesNotBlockCount = 0;

    for (int i = 0; i < total; i++) {
        const char* domain = TEST_DOMAINS[i].domain;
        TestCategory category = TEST_DOMAINS[i].category;

        // Test domain against router
        AdGuardTestResult result = testDomain(String(domain));
        result.category = (int)category;

        // Store result
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _lastResults.push_back(result);
            _lastTestedCount = i + 1;
            xSemaphoreGive(_mutex);
        }

        if (result.routerBlocks) {
            routerBlocksCount++;
            Serial.printf("  [%3d/%3d] ✓ router blocks: %s (%s)\n",
                          i + 1, total, domain, getCategoryName(category));
        } else {
            routerDoesNotBlockCount++;

            // Domain resolves (router can't block it) → add to block list
            if (autoAdd &&
                !result.resolvedIP.equals("TIMEOUT") &&
                !result.resolvedIP.equals("INVALID_ROUTER_IP") &&
                !result.resolvedIP.equals("QUERY_BUILD_ERROR") &&
                !result.resolvedIP.equals("PARSE_ERROR")) {
                Blocklist.addDomain(String(domain));
                addedCount++;
                Serial.printf("  [%3d/%3d] ✗ router can't block → ADDED: %s (%s) [%s]\n",
                              i + 1, total, domain, getCategoryName(category),
                              result.resolvedIP.c_str());
            } else if (result.resolvedIP.equals("TIMEOUT")) {
                Serial.printf("  [%3d/%3d] ? timeout: %s (%s)\n",
                              i + 1, total, domain, getCategoryName(category));
            } else {
                Serial.printf("  [%3d/%3d] ✗ router can't block → ADDED: %s (%s)\n",
                              i + 1, total, domain, getCategoryName(category));
                if (autoAdd) {
                    Blocklist.addDomain(String(domain));
                    addedCount++;
                }
            }
        }

        // Progress callback
        if (progressCallback) {
            progressCallback(i + 1, total, String(domain), result.routerBlocks);
        }

        // Small delay between queries
        delay(50);
        yield();
    }

    // Mark test as run
    _testRun = true;

    // Save results to LittleFS
    String resultsJson = getTestResults();
    Storage.writeFile(FS_TEST_RESULTS_PATH, resultsJson);

    // Save block list
    Blocklist.save();

    Serial.println();
    Serial.println(F("============================================"));
    Serial.println(F("  AdBlock Test Complete!"));
    Serial.println(F("============================================"));
    Serial.printf("  Total tested:        %d\n", total);
    Serial.printf("  Router blocks:       %d (skipped)\n", routerBlocksCount);
    Serial.printf("  Router can't block:  %d (added to ESP32-S3 list)\n", routerDoesNotBlockCount);
    Serial.printf("  Added to block list: %d\n", addedCount);
    Serial.printf("  Block list size:     %d domains\n", Blocklist.getBlockedCount());
    Serial.println(F("============================================"));
    Serial.println();

    return addedCount;
}

bool AdGuardTester::hasTestRun() {
    return _testRun;
}

void AdGuardTester::markTestRun() {
    _testRun = true;
}

const std::vector<AdGuardTestResult>& AdGuardTester::getLastResults() const {
    return _lastResults;
}
