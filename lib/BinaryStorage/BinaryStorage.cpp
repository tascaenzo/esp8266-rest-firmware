#include "BinaryStorage.h"
#include <LittleFS.h>

#include "Debug.h"

bool storageInit() { return LittleFS.begin(); }

/**
 * Write binary data to storage using LittleFS.
 * The file at 'path' will be overwritten if it exists.
 */
bool storageWrite(const char *path, const uint8_t *data, size_t length) {

  debugPrintln("[STORAGE] Writing file: " + String(path));
  debugPrintln("[STORAGE] Requested length: " + String(length));

  File f = LittleFS.open(path, "w");
  if (!f) {
    debugPrintln(F("[STORAGE] ERROR: Failed to open file for writing."));
    return false;
  }

  size_t writtenBytes = f.write(data, length);
  f.close();

  debugPrintln("[STORAGE] Bytes written: " + String(writtenBytes));

  if (writtenBytes != length) {
    debugPrintln(F("[STORAGE] ERROR: Incomplete write — storage full or "
                   "filesystem error."));
    return false;
  }

  debugPrintln(F("[STORAGE] Write completed successfully."));
  return true;
}

/**
 * Read binary data from storage using LittleFS.
 * Returns false if the file does not exist or if the size does not match.
 */
bool storageRead(const char *path, uint8_t *buffer, size_t length) {

  debugPrintln("[STORAGE] Reading file: " + String(path) +
               " into buffer of length " + String(length));

  if (!LittleFS.exists(path)) {
    debugPrintln(F("[STORAGE] File does not exist."));
    return false;
  }

  File f = LittleFS.open(path, "r");
  if (!f) {
    debugPrintln(F("[STORAGE] ERROR: Failed to open file."));
    return false;
  }

  size_t readBytes = f.read(buffer, length);
  f.close();

  debugPrintln("[STORAGE] Requested length: " + String(length));
  debugPrintln("[STORAGE] Bytes read:      " + String(readBytes));

  if (readBytes != length) {
    debugPrintln(F("[STORAGE] ERROR: Incomplete read — corrupted or "
                   "mismatched file size."));
    return false;
  }

  debugPrintln(F("[STORAGE] Read completed successfully."));
  return true;
}
