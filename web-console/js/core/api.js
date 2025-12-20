/**
 * @brief REST API bridge between the Firmware Console and the device.
 *
 * This module is the ONLY layer allowed to:
 *  - perform HTTP requests
 *  - handle nonce-based authentication
 *  - update the shared runtime state
 *
 * Views must never call fetch() directly.
 */

import { computeSignature, hasAuthKey } from "./auth.js";
import { getBaseUrl } from "./config.js";
import {
  RuntimeState,
  updateFromDeviceState,
  setLastError,
  clearDeviceState,
  recordResponse,
} from "./state.js";

/**
 * @brief Base URL of the target device.
 *
 * Empty string means "same origin" (recommended).
 */

/* -------------------------------------------------------------------------- */
/*                               INTERNAL HELPERS                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Request a nonce from the device.
 *
 * GET /api/auth/challenge
 *
 * This endpoint is ALWAYS public, regardless of auth state.
 *
 * @returns {Promise<string>} Nonce value
 */
async function requestNonce() {
  const res = await fetch(`${getBaseUrl()}/api/auth/challenge`);
  if (!res.ok) {
    throw new Error(`Nonce request failed (${res.status})`);
  }

  const data = await res.json();
  recordResponse("GET /api/auth/challenge", data);
  return String(data.nonce);
}

/**
 * @brief Public wrapper to fetch a fresh nonce.
 *
 * Exposed for documentation and live authentication previews.
 *
 * @returns {Promise<string>} Nonce value
 */
export async function fetchNonce() {
  try {
    return await requestNonce();
  } catch (err) {
    setLastError(err.message, "GET /api/auth/challenge");
    throw err;
  }
}

/**
 * @brief Perform an API request with optional authentication.
 *
 * Authentication behavior:
 *  - If device auth is DISABLED → request is NOT signed
 *  - If device auth is ENABLED  → request MUST be signed
 *
 * This logic mirrors the firmware security model exactly.
 *
 * @param {string} path - API path (e.g. "/api/state")
 * @param {object} opts
 * @returns {Promise<any>} Parsed response
 */
async function apiFetch(path, opts = {}) {
  const { method = "GET", body = null, preferAuth = true } = opts;

  const headers = {};
  let payload = "";

  // Prepare payload (if any)
  if (body !== null) {
    payload = JSON.stringify(body);
    headers["Content-Type"] = "application/json";
  }

  /**
   * Apply authentication ONLY if:
   *  - the device reports auth as enabled
   *  - and this request allows authentication
   */
  if (preferAuth && RuntimeState.authEnabled) {
    if (!hasAuthKey()) {
      throw new Error("Authentication required but no auth key is available");
    }

    const nonce = await requestNonce();
    const signature = await computeSignature(nonce, path, payload);

    headers["X-Nonce"] = nonce;
    headers["X-Auth"] = signature;
  }

  const res = await fetch(`${getBaseUrl()}${path}`, {
    method,
    headers,
    body: payload || undefined,
  });

  const contentType = res.headers.get("content-type") || "";
  const data = contentType.includes("application/json")
    ? await res.json()
    : await res.text();

  if (!res.ok) {
    const msg = typeof data === "string" ? data : JSON.stringify(data);
    throw new Error(`HTTP ${res.status}: ${msg}`);
  }

  const signature = `${method.toUpperCase()} ${path}`;
  recordResponse(signature, data);

  const barePath = path.split("?")[0];
  if (barePath !== path) {
    recordResponse(`${method.toUpperCase()} ${barePath}`, data);
  }

  return data;
}

/* -------------------------------------------------------------------------- */
/*                               PUBLIC API                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieve the full device state.
 *
 * GET /api/state
 *
 * @returns {Promise<object>} Device state payload
 */
export async function fetchDeviceState() {
  try {
    const data = await apiFetch("/api/state", {
      method: "GET",
      preferAuth: true,
    });

    updateFromDeviceState(data);
    return data;
  } catch (err) {
    setLastError(err.message, "GET /api/state");
    throw err;
  }
}

/**
 * @brief Apply system-level configuration.
 *
 * POST /api/setup
 *
 * Behavior:
 *  - Public if auth is disabled
 *  - Authenticated if auth is enabled
 *
 * @param {{ auth: boolean, serialDebug: boolean }} payload
 * @returns {Promise<object>} Setup response
 */
export async function applySetup(payload) {
  try {
    const data = await apiFetch("/api/setup", {
      method: "POST",
      body: payload,
      preferAuth: true,
    });

    /**
     * Setup changes may:
     *  - enable auth
     *  - disable auth
     *  - reset internal state
     *
     * Cached device state must be invalidated.
     */
    clearDeviceState();
    return data;
  } catch (err) {
    setLastError(err.message, "POST /api/setup");
    throw err;
  }
}

/**
 * @brief Update a GPIO pin configuration.
 *
 * PATCH /api/pin/set
 *
 * @param {object} payload
 */
export async function setPin(payload) {
  try {
    const data = await apiFetch("/api/pin/set", {
      method: "PATCH",
      body: payload,
      preferAuth: true,
    });
    return data;
  } catch (err) {
    setLastError(err.message, "PATCH /api/pin/set");
    throw err;
  }
}

/**
 * @brief Add or update a cron job.
 *
 * PATCH /api/cron/set
 *
 * @param {object} payload
 */
export async function setCron(payload) {
  try {
    const data = await apiFetch("/api/cron/set", {
      method: "PATCH",
      body: payload,
      preferAuth: true,
    });
    return data;
  } catch (err) {
    setLastError(err.message, "PATCH /api/cron/set");
    throw err;
  }
}

/**
 * @brief Delete a cron job by ID.
 *
 * DELETE /api/cron?id=<id>
 *
 * @param {number|string} id
 */
export async function deleteCron(id) {
  try {
    const data = await apiFetch(`/api/cron?id=${encodeURIComponent(id)}`, {
      method: "DELETE",
      preferAuth: true,
    });
    return data;
  } catch (err) {
    setLastError(err.message, "DELETE /api/cron");
    throw err;
  }
}

/**
 * @brief Reboot the device.
 *
 * POST /api/reboot
 */
export async function rebootDevice() {
  try {
    const data = await apiFetch("/api/reboot", {
      method: "POST",
      preferAuth: true,
    });
    return data;
  } catch (err) {
    setLastError(err.message, "POST /api/reboot");
    throw err;
  }
}
