#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the non-volatile memory used for credential storage.
 *
 * This function prepares the EEPROM (or emulated EEPROM in flash)
 * for read and write operations. It must be called once at startup
 * before accessing any stored credentials.
 *
 * @return true if the initialization completed successfully, false otherwise
 */
bool eepromInit();

/**
 * @brief Loads the stored WiFi credentials from non-volatile memory.
 *
 * If valid credentials are found, they are copied into the provided
 * output parameters. If no valid data is present, the function fails.
 *
 * @param ssid Reference where the stored WiFi SSID will be copied
 * @param pass Reference where the stored WiFi password will be copied
 * @return true if valid credentials were successfully loaded, false otherwise
 */
bool loadWifiCredentials(String &ssid, String &pass);

/**
 * @brief Saves the WiFi credentials into non-volatile memory.
 *
 * This function permanently stores the provided SSID and password
 * so they can be restored automatically at the next startup.
 *
 * @param ssid WiFi network name to store
 * @param pass WiFi network password to store
 */
void saveWifiCredentials(const String &ssid, const String &pass);

/**
 * @brief Clears the stored WiFi credentials from non-volatile memory.
 *
 * After calling this function, no WiFi credentials will be available
 * for automatic connection at startup.
 */
void clearWifiCredentials();
