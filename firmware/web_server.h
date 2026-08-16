#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>

class WebServerManager {
public:
    WebServerManager();
    ~WebServerManager();

    void begin(uint16_t port = WEB_SERVER_PORT);
    void handleClient();
    WebServer& server();

private:
    WebServer* _server;

    bool checkAuth();
    void addCORSHeaders();
    void sendJSON(int code, const String& json);
    void sendError(int code, const String& message);

    // --- API handlers ---
    void handleStatus();
    void handleGetStats();
    void handleGetBlocklist();
    void handleAddBlocklist();
    void handleDeleteBlocklist();
    void handleBulkBlocklist();
    void handleClearBlocklist();
    void handleAdguardTest();
    void handleAdguardResults();
    void handleGetReports();
    void handleAddReport();
    void handleVerifyReport();
    void handleDismissReport();
    void handleSync();
    void handleGetSettings();
    void handleUpdateSettings();
    void handleFactoryReset();

    // --- Wizard endpoints (no auth required — first-run setup) ---
    void handleWizardStatus();
    void handleWizardComplete();

    // --- Static file serving ---
    void handleStaticFile();
    void handleNotFound();

    String getSettingsJSON();
    bool updateSettingsFromJSON(const String& json);
    String getReportsJSON();
    bool addReport(const String& domain, const String& source, const String& reportedBy);
    bool dismissReport(const String& domain);
    bool verifyAndBlockReport(const String& domain);
};

extern WebServerManager WebServerMgr;

#endif // WEB_SERVER_H
