/**
 * @brief Runtime configuration for the Firmware Console.
 *
 * Stores user-defined settings such as the target device base URL
 * and optional API authentication parameters.
 *
 * Configuration is persisted locally using localStorage.
 */

const STORAGE_KEY = "fwconsole.config";

/**
 * @brief Default configuration values.
 */
const DEFAULT_CONFIG = {
  baseUrl: "",
  authEnabled: false,
  apiKey: "",
};

/**
 * @brief Load configuration from localStorage.
 *
 * Always merges with DEFAULT_CONFIG to guarantee
 * backward compatibility when new fields are added.
 *
 * @returns {{ baseUrl: string, authEnabled: boolean, apiKey: string }}
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
 * @brief Check whether API authentication is enabled.
 *
 * @returns {boolean}
 */
export function isAuthEnabled() {
  return loadConfig().authEnabled;
}

/**
 * @brief Enable or disable API authentication.
 *
 * @param {boolean} enabled
 */
export function setAuthEnabled(enabled) {
  const cfg = loadConfig();
  cfg.authEnabled = Boolean(enabled);

  // Optional safety: clear API key when disabling auth
  if (!cfg.authEnabled) {
    cfg.apiKey = "";
  }

  saveConfig(cfg);
}

/**
 * @brief Get the configured API key.
 *
 * @returns {string}
 */
export function getApiKey() {
  return loadConfig().apiKey;
}

/**
 * @brief Set the API key used for authentication.
 *
 * @param {string} key
 */
export function setApiKey(key) {
  const cfg = loadConfig();
  cfg.apiKey = key.trim();
  saveConfig(cfg);
}
