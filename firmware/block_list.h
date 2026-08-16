#ifndef BLOCK_LIST_H
#define BLOCK_LIST_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>

// ============================================================================
// Block List Manager — domain block list with LittleFS persistence
// ============================================================================
// Stores domains in a file (one per line) and in an in-memory vector
// for fast lookups. Supports exact match and wildcard (*.domain.com).
// ============================================================================

class BlockList {
public:
    BlockList();
    ~BlockList();

    // Load block list from LittleFS into memory. Returns count loaded.
    int load();

    // Save entire in-memory block list to LittleFS. Returns true on success.
    bool save();

    // Add a domain to the block list. Returns true if added (false = dup or full).
    bool addDomain(const String& domain);

    // Remove a domain from the block list. Returns true if removed.
    bool removeDomain(const String& domain);

    // Check if a domain is blocked (exact or wildcard match).
    bool isBlocked(const String& domain);

    // Get total number of blocked domains in memory.
    int getBlockedCount();

    // Get a page of blocked domains. Returns JSON array string.
    // offset: starting index, limit: max entries, search: filter substring
    String getBlockedDomains(int offset, int limit, const String& search);

    // Clear entire block list (memory + file).
    bool clearAll();

    // Get all domains as a vector (for sync/export).
    void getAllDomains(std::vector<String>& out);

    // Bulk add domains from a JSON array string.
    int addBulk(const String& jsonArray);

private:
    // Normalize domain to lowercase, trim whitespace and dots.
    String normalize(const String& domain);

    // Check wildcard match: domain matches pattern *.something
    bool wildcardMatch(const String& pattern, const String& domain);

    SemaphoreHandle_t _mutex;
    std::vector<String> _domains;          // all domains including wildcards
    std::vector<String> _exactDomains;      // exact-match domains
    std::vector<String> _wildcardPatterns;  // patterns starting with *.

    bool _dirty; // true if in-memory list changed and needs saving
};

// Global instance
extern BlockList Blocklist;

#endif // BLOCK_LIST_H
