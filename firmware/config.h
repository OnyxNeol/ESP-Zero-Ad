#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// ESP32-S3 DNS Sinkhole Ad Blocker — Configuration Header
// ============================================================================
// User-editable configuration. Fill in WiFi credentials and adjust settings
// as needed. Everything else uses sensible defaults.
// ============================================================================

// ---------------------------------------------------------------------------
// WiFi Configuration
// ---------------------------------------------------------------------------
// Fill in your WiFi credentials here.
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"

// WiFi connection timeout in milliseconds before giving up
#define WIFI_CONNECT_TIMEOUT_MS   20000
// Retry interval between connection attempts (ms)
#define WIFI_RETRY_INTERVAL_MS     5000
// Number of WiFi connection attempts before rebooting
#define WIFI_MAX_RETRIES           20

// ---------------------------------------------------------------------------
// Network Ports
// ---------------------------------------------------------------------------
#define DNS_PORT            53
#define WEB_SERVER_PORT     80

// ---------------------------------------------------------------------------
// Upstream DNS Servers
// ---------------------------------------------------------------------------
// Used to forward non-blocked DNS queries to the real DNS resolver.
#define DNS_UPSTREAM_1      "8.8.8.8"     // Google Public DNS
#define DNS_UPSTREAM_2      "1.1.1.1"     // Cloudflare DNS

// ---------------------------------------------------------------------------
// Router DNS IP (for Ad Blocker Testing)
// ---------------------------------------------------------------------------
// The IP of your  router (or any router) running ad blocker service.
// The ad blocker tester sends DNS queries here to check whether the router
// can/will block a given domain.
#define ROUTER_DNS_IP       "192.168.1.1"

// ---------------------------------------------------------------------------
// LittleFS Storage Paths
// ---------------------------------------------------------------------------
#define FS_BLOCKLIST_PATH       "/blocklist.txt"
#define FS_STATS_PATH            "/stats.json"
#define FS_SETTINGS_PATH         "/settings.json"
#define FS_TEST_RESULTS_PATH     "/test_results.json"
#define FS_REPORTS_PATH          "/reports.json"
#define FS_WEB_ROOT              "/"   // Web dashboard files served from here

// ---------------------------------------------------------------------------
// Block List Limits
// ---------------------------------------------------------------------------
#define BLOCKLIST_MAX_DOMAINS     10000   // Max domains in memory set
#define BLOCKLIST_LINE_MAX_LEN    256     // Max length of a domain string
#define BLOCKLIST_HASHSET_SIZE    1       // Use dynamic set (unordered)

// ---------------------------------------------------------------------------
// API Authentication
// ---------------------------------------------------------------------------
// Simple token-based auth. Clients must send header: X-API-Key: <token>
#define API_AUTH_TOKEN            "esp32-pihole-change-this-token"
#define API_AUTH_ENABLED          true

// ---------------------------------------------------------------------------
// Stats and Persistence
// ---------------------------------------------------------------------------
// Save stats to LittleFS every N milliseconds.
#define STATS_SAVE_INTERVAL_MS    60000

// Max top-blocked-domains entries to track/report.
#define MAX_TOP_DOMAINS           50

// Max reported ads stored.
#define MAX_REPORTS               100

// ---------------------------------------------------------------------------
// Debug / Serial Settings
// ---------------------------------------------------------------------------
#define DEBUG               true
#define SERIAL_BAUD_RATE    115200

#if DEBUG
  #define DEBUG_PRINT(x)      Serial.print(x)
  #define DEBUG_PRINTLN(x)    Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...)  Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#endif

// ---------------------------------------------------------------------------
// Status LED (ESP32-S3 built-in or external)
// ---------------------------------------------------------------------------
#define STATUS_LED_PIN     2       // Common onboard LED pin
#define LED_ON             HIGH
#define LED_OFF            LOW

// ---------------------------------------------------------------------------
// Sinkhole IPs returned for blocked domains
// ---------------------------------------------------------------------------
#define SINKHOLE_IP_0      0
#define SINKHOLE_IP_1      0
#define SINKHOLE_IP_2      0
#define SINKHOLE_IP_3      0   // 0.0.0.0

// ---------------------------------------------------------------------------
// Known sinkhole response IPs (for ad blocker tester detection)
// ---------------------------------------------------------------------------
// If the router returns any of these, the domain is considered blocked.
// 0.0.0.0, 0.0.0.1, and common ad blocker sinkhole IPs.
#define KNOWN_SINKHOLE_IP_COUNT   6

#endif // CONFIG_H
