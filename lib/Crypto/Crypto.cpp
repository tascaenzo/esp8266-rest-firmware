#include "Crypto.h"

#include <bearssl/bearssl.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* HMAC-SHA256                                                                */
/* -------------------------------------------------------------------------- */

void hmacSha256(const uint8_t *key, size_t keyLen, const uint8_t *data,
                size_t dataLen, uint8_t *out) {

  br_hmac_key_context kc;
  br_hmac_context ctx;

  br_hmac_key_init(&kc, &br_sha256_vtable, key, keyLen);
  br_hmac_init(&ctx, &kc, 0);
  br_hmac_update(&ctx, data, dataLen);
  br_hmac_out(&ctx, out);
}

/* -------------------------------------------------------------------------- */
/* Random                                                                     */
/* -------------------------------------------------------------------------- */

void randomBytes(uint8_t *out, size_t len) {

  if (!out)
    return;

  for (size_t i = 0; i < len; i += 4) {
    uint32_t r = os_random();

    size_t remaining = len - i;
    if (remaining > 0)
      out[i + 0] = (r >> 24) & 0xFF;
    if (remaining > 1)
      out[i + 1] = (r >> 16) & 0xFF;
    if (remaining > 2)
      out[i + 2] = (r >> 8) & 0xFF;
    if (remaining > 3)
      out[i + 3] = r & 0xFF;
  }
}

/* -------------------------------------------------------------------------- */
/* Hex encoding                                                               */
/* -------------------------------------------------------------------------- */

void bytesToHex(const uint8_t *in, size_t len, char *out) {

  static const char hexMap[] = "0123456789abcdef";

  for (size_t i = 0; i < len; i++) {
    out[i * 2] = hexMap[(in[i] >> 4) & 0x0F];
    out[i * 2 + 1] = hexMap[in[i] & 0x0F];
  }

  out[len * 2] = '\0';
}

static uint8_t hexToNibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return 0xFF;
}

bool hexToBytes(const char *hex, uint8_t *out, size_t outLen) {

  if (!hex || !out)
    return false;

  for (size_t i = 0; i < outLen; i++) {
    uint8_t hi = hexToNibble(hex[i * 2]);
    uint8_t lo = hexToNibble(hex[i * 2 + 1]);

    if (hi == 0xFF || lo == 0xFF)
      return false;

    out[i] = (hi << 4) | lo;
  }

  return true;
}

/* -------------------------------------------------------------------------- */
/* Constant-time compare                                                      */
/* -------------------------------------------------------------------------- */

bool secureCompare(const uint8_t *a, const uint8_t *b, size_t len) {

  uint8_t diff = 0;

  for (size_t i = 0; i < len; i++)
    diff |= a[i] ^ b[i];

  return diff == 0;
}
