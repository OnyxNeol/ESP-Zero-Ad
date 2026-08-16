#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <LittleFS.h>
#include <functional>

// ============================================================================
// Storage Manager — LittleFS wrapper for persistent file storage
// ============================================================================

class StorageManager {
public:
    // Initialize LittleFS. Returns true on success.
    bool begin();

    // Read entire file contents into a String. Returns empty String on error.
    String readFile(const String& path);

    // Read file line-by-line; calls callback for each line.
    // Returns number of lines read.
    int readFileLines(const String& path, std::function<void(const String&)> callback);

    // Write content to file (overwrites). Returns true on success.
    bool writeFile(const String& path, const String& content);

    // Append a single line to file (adds newline). Returns true on success.
    bool appendFile(const String& path, const String& line);

    // Check if file exists.
    bool fileExists(const String& path);

    // Delete a file. Returns true on success.
    bool deleteFile(const String& path);

    // Get file size in bytes. Returns 0 if not found.
    size_t fileSize(const String& path);

    // List files in a directory path.
    void listDir(const String& path);

private:
    bool _initialized = false;
};

// Global instance
extern StorageManager Storage;

#endif // STORAGE_H
