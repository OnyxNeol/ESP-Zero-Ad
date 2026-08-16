// ============================================================================
// ESP32-S3 DNS Sinkhole Ad Blocker — Main Sketch
// ============================================================================
// The ESP32-S3 acts as a DNS server in the middle of your network.
// It blocks ad domains by returning 0.0.0.0 and forwards everything
// else to the upstream DNS.
//
// BOOT FLOW (ZERO terminal, ZERO hotspots, ALL real):
//   1. User fills in firmware/data/wifi_config.json BEFORE flashing
//   2. Flash firmware + filesystem to ESP32-S3
//   3. On boot: reads wifi_config.json from LittleFS
//   4. Connects to user's existing WiFi network
//   5. Runs the AdBlock test (129 domains from adblock.turtlecute.org)
//      against the router ad blocker service
//   6. Only domains the router CANNOT block get added to the block list
//   7. Starts DNS sinkhole (port 53) + web dashboard (port 80)
//   8. mDNS: accessible at http://esp32-pihole.local
//   9. User opens that URL → first-run wizard shows IP, test results,
//      and instructions to set DNS on their devices
//
// NO SERIAL INPUT REQUIRED. NO HOTSPOT CREATED. NO TERMINAL NEEDED.
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "config.h"
#include "storage.h"
#include "block_list.h"
#include "dns_server.h"
#include "adguard_tester.h"
#include "web_server.h"
#include "setup_wizard.h"
#include "test_domains.h"

// ============================================================================
// Global State
// ============================================================================
bool wifiConnected = false;
unsigned long lastStatsSave = 0;
unsigned long lastWiFiCheck = 0;

// Loaded configuration
String cfg_ssid = "";
String cfg_password = "";
String cfg_routerIP = "";
String cfg_apiKey = "";

// Flag file for whether the adblock test has run
#define TEST_RUN_FLAG_PATH "/adblock_test_done.flag"

// ============================================================================
// Forward declarations
// ============================================================================
void connectWiFi();
void checkWiFiReconnect();
void setLED(bool on);
void blinkLED(int count, int delayMs);
void runAdBlockTest();
bool hasTestRun();
void markTestRun();

// ============================================================================
// Setup
// ============================================================================
void setup() {
    // Serial is for debug logging ONLY — no user input is ever needed
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    DEBUG_PRINTLN();
    DEBUG_PRINTLN(F("============================================"));
    DEBUG_PRINTLN(F("  ESP32-S3 DNS Sinkhole Ad Blocker"));
    DEBUG_PRINTLN(F("============================================"));

    // --- Status LED ---
    pinMode(STATUS_LED_PIN, OUTPUT);
    setLED(LED_ON);

    // --- LittleFS / Storage ---
    if (!Storage.begin()) {
        DEBUG_PRINTLN(F("[BOOT] FATAL: LittleFS mount failed! Rebooting..."));
        blinkLED(5, 200);
        delay(2000);
        ESP.restart();
    }

    // --- Load WiFi config from wifi_config.json (file-based, no terminal) ---
    if (!SetupWizard::isConfigured()) {
        DEBUG_PRINTLN(F("[BOOT] ERROR: wifi_config.json not configured!"));
        DEBUG_PRINTLN(F("[BOOT] Edit firmware/data/wifi_config.json with your"));
        DEBUG_PRINTLN(F("[BOOT] WiFi credentials, then reflash with:"));
        DEBUG_PRINTLN(F("[BOOT]   pio run --target uploadfs"));

        // Blink LED rapidly to indicate config error
        while (true) {
            blinkLED(2, 200);
            delay(2000);
        }
    }

    SetupWizard::loadConfig(cfg_ssid, cfg_password, cfg_routerIP, cfg_apiKey);
    DEBUG_PRINTF("[BOOT] Config: SSID=%s, Router=%s\n",
                 cfg_ssid.c_str(), cfg_routerIP.c_str());

    // Set the router IP for ad blocker testing
    AdGuardTest.setRouterIP(cfg_routerIP);

    // --- Connect WiFi (no terminal interaction) ---
    connectWiFi();

    if (wifiConnected) {
        // --- Start mDNS — user accesses dashboard at http://esp32-pihole.local ---
        if (MDNS.begin("esp32-pihole")) {
            DEBUG_PRINTLN(F("[BOOT] mDNS started: http://esp32-pihole.local"));
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        } else {
            DEBUG_PRINTLN(F("[BOOT] mDNS failed"));
        }

        // --- RUN ADBLOCK TEST BEFORE CREATING BLOCK LIST ---
        // Tests 129 domains from adblock.turtlecute.org against the router.
        // Only domains the router CANNOT block get added to the block list.
        if (!hasTestRun()) {
            runAdBlockTest();
        } else {
            int loaded = Blocklist.load();
            DEBUG_PRINTF("[BOOT] Block list loaded: %d domains (test already ran)\n", loaded);
        }

        // --- Load persisted DNS stats ---
        DNSServer.loadStats();

        // --- Start DNS sinkhole server (port 53) ---
        if (!DNSServer.begin(DNS_PORT)) {
            DEBUG_PRINTLN(F("[BOOT] FATAL: DNS server failed to start!"));
            blinkLED(3, 300);
        } else {
            DEBUG_PRINTLN(F("[BOOT] DNS sinkhole started on port 53"));
        }

        // --- Start web server (management dashboard, port 80) ---
        WebServerMgr.begin(WEB_SERVER_PORT);
        DEBUG_PRINTLN(F("[BOOT] Web dashboard started on port 80"));

        // --- Done — dashboard is accessible ---
        DEBUG_PRINTLN();
        DEBUG_PRINTLN(F("============================================"));
        DEBUG_PRINTLN(F("  ESP32-S3 Pi-Hole is RUNNING!"));
        DEBUG_PRINTLN(F("============================================"));
        DEBUG_PRINTF("  Dashboard: http://%s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("  Also at:   http://esp32-pihole.local\n");
        DEBUG_PRINTF("  DNS:       %s:53\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("  Block list: %d domains\n", Blocklist.getBlockedCount());
        DEBUG_PRINTLN(F("============================================"));
    } else {
        DEBUG_PRINTLN(F("[BOOT] WiFi connection failed after 3 attempts."));
        DEBUG_PRINTLN(F("[BOOT] Check wifi_config.json and reflash."));

        // Blink LED to indicate WiFi failure
        while (true) {
            blinkLED(3, 300);
            delay(3000);
        }
    }

    setLED(LED_ON);
    lastStatsSave = millis();
    lastWiFiCheck = millis();
}

// ============================================================================
// Main Loop
// ============================================================================
void loop() {
    // Process DNS requests (non-blocking, up to 10 per iteration)
    for (int i = 0; i < 10; i++) {
        if (!DNSServer.processNextRequest()) break;
    }

    // Handle web server clients (dashboard + API)
    WebServerMgr.handleClient();

    // Check WiFi connection periodically
    if (millis() - lastWiFiCheck > 10000) {
        lastWiFiCheck = millis();
        checkWiFiReconnect();
    }

    // Periodic stats save
    if (millis() - lastStatsSave > STATS_SAVE_INTERVAL_MS) {
        lastStatsSave = millis();
        DNSServer.saveStats();
        DEBUG_PRINTF("[LOOP] Heap: %u, Blocked: %u/%u\n",
                     ESP.getFreeHeap(),
                     DNSServer.getRawStats().blockedQueries,
                     DNSServer.getRawStats().totalQueries);
    }

    yield();
    delay(1);
}

// ============================================================================
// AdBlock Test — runs before the block list is created
// ============================================================================

void runAdBlockTest() {
    DEBUG_PRINTLN(F("[BOOT] Running AdBlock Test (129 domains from adblock.turtlecute.org)"));
    DEBUG_PRINTLN(F("[BOOT] Testing against router ad blocker service — only unblockable domains added"));

    // Clear any existing block list — building it from test results
    Blocklist.clearAll();

    // Run the built-in test with auto-add enabled
    // This tests all 129 domains against the router ad blocker service
    // Domains the router CAN'T block get added to the ESP32-S3's block list
    int added = AdGuardTest.runBuiltinTest(true);

    // Mark test as run
    markTestRun();

    DEBUG_PRINTF("[BOOT] AdBlock test complete. %d domains added to block list.\n", added);
}

bool hasTestRun() {
    return Storage.fileExists(TEST_RUN_FLAG_PATH);
}

void markTestRun() {
    Storage.writeFile(TEST_RUN_FLAG_PATH, String(millis()));
}

// ============================================================================
// WiFi Connection — no terminal interaction
// ============================================================================

void connectWiFi() {
    DEBUG_PRINTF("[WiFi] Connecting to: %s\n", cfg_ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.setHostname("ESP32-PiHole");

    // Try connecting up to 3 times
    for (int attempt = 1; attempt <= 3; attempt++) {
        WiFi.begin(cfg_ssid.c_str(), cfg_password.c_str());

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT_MS) {
            delay(500);
            setLED(LED_ON);
            delay(50);
            setLED(LED_OFF);
        }

        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            DEBUG_PRINTF("[WiFi] Connected! IP: %s, RSSI: %d dBm\n",
                         WiFi.localIP().toString().c_str(), WiFi.RSSI());
            setLED(LED_ON);
            return;
        }

        WiFi.disconnect();
        delay(1000);
    }

    wifiConnected = false;
    setLED(LED_OFF);
}

void checkWiFiReconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiConnected) {
            wifiConnected = true;
            DEBUG_PRINTLN(F("[WiFi] Reconnected!"));
            setLED(LED_ON);
        }
        return;
    }

    if (wifiConnected) {
        DEBUG_PRINTLN(F("[WiFi] Connection lost! Reconnecting..."));
        wifiConnected = false;
        setLED(LED_OFF);
    }

    WiFi.disconnect();
    delay(100);
    WiFi.begin(cfg_ssid.c_str(), cfg_password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        setLED(LED_ON);
    }
}

// ============================================================================
// Hardware helpers
// ============================================================================

void setLED(bool on) {
    digitalWrite(STATUS_LED_PIN, on ? LED_ON : LED_OFF);
}

void blinkLED(int count, int delayMs) {
    for (int i = 0; i < count; i++) {
        setLED(LED_ON);
        delay(delayMs);
        setLED(LED_OFF);
        delay(delayMs);
    }
}
