#include "Auth.h"

#include <Crypto.h>
#include <Debug.h>
#include <EepromConfig.h>
#include <string.h>

#define AUTH_KEY_LEN 32

/* Internal state */
static uint8_t authKey[AUTH_KEY_LEN];
static bool authEnabled = false;

static const uint32_t NONCE_TIMEOUT_MS = 50000;
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

    debugPrintln(F("[AUTH]"),
                 F("Authentication enabled (key loaded from EEPROM)"));

    // Convert key to hex string for debug
    char keyHex[AUTH_KEY_LEN * 2 + 1];
    bytesToHex(authKey, AUTH_KEY_LEN, keyHex);

    debugPrintln(F("[AUTH]"), String("key: ") + keyHex);

  } else {
    authEnabled = false;
    debugPrintln(F("[AUTH]"), F("Authentication disabled (no key in EEPROM)"));
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

  uint8_t expectedMac[AUTH_KEY_LEN];
  hmacSha256(authKey, AUTH_KEY_LEN, (const uint8_t *)data, strlen(data),
             expectedMac);

  uint8_t clientMac[AUTH_KEY_LEN];
  if (!hexToBytes(signature, clientMac, sizeof(clientMac))) {
    clearSlot(slot);
    return false;
  }

  clearSlot(slot); // nonce (anti-replay)

  return secureCompare(expectedMac, clientMac, AUTH_KEY_LEN);
}

bool getAuthEnabled() { return authEnabled; }

bool generateAuthKey(uint8_t *out) {

  if (!out)
    return false;

  randomBytes(out, AUTH_KEY_LEN);
  setAuthKey(out, AUTH_KEY_LEN);

  debugPrintln(F("[AUTH]"), F("New authentication key generated and stored"));
  return true;
}

void enableAuth() {
  setAuthFlag(true);
  authEnabled = true;
}

void disableAuth() {
  setAuthFlag(false);
  authEnabled = false;
}
