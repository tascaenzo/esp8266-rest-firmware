#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the EEPROM emulation layer.
 *
 * Must be called once at startup before accessing
 * any persistent configuration.
 */
bool eepromInit();

/* -------------------------------------------------------------------------- */
/* WiFi credentials                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Loads stored WiFi credentials.
 *
 * @param ssid Output SSID
 * @param pass Output password
 * @return true if a non-empty SSID was found
 */
bool loadWifiCredentials(String &ssid, String &pass);

/**
 * @brief Saves WiFi credentials to EEPROM.
 *
 * @param ssid WiFi network name
 * @param pass WiFi password
 */
void setWifiCredentials(const String &ssid, const String &pass);

/**
 * @brief Clears stored WiFi credentials.
 */
void clearWifiCredentials();

/* -------------------------------------------------------------------------- */
/* Authentication key                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Enables or disables authentication api.
 *
 * @param flag true to enable, false to disable
 */
bool loadAuthFlag(bool *flag);

/**
 * @brief Enables or disables authentication.
 *
 * @param flag true to enable, false to disable
 */
bool setAuthFleg(bool flag);

/**
 * @brief Loads the authentication shared secret.
 *
 * @param key Output buffer (binary)
 * @param length Buffer size (must be 32 bytes)
 * @return true if a valid key was loaded
 */
bool loadAuthKey(uint8_t *key, size_t length);

/**
 * @brief Saves a new authentication shared secret.
 *
 * @param key Binary key
 * @param length Key length (must be 32 bytes)
 */
void setAuthKey(const uint8_t *key, size_t length);

/**
 * @brief Clears the stored authentication key.
 */
void clearAuthKey();

/* -------------------------------------------------------------------------- */
/* Serial debug                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Checks if serial debug output is enabled.
 */
bool loadDebugFlag(bool *flag);

/**
 * @brief Enables or disables serial debug output.
 *
 * @param flag true to enable, false to disable
 */
void setSerialDebugFlag(bool flag);
