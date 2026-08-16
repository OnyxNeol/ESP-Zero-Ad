#ifndef SETUP_WIZARD_H
#define SETUP_WIZARD_H

#include <Arduino.h>

// ============================================================================
// Setup Wizard — File-Based Configuration (NO terminal, NO hotspot)
// ============================================================================
// The user fills in firmware/data/wifi_config.json BEFORE flashing.
// This file gets uploaded to LittleFS along with the web dashboard.
// On boot, the ESP32-S3 reads the config, connects to WiFi, runs the
// AdBlock test automatically, and starts the DNS server + web dashboard.
//
// The user finds the dashboard at http://esp32-pihole.local (mDNS).
// The first-run web wizard shows the IP, test results, and next steps.
//
// ZERO terminal interaction required. ZERO hotspots created.
// ============================================================================

class SetupWizard {
public:
    // Check if wifi_config.json exists and has valid credentials
    static bool isConfigured();

    // Load saved configuration from LittleFS (wifi_config.json)
    static bool loadConfig(String& ssid, String& password,
                           String& routerIP, String& apiKey);

    // Generate a random API key if user set "auto" in config
    static String generateAPIKey();

    // Check if the user has completed the first-run wizard (acknowledged)
    static bool isWizardCompleted();

    // Mark the first-run wizard as completed
    static void markWizardCompleted();

    // Get the status for the first-run web wizard
    // Returns JSON: { connected, ip, testRun, testResults, blockListSize, apiKey }
    static String getWizardStatus();

private:
    static const char* CONFIG_PATH;
    static const char* WIZARD_FLAG_PATH;
};

#endif // SETUP_WIZARD_H
