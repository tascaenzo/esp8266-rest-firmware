#pragma once

#include <Arduino.h>

/**
 * @brief Initialize filesystem.
 * @return true if successful.
 */
bool storageInit();

/**
 * @brief Write raw bytes to a file.
 *
 * @param path File path (ex: "/config.bin")
 * @param data Pointer to bytes to store
 * @param length Number of bytes to write
 *
 * @return true if written successfully
 */
bool storageWrite(const char *path, const uint8_t *data, size_t length);

/**
 * @brief Read raw bytes from a file.
 *
 * @param path File path
 * @param buffer Destination buffer (must be preallocated)
 * @param length Number of bytes to read
 *
 * @return true if read successfully and size matches
 */
bool storageRead(const char *path, uint8_t *buffer, size_t length);
