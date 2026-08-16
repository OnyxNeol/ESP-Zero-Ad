#include "storage.h"
#include "config.h"

StorageManager Storage;

bool StorageManager::begin() {
    DEBUG_PRINTLN(F("[Storage] Mounting LittleFS..."));
    if (!LittleFS.begin(true)) {  // true = format on failure
        DEBUG_PRINTLN(F("[Storage] Failed to mount LittleFS!"));
        _initialized = false;
        return false;
    }
    _initialized = true;
    DEBUG_PRINTLN(F("[Storage] LittleFS mounted successfully."));

    // Print usage info
    FSInfo fs_info;
    if (LittleFS.info(fs_info)) {
        DEBUG_PRINTF("[Storage] Total: %u bytes, Used: %u bytes\n",
                     fs_info.totalBytes, fs_info.usedBytes);
    }
    return true;
}

String StorageManager::readFile(const String& path) {
    if (!_initialized) return "";
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
        DEBUG_PRINTF("[Storage] readFile: cannot open %s\n", path.c_str());
        return "";
    }
    String content = file.readString();
    file.close();
    return content;
}

int StorageManager::readFileLines(const String& path, std::function<void(const String&)> callback) {
    if (!_initialized) return 0;
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
        DEBUG_PRINTF("[Storage] readFileLines: cannot open %s\n", path.c_str());
        return 0;
    }
    int count = 0;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            callback(line);
            count++;
        }
    }
    file.close();
    return count;
}

bool StorageManager::writeFile(const String& path, const String& content) {
    if (!_initialized) return false;
    File file = LittleFS.open(path, "w");
    if (!file) {
        DEBUG_PRINTF("[Storage] writeFile: cannot open %s\n", path.c_str());
        return false;
    }
    size_t written = file.print(content);
    file.close();
    if (written != content.length()) {
        DEBUG_PRINTLN(F("[Storage] writeFile: partial write"));
        return false;
    }
    return true;
}

bool StorageManager::appendFile(const String& path, const String& line) {
    if (!_initialized) return false;
    File file = LittleFS.open(path, "a");
    if (!file) {
        DEBUG_PRINTF("[Storage] appendFile: cannot open %s\n", path.c_str());
        return false;
    }
    size_t written = file.print(line);
    file.print("\n");
    file.close();
    return (written == line.length());
}

bool StorageManager::fileExists(const String& path) {
    if (!_initialized) return false;
    return LittleFS.exists(path);
}

bool StorageManager::deleteFile(const String& path) {
    if (!_initialized) return false;
    return LittleFS.remove(path);
}

size_t StorageManager::fileSize(const String& path) {
    if (!_initialized) return 0;
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) return 0;
    size_t sz = file.size();
    file.close();
    return sz;
}

void StorageManager::listDir(const String& path) {
    if (!_initialized) return;
    File root = LittleFS.open(path, "r");
    if (!root || !root.isDirectory()) {
        DEBUG_PRINTF("[Storage] listDir: %s is not a directory\n", path.c_str());
        return;
    }
    File f = root.openNextFile();
    while (f) {
        String type = f.isDirectory() ? "DIR " : "FILE";
        DEBUG_PRINTF("[Storage]   %s %s (%u bytes)\n", type.c_str(), f.name(), f.size());
        f = root.openNextFile();
    }
}
