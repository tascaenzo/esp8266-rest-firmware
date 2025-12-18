/**
 * @brief Runtime configuration for the Firmware Console.
 *
 * Stores user-defined settings such as the target device base URL.
 *
 * Configuration is persisted locally using localStorage.
 */

const STORAGE_KEY = "fwconsole.config";

/**
 * @brief Default configuration values.
 */
const DEFAULT_CONFIG = {
  baseUrl: "",
  authExpected: false,
};

/**
 * @brief Load configuration from localStorage.
 *
 * @returns {{ baseUrl: string }}
 */
function loadConfig() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw
      ? { ...DEFAULT_CONFIG, ...JSON.parse(raw) }
      : { ...DEFAULT_CONFIG };
  } catch {
    return { ...DEFAULT_CONFIG };
  }
}

/**
 * @brief Persist configuration to localStorage.
 *
 * @param {object} cfg
 */
function saveConfig(cfg) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(cfg));
}

/**
 * @brief Get the configured base URL.
 *
 * Empty string means "same origin".
 *
 * @returns {string}
 */
export function getBaseUrl() {
  return loadConfig().baseUrl;
}

/**
 * @brief Set the target device base URL.
 *
 * @param {string} url
 */
export function setBaseUrl(url) {
  const cfg = loadConfig();
  cfg.baseUrl = url.trim().replace(/\/$/, ""); // strip trailing slash
  saveConfig(cfg);
}

/**
 * @brief Get whether the device is expected to have authentication enabled.
 *
 * This informs the console whether it should sign requests before
 * a fresh /api/state response is available.
 *
 * @returns {boolean}
 */
export function getAuthExpected() {
  return !!loadConfig().authExpected;
}

/**
 * @brief Persist the expected authentication state.
 *
 * @param {boolean} enabled
 */
export function setAuthExpected(enabled) {
  const cfg = loadConfig();
  cfg.authExpected = !!enabled;
  saveConfig(cfg);
}
