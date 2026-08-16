#include "web_server.h"
#include "config.h"
#include "storage.h"
#include "block_list.h"
#include "dns_server.h"
#include "adguard_tester.h"

WebServerManager WebServerMgr;

WebServerManager::WebServerManager() {
    _server = nullptr;
}

WebServerManager::~WebServerManager() {
    if (_server) delete _server;
}

WebServer& WebServerManager::server() {
    return *_server;
}

bool WebServerManager::checkAuth() {
    if (!API_AUTH_ENABLED) return true;
    if (!_server->hasHeader("X-API-Key")) return false;
    String key = _server->header("X-API-Key");
    return (key == API_AUTH_TOKEN);
}

void WebServerManager::addCORSHeaders() {
    _server->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    _server->sendHeader(F("Access-Control-Allow-Methods"), F("GET, POST, DELETE, PUT, OPTIONS"));
    _server->sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type, X-API-Key"));
}

void WebServerManager::sendJSON(int code, const String& json) {
    addCORSHeaders();
    _server->sendHeader(F("Content-Type"), F("application/json"));
    _server->send(code, "application/json", json);
}

void WebServerManager::sendError(int code, const String& message) {
    JsonDocument doc;
    doc["error"] = message;
    String output;
    serializeJson(doc, output);
    sendJSON(code, output);
}

void WebServerManager::begin(uint16_t port) {
    _server = new WebServer(port);

    // Collect custom headers for API auth
    const char* headerKeys[] = {"X-API-Key", "Content-Type"};
    _server->collectHeaders(headerKeys, 2);

    // --- Register all API routes ---
    _server->on("/api/status", HTTP_GET, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleStatus();
    });

    _server->on("/api/stats", HTTP_GET, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleGetStats();
    });

    _server->on("/api/blocklist", HTTP_GET, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleGetBlocklist();
    });

    _server->on("/api/blocklist", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleAddBlocklist();
    });

    _server->on("/api/blocklist", HTTP_DELETE, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleDeleteBlocklist();
    });

    _server->on("/api/blocklist/bulk", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleBulkBlocklist();
    });

    _server->on("/api/blocklist/clear", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleClearBlocklist();
    });

    _server->on("/api/adguard/test", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleAdguardTest();
    });

    _server->on("/api/adguard/results", HTTP_GET, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleAdguardResults();
    });

    _server->on("/api/reports", HTTP_GET, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleGetReports();
    });

    _server->on("/api/reports", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleAddReport();
    });

    _server->on("/api/reports/verify", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleVerifyReport();
    });

    _server->on("/api/reports", HTTP_DELETE, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleDismissReport();
    });

    _server->on("/api/sync", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleSync();
    });

    _server->on("/api/settings", HTTP_GET, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleGetSettings();
    });

    _server->on("/api/settings", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleUpdateSettings();
    });

    // --- Wizard endpoints (no auth — first-run setup) ---
    _server->on("/api/wizard/status", HTTP_GET, [this]() {
        addCORSHeaders();
        handleWizardStatus();
    });

    _server->on("/api/wizard/complete", HTTP_POST, [this]() {
        addCORSHeaders();
        handleWizardComplete();
    });

    // --- Factory reset ---
    _server->on("/api/factory-reset", HTTP_POST, [this]() {
        addCORSHeaders();
        if (!checkAuth()) { sendError(401, "Unauthorized"); return; }
        handleFactoryReset();
    });

    // CORS preflight handler for all /api/ routes
    _server->on("/api", HTTP_OPTIONS, [this]() {
        addCORSHeaders();
        _server->send(204);
    });

    // Static file serving — catch-all for non-API routes
    _server->onNotFound([this]() {
        addCORSHeaders();

        String uri = _server->uri();
        if (uri.startsWith("/api/")) {
            // Unknown API route
            if (_server->method() == HTTP_OPTIONS) {
                _server->send(204);
                return;
            }
            sendError(404, "API endpoint not found");
            return;
        }

        // OPTIONS for static
        if (_server->method() == HTTP_OPTIONS) {
            _server->send(204);
            return;
        }

        handleStaticFile();
    });

    _server->begin();
    DEBUG_PRINTF("[WebServer] Listening on port %d\n", port);
}

void WebServerManager::handleClient() {
    if (_server) _server->handleClient();
}

// ============================================================================
// API Handlers
// ============================================================================

void WebServerManager::handleStatus() {
    JsonDocument doc;

    // System info
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["minFreeHeap"] = ESP.getMinFreeHeap();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["flashSpeed"] = ESP.getFlashChipSpeed();
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();

    // WiFi info
    doc["wifiSSID"] = WiFi.SSID();
    doc["wifiRSSI"] = WiFi.RSSI();
    doc["wifiIP"] = WiFi.localIP().toString();
    doc["wifiMAC"] = WiFi.macAddress();

    // DNS stats summary
    DNSServerStats stats = DNSServer.getRawStats();
    doc["totalQueries"] = stats.totalQueries;
    doc["blockedQueries"] = stats.blockedQueries;
    doc["forwardedQueries"] = stats.forwardedQueries;
    doc["uniqueDomains"] = stats.uniqueDomains;
    doc["blockedCount"] = Blocklist.getBlockedCount();

    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

void WebServerManager::handleGetStats() {
    DNSServerStats s = DNSServer.getRawStats();
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

    // Top blocked domains
    String topBlocked = DNSServer.getTopBlockedDomains(MAX_TOP_DOMAINS);
    JsonDocument topDoc;
    DeserializationError err = deserializeJson(topDoc, topBlocked);
    if (!err) {
        doc["topBlockedDomains"] = topDoc.as<JsonArray>();
    }

    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

void WebServerManager::handleGetBlocklist() {
    int offset = 0;
    int limit = 50;
    String search = "";

    if (_server->hasArg("offset")) offset = _server->arg("offset").toInt();
    if (_server->hasArg("limit")) limit = _server->arg("limit").toInt();
    if (_server->hasArg("search")) search = _server->arg("search");

    if (limit <= 0 || limit > 500) limit = 50;
    if (offset < 0) offset = 0;

    String domains = Blocklist.getBlockedDomains(offset, limit, search);

    JsonDocument doc;
    doc["total"] = Blocklist.getBlockedCount();
    doc["offset"] = offset;
    doc["limit"] = limit;
    doc["search"] = search;

    JsonDocument domainsDoc;
    DeserializationError err = deserializeJson(domainsDoc, domains);
    if (!err) {
        doc["domains"] = domainsDoc.as<JsonArray>();
    }

    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

void WebServerManager::handleAddBlocklist() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }
    if (!doc["domain"].is<String>()) {
        sendError(400, "Missing 'domain' field");
        return;
    }
    String domain = doc["domain"].as<String>();
    bool added = Blocklist.addDomain(domain);
    if (added) {
        JsonDocument resp;
        resp["success"] = true;
        resp["domain"] = domain;
        resp["total"] = Blocklist.getBlockedCount();
        String output;
        serializeJson(resp, output);
        sendJSON(200, output);
    } else {
        sendError(409, "Domain already exists or block list full");
    }
}

void WebServerManager::handleDeleteBlocklist() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }
    if (!doc["domain"].is<String>()) {
        sendError(400, "Missing 'domain' field");
        return;
    }
    String domain = doc["domain"].as<String>();
    bool removed = Blocklist.removeDomain(domain);
    if (removed) {
        JsonDocument resp;
        resp["success"] = true;
        resp["domain"] = domain;
        resp["total"] = Blocklist.getBlockedCount();
        String output;
        serializeJson(resp, output);
        sendJSON(200, output);
    } else {
        sendError(404, "Domain not found in block list");
    }
}

void WebServerManager::handleBulkBlocklist() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }
    if (!doc["domains"].is<JsonArray>()) {
        sendError(400, "Missing 'domains' array");
        return;
    }

    JsonArray domains = doc["domains"].as<JsonArray>();
    int added = 0;
    int alreadyExists = 0;

    for (JsonVariant v : domains) {
        if (!v.is<const char*>()) continue;
        String domain = v.as<String>();
        if (Blocklist.addDomain(domain)) {
            added++;
        } else {
            alreadyExists++;
        }
        yield();
    }

    Blocklist.save();

    JsonDocument resp;
    resp["added"] = added;
    resp["alreadyExists"] = alreadyExists;
    resp["total"] = Blocklist.getBlockedCount();
    String output;
    serializeJson(resp, output);
    sendJSON(200, output);
}

void WebServerManager::handleClearBlocklist() {
    Blocklist.clearAll();
    JsonDocument resp;
    resp["success"] = true;
    resp["total"] = 0;
    String output;
    serializeJson(resp, output);
    sendJSON(200, output);
}

void WebServerManager::handleAdguardTest() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }
    if (!doc["domains"].is<JsonArray>()) {
        sendError(400, "Missing 'domains' array");
        return;
    }

    bool autoAdd = false;
    if (doc["autoAdd"].is<bool>()) {
        autoAdd = doc["autoAdd"].as<bool>();
    }

    String domainsJson;
    serializeJson(doc["domains"], domainsJson);

    String results = AdGuardTest.testDomains(domainsJson, autoAdd);

    JsonDocument respDoc;
    DeserializationError rErr = deserializeJson(respDoc, results);
    if (rErr) {
        sendError(500, "Failed to parse test results");
        return;
    }

    JsonDocument outDoc;
    outDoc["success"] = true;
    outDoc["results"] = respDoc.as<JsonArray>();
    outDoc["testedCount"] = AdGuardTest.getLastTestedCount();
    outDoc["totalCount"] = AdGuardTest.getLastTotalCount();

    if (autoAdd) {
        outDoc["blocklistCount"] = Blocklist.getBlockedCount();
    }

    String output;
    serializeJson(outDoc, output);
    sendJSON(200, output);
}

void WebServerManager::handleAdguardResults() {
    String results = AdGuardTest.getTestResults();
    sendJSON(200, results);
}

void WebServerManager::handleGetReports() {
    String reports = getReportsJSON();
    sendJSON(200, reports);
}

String WebServerManager::getReportsJSON() {
    String content = Storage.readFile(FS_REPORTS_PATH);
    if (content.length() == 0) {
        return "[]";
    }
    return content;
}

bool WebServerManager::addReport(const String& domain, const String& source, const String& reportedBy) {
    // Read existing reports
    String content = Storage.readFile(FS_REPORTS_PATH);
    JsonDocument doc;
    JsonArray arr;
    if (content.length() > 0) {
        DeserializationError err = deserializeJson(doc, content);
        if (err) {
            arr = doc.to<JsonArray>();
        } else {
            arr = doc.as<JsonArray>();
        }
    } else {
        arr = doc.to<JsonArray>();
    }

    // Check max reports
    if ((int)arr.size() >= MAX_REPORTS) {
        arr.remove(0); // remove oldest
    }

    // Check for duplicate
    for (JsonVariant v : arr) {
        if (v["domain"].is<String>() && v["domain"].as<String>() == domain) {
            return false; // already reported
        }
    }

    JsonObject obj = arr.add<JsonObject>();
    obj["domain"] = domain;
    obj["source"] = source;
    obj["reportedBy"] = reportedBy;
    obj["timestamp"] = (uint32_t)(millis() / 1000);
    obj["status"] = "pending";

    String output;
    serializeJson(arr, output);
    return Storage.writeFile(FS_REPORTS_PATH, output);
}

void WebServerManager::handleAddReport() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }
    if (!doc["domain"].is<String>()) {
        sendError(400, "Missing 'domain' field");
        return;
    }
    String domain = doc["domain"].as<String>();
    String source = doc["source"] | "unknown";
    String reportedBy = doc["reportedBy"] | "anonymous";

    bool ok = addReport(domain, source, reportedBy);
    if (ok) {
        JsonDocument resp;
        resp["success"] = true;
        resp["domain"] = domain;
        String output;
        serializeJson(resp, output);
        sendJSON(200, output);
    } else {
        sendError(409, "Domain already reported");
    }
}

bool WebServerManager::verifyAndBlockReport(const String& domain) {
    // Verify the domain resolves via DNS lookup
    IPAddress resolved;
    if (WiFi.hostByName(domain.c_str(), resolved, 2000)) {
        // Domain resolves — it's a real ad domain, add to blocklist
        Blocklist.addDomain(domain);
        // Update report status to "verified"
        String content = Storage.readFile(FS_REPORTS_PATH);
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, content);
        if (!err && doc.is<JsonArray>()) {
            JsonArray arr = doc.as<JsonArray>();
            for (JsonVariant v : arr) {
                if (v["domain"].is<String>() && v["domain"].as<String>() == domain) {
                    v["status"] = "verified";
                    v["resolvedIP"] = resolved.toString();
                    v["addedToBlockList"] = true;
                    break;
                }
            }
            String output;
            serializeJson(arr, output);
            Storage.writeFile(FS_REPORTS_PATH, output);
        }
        return true;
    }
    return false; // couldn't resolve
}

void WebServerManager::handleVerifyReport() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }
    if (!doc["domain"].is<String>()) {
        sendError(400, "Missing 'domain' field");
        return;
    }
    String domain = doc["domain"].as<String>();

    bool verified = verifyAndBlockReport(domain);
    JsonDocument resp;
    resp["domain"] = domain;
    if (verified) {
        resp["verified"] = true;
        resp["addedToBlocklist"] = true;
        String output;
        serializeJson(resp, output);
        sendJSON(200, output);
    } else {
        resp["verified"] = false;
        resp["message"] = "Domain could not be resolved";
        String output;
        serializeJson(resp, output);
        sendJSON(200, output);
    }
}

bool WebServerManager::dismissReport(const String& domain) {
    String content = Storage.readFile(FS_REPORTS_PATH);
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) return false;
    if (!doc.is<JsonArray>()) return false;

    JsonArray arr = doc.as<JsonArray>();
    bool found = false;
    for (int i = (int)arr.size() - 1; i >= 0; i--) {
        if (arr[i]["domain"].is<String>() && arr[i]["domain"].as<String>() == domain) {
            arr.remove(i);
            found = true;
        }
    }

    if (found) {
        String output;
        serializeJson(arr, output);
        Storage.writeFile(FS_REPORTS_PATH, output);
    }
    return found;
}

void WebServerManager::handleDismissReport() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }
    if (!doc["domain"].is<String>()) {
        sendError(400, "Missing 'domain' field");
        return;
    }
    String domain = doc["domain"].as<String>();
    bool ok = dismissReport(domain);
    if (ok) {
        JsonDocument resp;
        resp["success"] = true;
        resp["domain"] = domain;
        String output;
        serializeJson(resp, output);
        sendJSON(200, output);
    } else {
        sendError(404, "Report not found");
    }
}

void WebServerManager::handleSync() {
    // Sync block list from cloud (Base44 backend)
    // This is a placeholder for cloud sync — the actual implementation
    // would use WiFiClient to call the Base44 backend API and pull the
    // latest block list.
    //
    // For now, we read from a sync file if present.
    String syncData = Storage.readFile("/sync_data.json");
    if (syncData.length() > 0) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, syncData);
        if (!err && doc["domains"].is<JsonArray>()) {
            int added = 0;
            JsonArray arr = doc["domains"].as<JsonArray>();
            for (JsonVariant v : arr) {
                if (v.is<const char*>()) {
                    if (Blocklist.addDomain(v.as<String>())) added++;
                }
            }
            JsonDocument resp;
            resp["success"] = true;
            resp["synced"] = added;
            resp["total"] = Blocklist.getBlockedCount();
            String output;
            serializeJson(resp, output);
            sendJSON(200, output);
            return;
        }
    }

    // No sync data available — return current block list for export
    std::vector<String> allDomains;
    Blocklist.getAllDomains(allDomains);

    JsonDocument resp;
    resp["success"] = false;
    resp["message"] = "No sync data found. Upload sync_data.json to LittleFS root.";
    resp["currentCount"] = (int)allDomains.size();
    String output;
    serializeJson(resp, output);
    sendJSON(200, output);
}

// --- Settings ---

String WebServerManager::getSettingsJSON() {
    String content = Storage.readFile(FS_SETTINGS_PATH);
    if (content.length() == 0) {
        // Return defaults
        JsonDocument doc;
        doc["routerDNSIP"] = ROUTER_DNS_IP;
        doc["upstreamDNS1"] = DNS_UPSTREAM_1;
        doc["upstreamDNS2"] = DNS_UPSTREAM_2;
        doc["dnsPort"] = DNS_PORT;
        doc["webPort"] = WEB_SERVER_PORT;
        doc["debug"] = DEBUG;
        String output;
        serializeJson(doc, output);
        return output;
    }
    return content;
}

void WebServerManager::handleGetSettings() {
    sendJSON(200, getSettingsJSON());
}

bool WebServerManager::updateSettingsFromJSON(const String& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return false;

    // Merge with existing settings
    String existing = Storage.readFile(FS_SETTINGS_PATH);
    JsonDocument merged;
    if (existing.length() > 0) {
        DeserializationError e2 = deserializeJson(merged, existing);
        if (e2) merged = doc;
    }

    // Update provided fields
    if (doc["routerDNSIP"].is<String>()) {
        merged["routerDNSIP"] = doc["routerDNSIP"].as<String>();
        AdGuardTest.setRouterIP(doc["routerDNSIP"].as<String>());
    }
    if (doc["upstreamDNS1"].is<String>()) merged["upstreamDNS1"] = doc["upstreamDNS1"].as<String>();
    if (doc["upstreamDNS2"].is<String>()) merged["upstreamDNS2"] = doc["upstreamDNS2"].as<String>();
    if (doc["dnsPort"].is<int>()) merged["dnsPort"] = doc["dnsPort"].as<int>();
    if (doc["webPort"].is<int>()) merged["webPort"] = doc["webPort"].as<int>();
    if (doc["debug"].is<bool>()) merged["debug"] = doc["debug"].as<bool>();

    String output;
    serializeJson(merged, output);
    return Storage.writeFile(FS_SETTINGS_PATH, output);
}

void WebServerManager::handleUpdateSettings() {
    if (!_server->hasArg("plain")) {
        sendError(400, "Missing body");
        return;
    }
    String body = _server->arg("plain");
    bool ok = updateSettingsFromJSON(body);
    if (ok) {
        sendJSON(200, getSettingsJSON());
    } else {
        sendError(500, "Failed to update settings");
    }
}

// --- Static file serving ---

void WebServerManager::handleStaticFile() {
    String uri = _server->uri();
    if (uri == "/") uri = "/index.html";

    // Prevent path traversal
    if (uri.indexOf("..") >= 0) {
        sendError(400, "Bad request");
        return;
    }

    String path = uri;
    if (!Storage.fileExists(path)) {
        // Try with .html extension
        if (!Storage.fileExists(path + ".html")) {
            // If no file found and it's not an API route, serve index.html
            // (SPA fallback)
            if (Storage.fileExists("/index.html")) {
                path = "/index.html";
            } else {
                sendError(404, "File not found");
                return;
            }
        } else {
            path = path + ".html";
        }
    }

    String content = Storage.readFile(path);

    // Determine content type
    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".json")) contentType = "application/json";
    else if (path.endsWith(".png")) contentType = "image/png";
    else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) contentType = "image/jpeg";
    else if (path.endsWith(".gif")) contentType = "image/gif";
    else if (path.endsWith(".svg")) contentType = "image/svg+xml";
    else if (path.endsWith(".ico")) contentType = "image/x-icon";
    else if (path.endsWith(".woff")) contentType = "font/woff";
    else if (path.endsWith(".woff2")) contentType = "font/woff2";
    else if (path.endsWith(".ttf")) contentType = "font/ttf";

    addCORSHeaders();
    _server->send(200, contentType, content);
}

void WebServerManager::handleNotFound() {
    addCORSHeaders();
    sendError(404, "Not found");
}

// ============================================================================
// Wizard Endpoints — first-run setup (no auth required)
// ============================================================================

void WebServerManager::handleWizardStatus() {
    // Returns setup status for the first-run web wizard
    // No auth required — this is the first thing the user sees

    JsonDocument doc;

    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ip"] = WiFi.localIP().toString();
    doc["hostname"] = "esp32-pihole.local";
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["testRun"] = AdGuardTest.hasTestRun();
    doc["blockListSize"] = Blocklist.getBlockedCount();
    doc["wizardCompleted"] = SetupWizard::isWizardCompleted();

    DNSServerStats s = DNSServer.getRawStats();
    doc["totalQueries"] = s.totalQueries;
    doc["blockedQueries"] = s.blockedQueries;

    // Test summary
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

    // Uptime
    doc["uptime"] = millis() / 1000;

    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

void WebServerManager::handleWizardComplete() {
    // Mark the first-run wizard as completed
    SetupWizard::markWizardCompleted();

    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Wizard completed. Welcome to your ESP32-S3 Pi-Hole!";
    doc["ip"] = WiFi.localIP().toString();
    doc["hostname"] = "esp32-pihole.local";

    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

// ============================================================================
// Factory Reset — clears WiFi config and reboots
// ============================================================================

void WebServerManager::handleFactoryReset() {
    // Clear config files
    Storage.deleteFile("/wifi_config.json");
    Storage.deleteFile("/adblock_test_done.flag");
    Storage.deleteFile("/wizard_done.flag");

    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Factory reset complete. Reflash with new wifi_config.json.";
    String output;
    serializeJson(doc, output);
    sendJSON(200, output);

    delay(2000);
    ESP.restart();
}
