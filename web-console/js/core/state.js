/**
 * @brief Runtime state container for the Firmware Console.
 *
 * This module stores volatile application state that is shared
 * across views during a single browser session.
 *
 * IMPORTANT:
 * - This state is NOT persistent
 * - This module performs NO I/O
 * - This module has NO knowledge of the DOM
 */
export const RuntimeState = {
  /**
   * @brief Last full device state returned by GET /api/state.
   *
   * This object mirrors the firmware JSON response and may contain:
   *  - device info
   *  - GPIO table
   *  - cron jobs
   *
   * @type {object|null}
   */
  deviceState: null,

  /**
   * @brief Indicates whether authentication is currently enabled
   * on the device.
   *
   * Derived from the last /api/state response.
   *
   * @type {boolean}
   */
  authEnabled: false,

  /**
   * @brief Timestamp of the last successful state update.
   *
   * Used to:
   *  - avoid unnecessary refreshes
   *  - show data freshness in the UI
   *
   * @type {number|null}
   */
  lastUpdateTs: null,

  /**
   * @brief Last error reported by an API operation.
   *
   * Stored for inspection and debugging purposes.
   *
   * @type {{ message: string, source?: string } | null}
   */
  lastError: null,
};

/**
 * @brief Update the runtime state using a new /api/state payload.
 *
 * This function should be called ONLY by core/api.js
 * after a successful GET /api/state.
 *
 * @param {object} payload - JSON returned by the firmware
 */
export function updateFromDeviceState(payload) {
  RuntimeState.deviceState = payload || null;
  RuntimeState.authEnabled = !!payload?.device?.auth;
  RuntimeState.lastUpdateTs = Date.now();
  RuntimeState.lastError = null;
}

/**
 * @brief Clear the cached device state.
 *
 * This does NOT affect the device.
 * It is typically used when:
 *  - authentication is disabled
 *  - the device is rebooted
 *  - the connection target changes
 */
export function clearDeviceState() {
  RuntimeState.deviceState = null;
  RuntimeState.lastUpdateTs = null;
}

/**
 * @brief Record an error in the runtime state.
 *
 * Used by core/api.js to expose failures to the UI.
 *
 * @param {string} message - Human-readable error message
 * @param {string} [source] - Optional source identifier (e.g. endpoint name)
 */
export function setLastError(message, source) {
  RuntimeState.lastError = {
    message,
    source,
  };
}

/**
 * @brief Check whether cached device state is still considered fresh.
 *
 * This helper can be used to avoid unnecessary refreshes.
 *
 * @param {number} maxAgeMs - Maximum allowed age in milliseconds
 * @returns {boolean} true if the cached state is fresh
 */
export function isStateFresh(maxAgeMs = 2000) {
  if (!RuntimeState.lastUpdateTs) {
    return false;
  }
  return Date.now() - RuntimeState.lastUpdateTs <= maxAgeMs;
}
