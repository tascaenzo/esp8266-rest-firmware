#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the HTTP API server.
 * @return true if successful.
 *
 * This function sets up all API endpoints and prepares the web server
 * to handle incoming HTTP requests from external clients.
 */
bool apiInit();

/**
 * @brief Periodic handler for the HTTP API server.
 *
 * This function must be called repeatedly inside the main loop()
 * to process incoming client requests and keep the API responsive.
 */
void apiLoop();
