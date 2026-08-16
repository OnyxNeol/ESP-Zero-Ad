// ============================================================================
// Setup Wizard — File-Based Configuration (NO terminal, NO hotspot)
// ============================================================================
// Zero terminal interaction. Zero hotspots. All real.
// The user fills in wifi_config.json before flashing. The ESP32-S3
// reads it on boot, connects to WiFi, runs the AdBlock test, and
// serves the dashboard at http://esp32-pihole.local.
// ============================================================================

#include "setup_wizard.h"
#include "storage.h"
#include "config.h"
#include "block_list.h"
#include "dns_server.h"
#include "adguard_tester.h"
#include <ArduinoJson.h>
#include <WiFi.h>

const char* SetupWizard::CONFIG_PATH = "/wifi_config.json";
const char* SetupWizard::WIZARD_FLAG_PATH = "/wizard_done.flag";

// ============================================================================
// Public Methods
// ============================================================================

bool SetupWizard::isConfigured() {
    String content = Storage.readFile(CONFIG_PATH);
    if (content.length() == 0) return false;

    // Check if SSID is still the placeholder
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) return false;

    String ssid = doc["ssid"] | "";
    if (ssid.length() == 0) return false;
    if (ssid == "YOUR_WIFI_SSID") return false;

    return true;
}

bool SetupWizard::loadConfig(String& ssid, String& password,
                              String& routerIP, String& apiKey) {
    String content = Storage.readFile(CONFIG_PATH);
    if (content.length() == 0) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) return false;

    ssid = doc["ssid"] | "";
    password = doc["password"] | "";
    routerIP = doc["routerIP"] | "192.168.1.1";
    apiKey = doc["apiKey"] | "auto";

    // If apiKey is "auto" or empty, generate one
    if (apiKey == "auto" || apiKey.length() == 0) {
        apiKey = generateAPIKey();

        // Save the generated key back to config
        doc["apiKey"] = apiKey;
        String updated;
        serializeJson(doc, updated);
        Storage.writeFile(CONFIG_PATH, updated);
    }

    return ssid.length() > 0 && ssid != "YOUR_WIFI_SSID";
}

String SetupWizard::generateAPIKey() {
    uint64_t mac = ESP.getEfuseMac();
    char key[48];
    snprintf(key, sizeof(key), "ph-%04X%04X%04X-%04X",
             (uint16_t)(mac >> 32),
             (uint16_t)(mac >> 16),
             (uint16_t)mac,
             (uint16_t)esp_random());
    return String(key);
}

bool SetupWizard::isWizardCompleted() {
    return Storage.fileExists(WIZARD_FLAG_PATH);
}

void SetupWizard::markWizardCompleted() {
    Storage.writeFile(WIZARD_FLAG_PATH, String(millis()));
}

String SetupWizard::getWizardStatus() {
    JsonDocument doc;

    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ip"] = WiFi.localIP().toString();
    doc["hostname"] = "esp32-pihole.local";
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["testRun"] = AdGuardTest.hasTestRun();
    doc["blockListSize"] = Blocklist.getBlockedCount();
    doc["wizardCompleted"] = isWizardCompleted();

    DNSServerStats s = DNSServer.getRawStats();
    doc["totalQueries"] = s.totalQueries;
    doc["blockedQueries"] = s.blockedQueries;

    // Test summary if available
    const auto& results = AdGuardTest.getLastResults();
    int routerBlocks = 0;
    int routerDoesNotBlock = 0;
    for (const auto& r : results) {
        if (r.routerBlocks) routerBlocks++;
        else routerDoesNotBlock++;
    }
    doc["testTotal"] = (int)results.size();
    doc["testRouterBlocks"] = routerBlocks;
    doc["testNotBlocked"] = routerDoesNotBlock;

    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// Private Methods — removed (no serial interaction needed)
// ============================================================================
