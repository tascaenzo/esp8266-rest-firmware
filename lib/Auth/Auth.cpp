#include "Auth.h"

#include <EepromConfig.h>
#include <bearssl/bearssl.h>
#include <string.h>

#define AUTH_KEY_LEN 32

/* Internal state */
static uint8_t authKey[32];
static bool authEnabled = false;

static const uint32_t NONCE_TIMEOUT_MS = 30000;
static AuthSlot authSlots[MAX_AUTH_SLOTS];

static void clearSlot(AuthSlot &slot) {
  slot.ip = IPAddress();
  slot.nonce = 0;
  slot.timestamp = 0;
  slot.active = false;
}

static int findSlotByIp(const IPAddress &ip) {
  for (int i = 0; i < MAX_AUTH_SLOTS; i++) {
    if (authSlots[i].active && authSlots[i].ip == ip)
      return i;
  }
  return -1;
}

static int findFreeSlot() {
  for (int i = 0; i < MAX_AUTH_SLOTS; i++) {
    if (!authSlots[i].active)
      return i;
  }
  return -1;
}

static int findOldestSlot() {
  int idx = 0;
  uint32_t oldest = authSlots[0].timestamp;

  for (int i = 1; i < MAX_AUTH_SLOTS; i++) {
    if (authSlots[i].timestamp < oldest) {
      oldest = authSlots[i].timestamp;
      idx = i;
    }
  }
  return idx;
}

bool authInit() {

  // Clear all authentication slots
  for (int i = 0; i < MAX_AUTH_SLOTS; i++)
    clearSlot(authSlots[i]);

  loadAuthFlag(&authEnabled);

  // Try loading authentication key from EEPROM
  if (authEnabled && loadAuthKey(authKey, AUTH_KEY_LEN)) {
    Serial.println("[AUTH] Authentication enabled (key loaded from EEPROM)");
  } else {
    authEnabled = false;
    Serial.println("[AUTH] Authentication disabled (no key in EEPROM)");
  }

  return true;
}

static void hmacSha256(const uint8_t *key, size_t keyLen, const uint8_t *data,
                       size_t dataLen, uint8_t *out) {
  br_hmac_key_context kc;
  br_hmac_context ctx;

  br_hmac_key_init(&kc, &br_sha256_vtable, key, keyLen);
  br_hmac_init(&ctx, &kc, 0);
  br_hmac_update(&ctx, data, dataLen);
  br_hmac_out(&ctx, out);
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

static bool hexToBytes(const char *hex, uint8_t *out, size_t outLen) {
  for (size_t i = 0; i < outLen; i++) {
    uint8_t hi = hexToNibble(hex[i * 2]);
    uint8_t lo = hexToNibble(hex[i * 2 + 1]);
    if (hi == 0xFF || lo == 0xFF)
      return false;
    out[i] = (hi << 4) | lo;
  }
  return true;
}

uint32_t authGenerateChallenge(const IPAddress &clientIp) {

  int idx = findSlotByIp(clientIp);

  if (idx < 0) {
    idx = findFreeSlot();
    if (idx < 0)
      idx = findOldestSlot();
  }

  authSlots[idx].ip = clientIp;
  authSlots[idx].nonce = os_random();
  authSlots[idx].timestamp = millis();
  authSlots[idx].active = true;

  return authSlots[idx].nonce;
}

bool authVerify(const IPAddress &clientIp, uint32_t nonce, const char *uri,
                const char *payload, const char *signature) {
  if (!uri || !payload || !signature)
    return false;

  int idx = findSlotByIp(clientIp);
  if (idx < 0)
    return false;

  AuthSlot &slot = authSlots[idx];

  if (!slot.active || slot.nonce != nonce)
    return false;

  if (millis() - slot.timestamp > NONCE_TIMEOUT_MS) {
    clearSlot(slot);
    return false;
  }

  if (strlen(signature) != 64) {
    clearSlot(slot);
    return false;
  }

  char nonceBuf[11];
  snprintf(nonceBuf, sizeof(nonceBuf), "%lu", (unsigned long)nonce);

  size_t dataLen = strlen(nonceBuf) + strlen(uri) + strlen(payload);

  if (dataLen > 1024) {
    clearSlot(slot);
    return false;
  }

  char data[1024];
  strcpy(data, nonceBuf);
  strcat(data, uri);
  strcat(data, payload);

  uint8_t expectedMac[32];
  hmacSha256(authKey, AUTH_KEY_LEN, (const uint8_t *)data, strlen(data),
             expectedMac);

  uint8_t clientMac[32];
  if (!hexToBytes(signature, clientMac, sizeof(clientMac))) {
    clearSlot(slot);
    return false;
  }

  clearSlot(slot); // nonce monouso (anti-replay)

  uint8_t diff = 0;
  for (size_t i = 0; i < 32; i++)
    diff |= expectedMac[i] ^ clientMac[i];

  return diff == 0;
}

bool getAuthEnabled() { return authEnabled; }
