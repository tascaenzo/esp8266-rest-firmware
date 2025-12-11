#include "BinaryStorage.h"
#include <LittleFS.h>

bool storageInit() { return LittleFS.begin(); }

/**
 * Write binary data to storage using LittleFS.
 * The file at 'path' will be overwritten if it exists.
 */
bool storageWrite(const char *path, const uint8_t *data, size_t length) {

  Serial.println("[STORAGE] Writing file: " + String(path));
  Serial.println("[STORAGE] Requested length: " + String(length));

  File f = LittleFS.open(path, "w");
  if (!f) {
    Serial.println("[STORAGE] ERROR: Failed to open file for writing.");
    return false;
  }

  size_t writtenBytes = f.write(data, length);
  f.close();

  Serial.println("[STORAGE] Bytes written: " + String(writtenBytes));

  if (writtenBytes != length) {
    Serial.println("[STORAGE] ERROR: Incomplete write — storage full or "
                   "filesystem error.");
    return false;
  }

  Serial.println("[STORAGE] Write completed successfully.");
  return true;
}

/**
 * Read binary data from storage using LittleFS.
 * Returns false if the file does not exist or if the size does not match.
 */
bool storageRead(const char *path, uint8_t *buffer, size_t length) {

  Serial.println("[STORAGE] Reading file: " + String(path) +
                 " into buffer of length " + String(length));

  if (!LittleFS.exists(path)) {
    Serial.println("[STORAGE] File does not exist.");
    return false;
  }

  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.println("[STORAGE] ERROR: Failed to open file.");
    return false;
  }

  size_t readBytes = f.read(buffer, length);
  f.close();

  Serial.println("[STORAGE] Requested length: " + String(length));
  Serial.println("[STORAGE] Bytes read:      " + String(readBytes));

  if (readBytes != length) {
    Serial.println("[STORAGE] ERROR: Incomplete read — corrupted or mismatched "
                   "file size.");
    return false;
  }

  Serial.println("[STORAGE] Read completed successfully.");
  return true;
}
