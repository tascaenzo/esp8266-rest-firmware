#pragma once

/**
 * @brief Handles authentication challenge request.
 *
 * Endpoint: GET /api/auth/challenge
 *
 * Generates and returns a one-time nonce bound to the client IP.
 * The nonce is used by the client to compute the HMAC signature
 * for subsequent authenticated API calls.
 *
 * This endpoint is available only when authentication is enabled.
 */
void handleAuthChallenge();

/**
 * @brief Handles device setup and system configuration.
 *
 * This endpoint is used to configure system-level settings such as:
 * - enabling or disabling API authentication
 * - enabling or disabling serial debug output
 *
 * Behavior:
 * - When authentication is enabled and no key is present, a new random
 *   shared secret is generated, stored in EEPROM, and returned once
 *   in the response.
 * - When authentication is disabled, any stored authentication key
 *   is cleared from persistent storage.
 * - Serial debug settings are stored persistently and take effect
 *   immediately or after reboot, depending on the subsystem.
 *
 * Access control:
 * - If authentication is currently disabled, this endpoint is public.
 * - If authentication is enabled, this endpoint requires authentication.
 *
 * Request (JSON):
 * {
 *   "auth": true | false,
 *   "serialDebug": true | false
 * }
 *
 * Response (JSON):
 * {
 *   "auth": true | false,
 *   "serialDebug": true | false,
 *   "authKey": "hex-string"   // only returned when auth is newly enabled
 * }
 */
void handleSetup();

/**
 * @brief Returns the full device state.
 *
 * Endpoint: GET /api/state
 *
 * Returns a JSON document containing:
 * - Device information (IP, chip ID, RSSI, uptime, settings)
 * - All configured GPIO pins with state and capabilities
 * - All configured cron jobs
 *
 * Requires authentication if enabled.
 */
void handleGetState();

/**
 * @brief Returns the configuration of a single GPIO pin.
 *
 * Endpoint: GET /api/pin?id=GPIOx
 *
 * The response includes the current mode and state of the requested pin.
 * Analog pin A0 is handled separately.
 *
 * Requires authentication if enabled.
 */
void handleGetPin();

/**
 * @brief Replaces the entire GPIO configuration table.
 *
 * Endpoint: POST /api/config
 *
 * Accepts a JSON body defining the desired configuration for
 * all GPIO pins. Pins not included in the request are disabled.
 *
 * The request is fully validated before applying any changes.
 *
 * Requires authentication if enabled.
 */
void handleConfig();

/**
 * @brief Updates the mode and/or state of a single GPIO pin.
 *
 * Endpoint: PATCH /api/pin/set
 *
 * Accepts a JSON body containing:
 * - Pin identifier
 * - Optional new mode
 * - Optional new state or PWM value
 *
 * The pin configuration is validated and applied atomically.
 *
 * Requires authentication if enabled.
 */
void handlePatchPin();

/**
 * @brief Reboots the device.
 *
 * Endpoint: POST /api/reboot
 *
 * Sends a confirmation response and then performs a soft reboot
 * of the ESP8266.
 *
 * Requires authentication if enabled.
 */
void handleReboot();

/**
 * @brief Returns a specific cron job configuration.
 *
 * Endpoint: GET /api/cron?id=N
 *
 * Returns the cron expression, action, target pin and value
 * for the specified cron job slot.
 *
 * Requires authentication if enabled.
 */
void handleGetCron();

/**
 * @brief Creates or updates a cron job.
 *
 * Endpoint: PATCH /api/cron/set
 *
 * Accepts a JSON body describing:
 * - Cron expression
 * - Action (set, toggle, reboot)
 * - Optional pin and value
 *
 * The job is stored persistently and activated immediately.
 *
 * Requires authentication if enabled.
 */
void handleCronSet();

/**
 * @brief Disables a cron job.
 *
 * Endpoint: DELETE /api/cron?id=N
 *
 * Marks the specified cron job slot as inactive.
 * The job remains stored but will no longer execute.
 *
 * Requires authentication if enabled.
 */
void handleDeleteCron();

/**
 * @brief Clears all cron jobs.
 *
 * Endpoint: DELETE /api/cron/clear
 *
 * Disables all cron job slots and removes their configuration.
 *
 * Requires authentication if enabled.
 */
void handleClearCron();
