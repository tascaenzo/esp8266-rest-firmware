#include "EepromConfig.h"
#include <EEPROM.h>

/*
 * EEPROM memory layout (total size: 128 bytes)
 *
 * Address range | Size | Purpose
 * --------------|------|-------------------------------------------
 * 0             | 1    | MAGIC byte (0x42 = initialized EEPROM)
 * 1             | 1    | Authentication enabled flag (0xA5 = enabled)
 * 2  – 33       | 32   | Authentication shared secret (binary)
 * 34 – 39       | 6    | Reserved (future use / alignment)
 * 40 – 70       | 31   | WiFi SSID
 * 71            | 1    | Reserved
 * 72 – 102      | 31   | WiFi password
 * 103           | 1    | Serial debug flag (0xA5 = enabled)
 * 104 – 127     | 24   | Free / reserved
 *
 * Notes:
 * - EEPROM is emulated in flash on ESP8266.
 * - MAGIC byte is used to detect first boot / uninitialized memory.
 * - All flags use value 0xA5 to indicate enabled/valid state.
 * - Authentication keys are stored in binary form.
 * - This module contains NO business logic, only storage access.
 */

#define EEPROM_SIZE 128

/* Magic */
#define MAGIC_ADDR 0
#define MAGIC_VALUE 0x42

/* Authentication */
#define AUTH_FLAG_ADDR 1
#define AUTH_KEY_ADDR 2
#define AUTH_KEY_LEN 32

/* WiFi */
#define SSID_ADDR 40
#define PASS_ADDR 72
#define MAX_WIFI_LEN 31

/* Serial debug */
#define DEBUG_FLAG_ADDR 103

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

bool eepromInit() {
  EEPROM.begin(EEPROM_SIZE);

  uint8_t magic = EEPROM.read(MAGIC_ADDR);

  if (magic != MAGIC_VALUE) {
    // First boot or corrupted EEPROM
    Serial.println("[EEPROM] Magic not found, initializing EEPROM");

    // Clear entire EEPROM
    for (int i = 0; i < EEPROM_SIZE; i++)
      EEPROM.write(i, 0x00);

    // Explicit defaults
    EEPROM.write(AUTH_FLAG_ADDR, 0x00);  // auth disabled
    EEPROM.write(DEBUG_FLAG_ADDR, 0x00); // debug disabled

    // Write magic
    EEPROM.write(MAGIC_ADDR, MAGIC_VALUE);

    EEPROM.commit();
    Serial.println("[EEPROM] EEPROM initialized with default values");
  } else {
    Serial.println("[EEPROM] Magic found, EEPROM already initialized");
  }

  return true;
}

/* -------------------------------------------------------------------------- */
/* WiFi credentials                                                           */
/* -------------------------------------------------------------------------- */

bool loadWifiCredentials(String &ssid, String &pass) {
  char buf[MAX_WIFI_LEN];

  // SSID
  for (int i = 0; i < MAX_WIFI_LEN - 1; i++)
    buf[i] = EEPROM.read(SSID_ADDR + i);
  buf[MAX_WIFI_LEN - 1] = '\0';
  ssid = buf;

  // Password
  for (int i = 0; i < MAX_WIFI_LEN - 1; i++)
    buf[i] = EEPROM.read(PASS_ADDR + i);
  buf[MAX_WIFI_LEN - 1] = '\0';
  pass = buf;

  return ssid.length() > 0;
}

void setWifiCredentials(const String &ssid, const String &pass) {
  for (uint i = 0; i < MAX_WIFI_LEN - 1; i++)
    EEPROM.write(SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);

  for (uint i = 0; i < MAX_WIFI_LEN - 1; i++)
    EEPROM.write(PASS_ADDR + i, i < pass.length() ? pass[i] : 0);

  EEPROM.commit();
}

void clearWifiCredentials() {
  for (int i = 0; i < MAX_WIFI_LEN - 1; i++) {
    EEPROM.write(SSID_ADDR + i, 0);
    EEPROM.write(PASS_ADDR + i, 0);
  }

  EEPROM.commit();
}

/* -------------------------------------------------------------------------- */
/* Authentication                                                             */
/* -------------------------------------------------------------------------- */

bool loadAuthFlag(bool *flag) {
  if (!flag)
    return false;

  *flag = (EEPROM.read(AUTH_FLAG_ADDR) == 0xA5);
  return true;
}

bool loadAuthKey(uint8_t *key, size_t length) {
  if (!key || length != AUTH_KEY_LEN)
    return false;

  if (EEPROM.read(AUTH_FLAG_ADDR) != 0xA5)
    return false;

  for (size_t i = 0; i < AUTH_KEY_LEN; i++)
    key[i] = EEPROM.read(AUTH_KEY_ADDR + i);

  return true;
}

bool setAuthFlag(bool flag) {
  EEPROM.write(AUTH_FLAG_ADDR, flag ? 0xA5 : 0x00);
  EEPROM.commit();
  return true;
}

void setAuthKey(const uint8_t *key, size_t length) {
  if (!key || length != AUTH_KEY_LEN)
    return;

  EEPROM.write(AUTH_FLAG_ADDR, 0xA5);

  for (size_t i = 0; i < AUTH_KEY_LEN; i++)
    EEPROM.write(AUTH_KEY_ADDR + i, key[i]);

  EEPROM.commit();
}

void clearAuthKey() {
  EEPROM.write(AUTH_FLAG_ADDR, 0x00);

  for (size_t i = 0; i < AUTH_KEY_LEN; i++)
    EEPROM.write(AUTH_KEY_ADDR + i, 0);

  EEPROM.commit();
}

/* -------------------------------------------------------------------------- */
/* Serial debug                                                               */
/* -------------------------------------------------------------------------- */

bool loadDebugFlag(bool *flag) {
  if (!flag)
    return false;

  *flag = (EEPROM.read(DEBUG_FLAG_ADDR) == 0xA5);
  return true;
}

void setSerialDebugFlag(bool flag) {
  EEPROM.write(DEBUG_FLAG_ADDR, flag ? 0xA5 : 0x00);
  EEPROM.commit();
}
