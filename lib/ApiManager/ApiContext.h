#pragma once

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

/**
 * @brief Returns the singleton instance of the HTTP API server.
 *
 * This function exposes a single shared ESP8266WebServer instance
 * used by all API modules (routing, authentication, handlers).
 *
 * The server is created and configured during apiInit() and
 * accessed through this function to avoid global variables
 * scattered across multiple translation units.
 *
 * @return Reference to the global ESP8266WebServer instance
 */
ESP8266WebServer &apiServer();

/**
 * @brief Sends a JSON document as an HTTP response.
 *
 * This helper serializes the provided ArduinoJson document and
 * sends it with the given HTTP status code and the
 * "application/json" content type.
 *
 * It centralizes JSON response handling to ensure consistent
 * formatting and headers across all API endpoints.
 *
 * @param doc        JSON document to send
 * @param statusCode HTTP status code (e.g. 200, 400, 401)
 */
void sendJSON(JsonDocument &doc, int statusCode);

/**
 * @brief Sends a standardized JSON error response.
 *
 * The response format is:
 * { "error": "<message>" }
 *
 * This function should be used for all API errors to ensure
 * consistent error reporting and simplify client-side handling.
 *
 * @param msg  Error message string
 * @param code HTTP status code (default: 400)
 */
void sendError(const char *msg, int code = 400);

/**
 * @brief Verifies API authentication for the current request.
 *
 * This function performs authentication checks for protected
 * API endpoints. Its behavior depends on the current device
 * configuration:
 *
 * - If authentication is disabled, the request is always accepted
 * - If authentication is enabled, the function validates:
 *   - Presence of X-Nonce and X-Auth headers
 *   - HMAC signature correctness
 *   - Nonce validity and freshness
 *
 * If authentication fails, an HTTP 401 response is automatically
 * sent and the function returns false.
 *
 * This function should be called at the beginning of every
 * protected API handler.
 *
 * @param doc JSON document representing the request body
 *            (used for signature verification)
 * @return true if the request is authorized, false otherwise
 */
bool checkAuth(const JsonDocument &doc);

/**
 * @brief Sends HTTP CORS headers for browser-based clients.
 *
 * This function adds the required Cross-Origin Resource Sharing (CORS)
 * headers to the current HTTP response.
 *
 * It allows web applications running in a browser (HTML/JS)
 * to access the REST API exposed by the device, even when the page
 * is served from a different origin (e.g. local file or another host).
 *
 * The headers enable:
 * - Cross-origin requests
 * - Custom authentication headers (X-Nonce, X-Auth)
 * - Preflight OPTIONS requests required by modern browsers
 *
 * This function does NOT perform authentication checks and should be
 * called before sending any API response.
 *
 * Security note:
 * CORS does not replace authentication. All protected endpoints
 * still require valid HMAC-based authentication.
 */
void sendCorsHeaders();
