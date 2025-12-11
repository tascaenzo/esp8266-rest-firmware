#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the WiFi module in Station (STA) mode.
 *
 * This function configures the device to operate as a WiFi client
 * and resets any previous network connection state.
 *
 * @return true if the initialization completed successfully
 */
bool wifiInit();

/**
 * @brief Connects to a WiFi network using the provided credentials.
 *
 * The function attempts to establish a connection and blocks
 * until the connection is successful or a timeout is reached.
 *
 * @param ssid WiFi network name
 * @param pass WiFi network password
 * @return true if the connection is established successfully, false otherwise
 */
bool wifiConnect(const String &ssid, const String &pass);

/**
 * @brief Checks whether the device is currently connected to a WiFi network.
 *
 * @return true if connected, false otherwise
 */
bool wifiIsConnected();

/**
 * @brief Returns the local IP address as a string.
 *
 * If the device is not connected to WiFi, an empty string is returned.
 *
 * @return Local IP address in string format, or empty string if not connected
 */
String wifiGetIP();
