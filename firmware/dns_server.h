#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>

// ============================================================================
// DNS Sinkhole Server
// ============================================================================
// Listens on UDP port 53. For each query:
//   - If domain is in the block list -> returns 0.0.0.0 A record
//   - Otherwise -> forwards to upstream DNS and returns the real response
// Tracks query statistics with thread-safe mutex.
// ============================================================================

struct DNSServerStats {
    uint32_t totalQueries;
    uint32_t blockedQueries;
    uint32_t forwardedQueries;
    uint32_t uniqueDomains;   // number of unique queried domains
};

struct DomainCount {
    String domain;
    uint32_t count;
};

class DNSSinkholeServer {
public:
    DNSSinkholeServer();
    ~DNSSinkholeServer();

    // Start listening on port 53. Returns true on success.
    bool begin(uint16_t port = DNS_PORT);

    // Process the next incoming DNS request (non-blocking). Returns true if processed one.
    bool processNextRequest();

    // Get current stats as JSON string.
    String getStats();

    // Reset all stats to zero.
    void resetStats();

    // Get top blocked domains (by query count).
    String getTopBlockedDomains(int limit = MAX_TOP_DOMAINS);

    // Save stats to LittleFS.
    void saveStats();

    // Load stats from LittleFS.
    void loadStats();

    // Get raw stats struct.
    DNSServerStats getRawStats();

private:
    // Parse a DNS query packet and extract the queried domain name.
    String parseDomainName(const uint8_t* buf, size_t len, size_t& qOffset);

    // Build a sinkhole DNS response (0.0.0.0) for the given query packet.
    void buildSinkholeResponse(const uint8_t* query, size_t queryLen, uint8_t* response, size_t& respLen);

    // Forward a DNS query to upstream and relay the response back to client.
    void forwardToUpstream(const uint8_t* query, size_t queryLen,
                           const IPAddress& clientIP, uint16_t clientPort);

    // Track a queried domain for stats (unique domains + top domains).
    void trackDomain(const String& domain);

    // Track a blocked domain specifically.
    void trackBlockedDomain(const String& domain);

    WiFiUDP _udp;
    WiFiUDP _udpForward;  // separate socket for forwarding
    uint16_t _port;
    bool _running;
    SemaphoreHandle_t _mutex;

    DNSServerStats _stats;

    // Track unique queried domains (simple list, capped)
    std::vector<String> _uniqueDomains;
    // Track top blocked domains with counts
    std::vector<DomainCount> _blockedDomainCounts;

    // Known sinkhole IPs for forwarding check
    bool isSinkholeIP(uint32_t ip);

    // Upstream DNS IPs
    IPAddress _upstream1;
    IPAddress _upstream2;
};

// Global instance
extern DNSSinkholeServer DNSServer;

#endif // DNS_SERVER_H
