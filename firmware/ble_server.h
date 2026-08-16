// ============================================================================
// BLE Server — Bluetooth pairing for dashboard authentication
// ============================================================================
// The ESP32-S3 advertises a BLE service. When the browser connects via
// Web Bluetooth API, it reads the device info and API key over GATT.
// This replaces manual password entry — the BLE connection IS the auth.
// ============================================================================

#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>

class BLEServerManager {
public:
    void begin();
    void loop();

    // Returns true if a BLE client is currently connected
    bool isClientConnected();

    // Update the device info characteristics (call when stats change)
    void updateDeviceInfo();

private:
    bool _started = false;
    bool _clientConnected = false;
};

extern BLEServerManager BLEMgr;

#endif // BLE_SERVER_H
