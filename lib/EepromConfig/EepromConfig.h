#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the EEPROM emulation layer.
 *
 * Must be called once at startup before accessing
 * any persistent configuration.
 */
bool eepromInit();

/**
 * @brief Resets the entire EEPROM configuration area.
 *
 * This function performs a full reset of the persistent EEPROM storage.
 *
 * Behavior:
 * - Clears all stored data (WiFi credentials, auth key, flags)
 * - Resets the magic value so the device is treated as "factory new"
 * - Disables authentication
 * - Disables serial debug
 *
 * After calling this function, the device will:
 * - Start in provisioning mode on next boot
 * - Require full reconfiguration
 *
 * @return true if the reset operation completed successfully
 */
bool resetEeprom();

/**
 * @brief Checks for a hardware-triggered factory reset at boot.
 *
 * This function implements a recovery mechanism based on a physical GPIO.
 * If a predefined reset pin is held in its active state (e.g. LOW when using
 * INPUT_PULLUP) for a fixed amount of time during startup, the device will:
 *
 *   - Clear all persistent configuration stored in EEPROM
 *   - Reset authentication keys and flags
 *   - Disable serial debug
 *   - Reboot into provisioning mode
 *
 * The reset pin is sampled immediately after EEPROM initialization and
 * before WiFi, authentication, or API services are started.
 *
 * This mechanism allows device recovery even when:
 *   - Authentication is enabled but the key is lost
 *   - WiFi credentials are invalid
 *   - Remote access is no longer possible
 *
 * IMPORTANT:
 * - The reset GPIO must NOT be a boot-sensitive pin (avoid GPIO0, GPIO2,
 * GPIO15)
 * - The pin logic (active HIGH or LOW) and hold time must be defined at compile
 * time
 * - The function is intended to be called once during startup (setup())
 */
void checkHardwareReset();

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
void setAuthFlag(bool flag);

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
