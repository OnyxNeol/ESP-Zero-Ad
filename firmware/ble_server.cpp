// ============================================================================
// BLE Server — Bluetooth pairing for dashboard authentication
// ============================================================================
// Uses ESP32 BLE GATT server. The browser connects via Web Bluetooth API,
// reads the API key and device info over BLE, then uses the key for HTTP API.
//
// BLE Service:  "ESP-Zero-Ad" (UUID: 0000ffe0-0000-1000-8000-00805f9b34fb)
// Characteristics:
//   - Device Info  (read):  JSON with IP, WiFi, block list size, stats
//   - API Key      (read):  The dashboard password for HTTP API auth
// ============================================================================

#include "ble_server.h"
#include "config.h"
#include "storage.h"
#include "block_list.h"
#include "dns_server.h"
#include "adguard_tester.h"
#include "setup_wizard.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServerManager BLEMgr;

// Service and characteristic UUIDs
#define SERVICE_UUID           "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHAR_DEVICE_INFO_UUID  "0000ffe1-0000-1000-8000-00805f9b34fb"
#define CHAR_API_KEY_UUID      "0000ffe2-0000-1000-8000-00805f9b34fb"

BLEServer* pServer = nullptr;
BLECharacteristic* pDeviceInfoChar = nullptr;
BLECharacteristic* pApiKeyChar = nullptr;
bool deviceConnected = false;

// ============================================================================
// BLE Server Callbacks
// ============================================================================
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) {
        deviceConnected = true;
        DEBUG_PRINTLN(F("[BLE] Client connected"));
    }

    void onDisconnect(BLEServer* server) {
        deviceConnected = false;
        DEBUG_PRINTLN(F("[BLE] Client disconnected"));
        // Restart advertising so new clients can find us
        server->getAdvertising()->start();
        DEBUG_PRINTLN(F("[BLE] Advertising restarted"));
    }
};

// ============================================================================
// BLEServerManager
// ============================================================================

void BLEServerManager::begin() {
    DEBUG_PRINTLN(F("[BLE] Starting BLE server…"));

    BLEDevice::init("ESP-Zero-Ad");
    BLEDevice::setMTU(512);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Device Info characteristic (read-only)
    pDeviceInfoChar = pService->createCharacteristic(
        CHAR_DEVICE_INFO_UUID,
        BLECharacteristic::PROPERTY_READ
    );

    // API Key characteristic (read-only — the BLE connection IS the auth)
    pApiKeyChar = pService->createCharacteristic(
        CHAR_API_KEY_UUID,
        BLECharacteristic::PROPERTY_READ
    );

    // Set initial values
    updateDeviceInfo();

    // Load the API key from config
    String ssid, password, routerIP, apiKey;
    SetupWizard::loadConfig(ssid, password, routerIP, apiKey);
    if (apiKey.length() == 0) apiKey = API_AUTH_TOKEN;
    pApiKeyChar->setValue(apiKey.c_str());

    pService->start();

    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferredInterval(0x06);
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x40);
    BLEDevice::startAdvertising();

    _started = true;
    DEBUG_PRINTLN(F("[BLE] BLE server started and advertising"));
    DEBUG_PRINTLN(F("[BLE] Connect via Web Bluetooth: 'ESP-Zero-Ad'"));
}

void BLEServerManager::loop() {
    // Nothing needed here — BLE is event-driven
}

bool BLEServerManager::isClientConnected() {
    return deviceConnected;
}

void BLEServerManager::updateDeviceInfo() {
    if (!pDeviceInfoChar) return;

    // Build JSON with device info
    JsonDocument doc;
    doc["hostname"] = "esp32-pihole.local";
    doc["ip"] = WiFi.localIP().toString();
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["blockListSize"] = Blocklist.getBlockedCount();
    doc["testRun"] = AdGuardTest.hasTestRun();

    DNSServerStats s = DNSServer.getRawStats();
    doc["totalQueries"] = s.totalQueries;
    doc["blockedQueries"] = s.blockedQueries;
    doc["blockedPercent"] = s.totalQueries > 0
        ? (int)((s.blockedQueries * 100) / s.totalQueries) : 0;
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();

    String output;
    serializeJson(doc, output);
    pDeviceInfoChar->setValue(output.c_str());
}
