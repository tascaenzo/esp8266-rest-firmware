#include "EepromConfig.h"
#include <EEPROM.h>

#define EEPROM_SIZE 128
#define FLAG_ADDR 0 // 0xA5 = valid data flag
#define SSID_ADDR 1
#define PASS_ADDR 33
#define MAX_LEN 31 // 31 + null terminator = 32 bytes per string

/**
 * Initializes the EEPROM memory for credential storage.
 */
bool eepromInit() {
  EEPROM.begin(EEPROM_SIZE);
  return true;
}

/**
 * Loads the WiFi credentials from EEPROM if the valid flag is present.
 */
bool loadWifiCredentials(String &ssid, String &pass) {
  // Check if valid credentials flag is present
  if (EEPROM.read(FLAG_ADDR) != 0xA5) {
    Serial.println("EEPROM: no valid credentials found.");
    return false;
  }

  char buf[MAX_LEN + 1];

  // Read SSID
  for (int i = 0; i < MAX_LEN; i++)
    buf[i] = EEPROM.read(SSID_ADDR + i);
  buf[MAX_LEN] = '\0';
  ssid = buf;

  // Read password
  for (int i = 0; i < MAX_LEN; i++)
    buf[i] = EEPROM.read(PASS_ADDR + i);
  buf[MAX_LEN] = '\0';
  pass = buf;

  Serial.println("EEPROM: WiFi credentials loaded.");
  Serial.print("SSID: ");
  Serial.println(ssid);

  return ssid.length() > 0;
}

/**
 * Saves the WiFi credentials into EEPROM and sets the valid flag.
 */
void saveWifiCredentials(const String &ssid, const String &pass) {
  Serial.println("EEPROM: saving WiFi credentials...");

  // Write SSID
  for (int i = 0; i < MAX_LEN; i++)
    EEPROM.write(SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);

  // Write password
  for (int i = 0; i < MAX_LEN; i++)
    EEPROM.write(PASS_ADDR + i, i < pass.length() ? pass[i] : 0);

  // Set valid credentials flag
  EEPROM.write(FLAG_ADDR, 0xA5);
  EEPROM.commit();

  Serial.println("EEPROM: credentials saved successfully.");
}

/**
 * Clears all stored WiFi credentials from EEPROM.
 */
void clearWifiCredentials() {
  Serial.println("EEPROM: clearing stored credentials...");

  for (int i = 0; i < EEPROM_SIZE; i++)
    EEPROM.write(i, 0);

  EEPROM.commit();

  Serial.println("EEPROM: memory cleared.");
}
