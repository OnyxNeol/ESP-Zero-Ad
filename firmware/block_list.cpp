#include "block_list.h"
#include "config.h"
#include "storage.h"

BlockList Blocklist;

BlockList::BlockList() {
    _mutex = xSemaphoreCreateMutex();
    _dirty = false;
}

BlockList::~BlockList() {
    if (_mutex) vSemaphoreDelete(_mutex);
}

String BlockList::normalize(const String& domain) {
    String d = domain;
    d.trim();
    d.toLowerCase();
    // Remove leading/trailing dots
    while (d.length() > 0 && d.charAt(0) == '.') d.remove(0, 1);
    while (d.length() > 0 && d.charAt(d.length() - 1) == '.') d.remove(d.length() - 1);
    return d;
}

int BlockList::load() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return -1;

    _domains.clear();
    _exactDomains.clear();
    _wildcardPatterns.clear();

    int count = Storage.readFileLines(FS_BLOCKLIST_PATH, [this](const String& line) {
        String d = normalize(line);
        if (d.length() == 0 || d.length() > BLOCKLIST_LINE_MAX_LEN) return;
        // Check for duplicate
        for (const String& existing : _domains) {
            if (existing == d) return;
        }
        _domains.push_back(d);
        if (d.startsWith("*.")) {
            _wildcardPatterns.push_back(d);
        } else {
            _exactDomains.push_back(d);
        }
    });

    _dirty = false;
    xSemaphoreGive(_mutex);

    DEBUG_PRINTF("[BlockList] Loaded %d domains from LittleFS\n", count);
    return count;
}

bool BlockList::save() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return false;

    String content = "";
    for (const String& d : _domains) {
        content += d + "\n";
    }

    xSemaphoreGive(_mutex);

    bool ok = Storage.writeFile(FS_BLOCKLIST_PATH, content);
    if (ok) {
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _dirty = false;
            xSemaphoreGive(_mutex);
        }
        DEBUG_PRINTF("[BlockList] Saved %d domains to LittleFS\n", (int)_domains.size());
    } else {
        DEBUG_PRINTLN(F("[BlockList] Failed to save!"));
    }
    return ok;
}

bool BlockList::addDomain(const String& domain) {
    String d = normalize(domain);
    if (d.length() == 0 || d.length() > BLOCKLIST_LINE_MAX_LEN) return false;

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return false;

    // Check max size
    if ((int)_domains.size() >= BLOCKLIST_MAX_DOMAINS) {
        xSemaphoreGive(_mutex);
        DEBUG_PRINTLN(F("[BlockList] Max size reached!"));
        return false;
    }

    // Check for duplicate
    for (const String& existing : _domains) {
        if (existing == d) {
            xSemaphoreGive(_mutex);
            return false; // already exists
        }
    }

    _domains.push_back(d);
    if (d.startsWith("*.")) {
        _wildcardPatterns.push_back(d);
    } else {
        _exactDomains.push_back(d);
    }
    _dirty = true;

    xSemaphoreGive(_mutex);

    // Append to file (faster than full rewrite for single adds)
    Storage.appendFile(FS_BLOCKLIST_PATH, d);
    DEBUG_PRINTF("[BlockList] Added domain: %s\n", d.c_str());
    return true;
}

bool BlockList::removeDomain(const String& domain) {
    String d = normalize(domain);
    if (d.length() == 0) return false;

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return false;

    bool found = false;
    for (size_t i = 0; i < _domains.size(); i++) {
        if (_domains[i] == d) {
            _domains.erase(_domains.begin() + i);
            found = true;
            break;
        }
    }
    if (found) {
        // Rebuild exact/wildcard vectors
        _exactDomains.clear();
        _wildcardPatterns.clear();
        for (const String& dom : _domains) {
            if (dom.startsWith("*.")) {
                _wildcardPatterns.push_back(dom);
            } else {
                _exactDomains.push_back(dom);
            }
        }
        _dirty = true;
    }

    xSemaphoreGive(_mutex);

    if (found) {
        save(); // full rewrite needed after removal
        DEBUG_PRINTF("[BlockList] Removed domain: %s\n", d.c_str());
    }
    return found;
}

bool BlockList::wildcardMatch(const String& pattern, const String& domain) {
    // pattern is like "*.example.com"
    // matches "example.com" and any subdomain "foo.example.com"
    String suffix = pattern.substring(2); // remove "*."
    if (domain == suffix) return true;
    if (domain.endsWith("." + suffix)) return true;
    return false;
}

bool BlockList::isBlocked(const String& domain) {
    String d = normalize(domain);
    if (d.length() == 0) return false;

    if (xSemaphoreTake(_mutex, 5 / portTICK_PERIOD_MS) != pdTRUE) {
        // Mutex busy — allow to pass (don't block DNS for long)
        return false;
    }

    // Exact match
    for (const String& ex : _exactDomains) {
        if (ex == d) {
            xSemaphoreGive(_mutex);
            return true;
        }
    }

    // Wildcard match
    for (const String& wc : _wildcardPatterns) {
        if (wildcardMatch(wc, d)) {
            xSemaphoreGive(_mutex);
            return true;
        }
    }

    xSemaphoreGive(_mutex);
    return false;
}

int BlockList::getBlockedCount() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return 0;
    int count = (int)_domains.size();
    xSemaphoreGive(_mutex);
    return count;
}

String BlockList::getBlockedDomains(int offset, int limit, const String& search) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return "[]";

    String searchLower = search;
    searchLower.toLowerCase();

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    int idx = 0;
    int added = 0;
    for (const String& d : _domains) {
        bool matches = (search.length() == 0);
        if (!matches) {
            String dl = d;
            dl.toLowerCase();
            matches = (dl.indexOf(searchLower) >= 0);
        }
        if (matches) {
            if (idx >= offset && added < limit) {
                arr.add(d);
                added++;
            }
            idx++;
        }
    }

    String output;
    serializeJson(arr, output);
    xSemaphoreGive(_mutex);
    return output;
}

bool BlockList::clearAll() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return false;

    _domains.clear();
    _exactDomains.clear();
    _wildcardPatterns.clear();
    _dirty = true;

    xSemaphoreGive(_mutex);

    bool ok = Storage.deleteFile(FS_BLOCKLIST_PATH);
    // Recreate empty file
    Storage.writeFile(FS_BLOCKLIST_PATH, "");
    DEBUG_PRINTLN(F("[BlockList] Cleared all domains"));
    return ok;
}

void BlockList::getAllDomains(std::vector<String>& out) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) return;
    out = _domains;
    xSemaphoreGive(_mutex);
}

int BlockList::addBulk(const String& jsonArray) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonArray);
    if (err) {
        DEBUG_PRINTLN(F("[BlockList] addBulk: invalid JSON"));
        return -1;
    }
    JsonArray arr = doc.as<JsonArray>();
    int added = 0;
    for (JsonVariant v : arr) {
        if (v.is<const char*>()) {
            if (addDomain(v.as<String>())) {
                added++;
            }
        }
    }
    DEBUG_PRINTF("[BlockList] Bulk added %d domains\n", added);
    return added;
}
