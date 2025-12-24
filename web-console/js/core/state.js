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
