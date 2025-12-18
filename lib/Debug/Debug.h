#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the debug subsystem using the persisted EEPROM flag.
 */
void debugInit();

/**
 * @brief Returns the current runtime debug state.
 */
bool debugEnabled();

/**
 * @brief Overrides the runtime debug state without touching EEPROM.
 */
void debugSetEnabled(bool enabled);

/**
 * @brief Prints a message without a trailing newline when debug is enabled.
 */
void debugPrint(const String &message);

/**
 * @brief Prints a flash-resident string without newline when debug is enabled.
 */
void debugPrint(const __FlashStringHelper *message);

/**
 * @brief Prints a message with newline when debug is enabled.
 */
void debugPrintln(const String &message);

/**
 * @brief Prints a flash-resident string with newline when debug is enabled.
 */
void debugPrintln(const __FlashStringHelper *message);

/**
 * @brief Formatted print with newline when debug is enabled.
 */
void debugPrintf(const char *format, ...);
