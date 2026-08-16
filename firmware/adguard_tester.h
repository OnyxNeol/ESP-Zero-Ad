#ifndef ADGUARD_TESTER_H
#define ADGUARD_TESTER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>
#include "test_domains.h"

// ============================================================================
// AdGuard Tester
// ============================================================================
// Sends DNS A queries to the router's IP (e.g., 192.168.1.1) to determine
// whether the router ad blocker service (or built-in DNS filtering) can block each
// domain. Returns results indicating routerBlocks (bool) and resolvedIP.
//
// The built-in test uses 129 domains from adblock.turtlecute.org (d3host.txt).
// This test runs BEFORE the block list is created — only domains the router
// CANNOT block get added to the ESP32-S3's block list.
// ============================================================================

struct AdGuardTestResult {
    String domain;
    bool routerBlocks;   // true if router returns sinkhole/NXDOMAIN
    String resolvedIP;   // the IP returned by the router (or "NXDOMAIN")
    int category;        // TestCategory enum value
};

class AdGuardTester {
public:
    AdGuardTester();
    ~AdGuardTester();

    // Set the router IP to test against.
    void setRouterIP(const String& ip);

    // Get current router IP.
    String getRouterIP();

    // Test a single domain against the router.
    AdGuardTestResult testDomain(const String& domain);

    // Test a list of domains. Returns JSON array of results.
    // If autoAdd is true, domains the router CANNOT block are added to the block list.
    String testDomains(const String& domainsJsonArray, bool autoAdd = false,
                       std::function<void(int current, int total)> progressCallback = nullptr);

    // Run the built-in ad-blocker test (129 domains from d3host.txt).
    // This is the test that runs BEFORE the block list is created.
    // Tests all domains against the router, adds unblockable ones to the block list.
    // progressCallback is called after each domain (current, total, domain, routerBlocks).
    // Returns the number of domains added to the block list.
    int runBuiltinTest(bool autoAdd = true,
                       std::function<void(int current, int total, const String& domain, bool routerBlocks)> progressCallback = nullptr);

    // Get last test results as JSON.
    String getTestResults();

    // Get last test results as a vector (for firmware use).
    const std::vector<AdGuardTestResult>& getLastResults() const;

    // Get last test progress info.
    int getLastTestedCount();
    int getLastTotalCount();

    // Check if the built-in test has been run.
    bool hasTestRun();
    void markTestRun();

private:
    size_t buildDNSQuery(const String& domain, uint8_t* buf, size_t bufLen);
    bool parseDNSResponse(const uint8_t* buf, size_t len, String& outIP, bool& isNXDOMAIN, bool& isSinkhole);
    bool isKnownSinkholeIP(const String& ip);

    WiFiUDP _udp;
    String _routerIP;
    SemaphoreHandle_t _mutex;

    std::vector<AdGuardTestResult> _lastResults;
    int _lastTestedCount;
    int _lastTotalCount;
    bool _testRun;
};

// Global instance
extern AdGuardTester AdGuardTest;

#endif // ADGUARD_TESTER_H
