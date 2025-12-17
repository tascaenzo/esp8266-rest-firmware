#pragma once

#include <Arduino.h>
#include <IPAddress.h>

/**
 * Maximum number of concurrent authentication slots.
 *
 * Each slot represents a client identified by its IP address
 * that has requested an authentication challenge.
 */
#define MAX_AUTH_SLOTS 8

/**
 * @brief Authentication slot structure.
 *
 * This structure stores the authentication state associated
 * with a single client IP address.
 */
struct AuthSlot {
  IPAddress ip;       ///< Client IP address
  uint32_t nonce;     ///< One-time nonce associated with the client
  uint32_t timestamp; ///< Timestamp of nonce generation (millis)
  bool active;        ///< Slot active flag
};

/**
 * @brief Initializes the authentication system.
 * @return true if successful.
 *
 * This function initializes internal authentication state
 * and clears all authentication slots.
 * It must be called once during application startup.
 */
bool authInit();

/**
 * @brief Generates a new authentication challenge (nonce).
 * @param clientIp IP address of the requesting client.
 * @return The generated nonce.
 *
 * This function creates a one-time nonce bound to the specified
 * client IP address. The nonce must be sent to the client and
 * used to sign the next protected API request coming from the
 * same IP address.
 *
 * If the client already has an active slot, the nonce is
 * replaced. If no free slot is available, the oldest slot
 * is reused.
 */
uint32_t authGenerateChallenge(const IPAddress &clientIp);

/**
 * @brief Verifies authentication of an API request.
 * @param clientIp IP address of the requesting client.
 * @param nonce Nonce provided by the client.
 * @param uri Requested API endpoint.
 * @param payload Raw request payload.
 * @param signature Authentication signature provided by the client.
 * @return true if the request is authenticated.
 *
 * This function validates the request signature using a
 * nonce-based HMAC authentication mechanism.
 *
 * The nonce is bound to the client IP address and is
 * invalidated after a successful verification or timeout.
 *
 * The shared secret is never transmitted over the network.
 */
bool authVerify(const IPAddress &clientIp, uint32_t nonce, const char *uri,
                const char *payload, const char *signature);

/**
 * @brief Checks whether authentication is enabled.
 *
 * @return true if authentication is enabled, false otherwise
 */
bool getAuthEnabled();
