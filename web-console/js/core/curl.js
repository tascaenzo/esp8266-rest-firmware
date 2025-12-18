/**
 * @brief Build curl commands consistent with current runtime settings.
 *
 * This helper keeps all views aligned with the auth model while
 * presenting copyable firmware calls.
 */

import { getBaseUrl } from "./config.js";
import { RuntimeState } from "./state.js";

/**
 * @param {string} path - API path (e.g. "/api/state")
 * @param {string} method - HTTP verb
 * @param {object|null} body - JSON payload (if any)
 * @param {boolean|null} forceAuth - override for auth header inclusion
 * @returns {string} cURL command
 */
export function buildCurl(path, method = "GET", body = null, forceAuth = null) {
  const base = getBaseUrl() || "";
  const url = `${base}${path}` || path;
  const headers = [];

  const authEnabled = forceAuth ?? RuntimeState.authEnabled;
  if (authEnabled) {
    headers.push('-H "X-Nonce: <nonce>"');
    headers.push('-H "X-Auth: <signature>"');
  }

  if (body) {
    headers.push('-H "Content-Type: application/json"');
  }

  const headerStr = headers.length ? `\\
  ${headers.join(" \\\n  ")}` : "";
  const dataStr = body ? ` \\\n  -d '${JSON.stringify(body)}'` : "";

  return `curl -X ${method.toUpperCase()} ${url}${headerStr}${dataStr}`;
}
