/**
 * @brief Client-side connection and authentication settings view.
 *
 * This view allows the user to configure:
 *  - Target device Base URL
 *  - Authentication key (if already provisioned)
 *
 * Settings are stored locally and do NOT affect the device directly.
 */

import { getBaseUrl, setBaseUrl } from "../core/config.js";
import {
  getAuthKey,
  setAuthKey,
  clearAuthKey,
  isValidAuthKey,
} from "../core/auth.js";
import { fetchDeviceState } from "../core/api.js";
import { clearDeviceState } from "../core/state.js";

let container = null;

/**
 * @brief Initialize the settings view.
 *
 * This function assumes that the HTML structure
 * is already present in the DOM.
 */
export function init() {
  container = document.getElementById("settings-panel");
  if (!container) {
    console.warn("[settings] panel container not found");
    return;
  }

  bindUI();
  loadCurrentValues();
}

/**
 * @brief Bind UI event handlers.
 */
function bindUI() {
  container.querySelector("#settings-save").addEventListener("click", onSave);

  container.querySelector("#settings-clear").addEventListener("click", onClear);

  container
    .querySelector("#settings-test")
    .addEventListener("click", onTestConnection);
}

/**
 * @brief Load stored configuration into the form.
 */
function loadCurrentValues() {
  const baseUrlInput = container.querySelector("#settings-base-url");
  const authKeyInput = container.querySelector("#settings-auth-key");

  baseUrlInput.value = getBaseUrl() || "";
  authKeyInput.value = getAuthKey() || "";
}

/**
 * @brief Handle Save button.
 *
 * Persists Base URL and Auth Key locally.
 */
function onSave() {
  const baseUrlInput = container.querySelector("#settings-base-url");
  const authKeyInput = container.querySelector("#settings-auth-key");

  try {
    setBaseUrl(baseUrlInput.value);

    const key = authKeyInput.value.trim();
    if (key.length > 0) {
      if (!isValidAuthKey(key)) {
        throw new Error("Invalid auth key format (expected 64 hex chars)");
      }
      setAuthKey(key);
    }

    clearDeviceState();
    showStatus("Settings saved", "success");
  } catch (err) {
    showStatus(err.message, "error");
  }
}

/**
 * @brief Handle Clear button.
 *
 * Clears local configuration only.
 */
function onClear() {
  setBaseUrl("");
  clearAuthKey();
  clearDeviceState();
  loadCurrentValues();
  showStatus("Local settings cleared", "success");
}

/**
 * @brief Handle Test Connection button.
 *
 * Performs GET /api/state using current settings.
 */
async function onTestConnection() {
  showStatus("Testing connection...", "info");

  try {
    await fetchDeviceState();
    showStatus("Connection successful", "success");
  } catch (err) {
    showStatus(`Connection failed: ${err.message}`, "error");
  }
}

/**
 * @brief Display a status message inside the settings panel.
 *
 * @param {string} message
 * @param {"success"|"error"|"info"} level
 */
function showStatus(message, level) {
  const el = container.querySelector("#settings-status");
  if (!el) return;

  el.textContent = message;
  el.className = `status ${level}`;
}
