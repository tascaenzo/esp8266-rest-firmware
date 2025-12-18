/**
 * @brief Authentication helper for the Firmware Console.
 *
 * This module manages the shared authentication secret
 * and provides helpers to compute HMAC-SHA256 signatures.
 *
 * Responsibilities:
 *  - Store and retrieve the auth key
 *  - Validate auth key format
 *  - Compute request signatures
 *
 * This module performs NO network requests
 * and has NO knowledge of API endpoints.
 */

/**
 * @brief LocalStorage key used to persist the authentication secret.
 */
const STORAGE_KEY = "fwconsole.authKey";

/**
 * @brief Check whether a string is a valid hex-encoded auth key.
 *
 * The firmware expects a 32-byte key, hex-encoded (64 chars).
 *
 * @param {string} key
 * @returns {boolean} true if the key is valid
 */
export function isValidAuthKey(key) {
  return typeof key === "string" && /^[0-9a-fA-F]{64}$/.test(key.trim());
}

/**
 * @brief Retrieve the stored authentication key.
 *
 * @returns {string|null} Hex-encoded auth key, or null if not set
 */
export function getAuthKey() {
  const key = localStorage.getItem(STORAGE_KEY);
  return key && isValidAuthKey(key) ? key.toLowerCase() : null;
}

/**
 * @brief Store a new authentication key.
 *
 * @param {string} key - Hex-encoded auth key
 * @throws Error if the key format is invalid
 */
export function setAuthKey(key) {
  if (!isValidAuthKey(key)) {
    throw new Error("Invalid auth key format");
  }
  localStorage.setItem(STORAGE_KEY, key.toLowerCase());
}

/**
 * @brief Clear the stored authentication key.
 *
 * This does NOT affect the device.
 */
export function clearAuthKey() {
  localStorage.removeItem(STORAGE_KEY);
}

/**
 * @brief Check whether a valid authentication key is available.
 *
 * @returns {boolean}
 */
export function hasAuthKey() {
  return !!getAuthKey();
}

/**
 * @brief Compute an HMAC-SHA256 signature for an authenticated request.
 *
 * Signature format (MUST match firmware exactly):
 *
 *   HMAC_SHA256(
 *     nonce + uri + payload,
 *     AUTH_SECRET
 *   )
 *
 * Payload rules:
 *  - GET requests: empty string
 *  - POST/PATCH: raw JSON string (no reformatting)
 *
 * @param {string} nonce   - Nonce returned by /api/auth/challenge
 * @param {string} uri     - Request path (e.g. "/api/state")
 * @param {string} payload - Raw request body or empty string
 * @returns {Promise<string>} Hex-encoded HMAC signature
 */
export async function computeSignature(nonce, uri, payload = "") {
  const keyHex = getAuthKey();
  if (!keyHex) {
    throw new Error("Auth key not available");
  }

  const message = `${nonce}${uri}${payload}`;
  return hmacSha256Hex(keyHex, message);
}

/* -------------------------------------------------------------------------- */
/*                                CRYPTO HELPERS                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a hex string into a Uint8Array.
 *
 * @param {string} hex
 * @returns {Uint8Array}
 */
function hexToBytes(hex) {
  const bytes = new Uint8Array(hex.length / 2);
  for (let i = 0; i < hex.length; i += 2) {
    bytes[i / 2] = parseInt(hex.slice(i, i + 2), 16);
  }
  return bytes;
}

/**
 * @brief Convert a Uint8Array into a hex string.
 *
 * @param {Uint8Array} bytes
 * @returns {string}
 */
function bytesToHex(bytes) {
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

/**
 * @brief Compute HMAC-SHA256 using the Web Crypto API.
 *
 * This implementation is deterministic and compatible
 * with the firmware-side HMAC computation.
 *
 * @param {string} keyHex  - Hex-encoded secret key
 * @param {string} message - Message to sign
 * @returns {Promise<string>} Hex-encoded signature
 */
async function hmacSha256Hex(keyHex, message) {
  if (!crypto?.subtle) {
    throw new Error("Web Crypto API not available");
  }

  const keyBytes = hexToBytes(keyHex);
  const data = new TextEncoder().encode(message);

  const cryptoKey = await crypto.subtle.importKey(
    "raw",
    keyBytes,
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign"]
  );

  const signature = await crypto.subtle.sign("HMAC", cryptoKey, data);
  return bytesToHex(new Uint8Array(signature));
}
