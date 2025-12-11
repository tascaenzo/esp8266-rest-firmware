#pragma once

#include <Arduino.h>

/**
 * @brief Starts the configuration captive portal.
 *
 * This function initializes and launches the web-based configuration
 * portal, allowing the user to connect to the device and configure
 * network settings and other parameters.
 *
 * @return true if the portal started successfully, false otherwise
 */
bool portalStart();

/**
 * @brief Periodic handler for the configuration portal.
 *
 * This function must be called repeatedly inside the main loop()
 * to handle client requests and keep the portal responsive.
 */
void portalLoop();

/**
 * @brief Checks whether the configuration portal is currently active.
 *
 * @return true if the portal is running, false otherwise
 */
bool portalActive();
