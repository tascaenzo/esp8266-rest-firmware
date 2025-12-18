#pragma once

#include <Arduino.h>
#include <stddef.h>

/**
 * @brief Computes HMAC-SHA256.
 *
 * @param key     Secret key (binary)
 * @param keyLen  Key length in bytes
 * @param data    Input data
 * @param dataLen Input data length
 * @param out     Output buffer (32 bytes)
 */
void hmacSha256(const uint8_t *key, size_t keyLen, const uint8_t *data,
                size_t dataLen, uint8_t *out);

/**
 * @brief Generates cryptographically strong random bytes.
 *
 * Uses ESP8266 hardware RNG.
 *
 * @param out Output buffer
 * @param len Number of bytes to generate
 */
void randomBytes(uint8_t *out, size_t len);

/**
 * @brief Converts hexadecimal string to binary data.
 *
 * @param hex    Input hex string
 * @param out    Output buffer
 * @param outLen Expected output length
 * @return true if conversion succeeded
 */
bool hexToBytes(const char *hex, uint8_t *out, size_t outLen);

/**
 * @brief Constant-time buffer comparison.
 *
 * Prevents timing attacks.
 *
 * @param a First buffer
 * @param b Second buffer
 * @param len Number of bytes
 * @return true if buffers are equal
 */
bool secureCompare(const uint8_t *a, const uint8_t *b, size_t len);

/**
 * @brief Converts a binary buffer to a hexadecimal string.
 *
 * This utility function encodes raw binary data into a
 * null-terminated lowercase hexadecimal string.
 *
 * Each input byte is converted to two hexadecimal characters.
 * The output buffer must be large enough to hold:
 *
 *   (len * 2) + 1 bytes
 *
 * for the terminating null character.
 *
 * @param in   Pointer to the input binary buffer
 * @param len  Number of bytes in the input buffer
 * Output uses lowercase hexadecimal characters (0-9, a-f).
 */
void bytesToHex(const uint8_t *in, size_t len, char *out);
