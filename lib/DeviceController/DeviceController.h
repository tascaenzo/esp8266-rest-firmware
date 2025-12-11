#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GpioUtils.h>

/**
 * @brief Initializes all GPIO hardware according to the current configuration.
 *
 * This function:
 * - Configures pin modes
 * - Resets output states
 * - Initializes PWM channels and related timers
 *
 * It should be called once during system startup.
 *
 * @return true if the hardware initialization completed successfully
 */
bool deviceInit();

/**
 * @brief Applies a single GPIO configuration directly to the hardware.
 *
 * This function updates the real hardware state of a pin by applying:
 * - Pin mode configuration
 * - Digital output state
 * - PWM output value (if enabled)
 *
 * The internal cached state is also updated accordingly.
 *
 * @param cfg Reference to a GpioConfig structure
 */
void applyConfigToHardware(const GpioConfig &cfg);

/**
 * @brief Applies a single GPIO configuration to the hardware.
 *
 * This function updates the real hardware state of a pin by applying:
 * - Pin mode configuration
 * - Digital output state
 * - PWM output value (if enabled)
 *
 * The internal cached state is also updated accordingly.
 *
 * @param config Reference to a GpioConfig structure
 * @return true if the configuration was applied successfully, false otherwise
 */
bool deviceSet(GpioConfig &config);

/**
 * @brief Replaces the entire GPIO configuration with a new set.
 *
 * This function applies multiple GPIO configurations at once,
 * updating both the hardware state and the internal cache.
 *
 * @param configs Pointer to an array of GpioConfig structures
 * @param count Number of configurations in the array
 * @return true if all configurations were applied successfully, false otherwise
 */
bool deviceReplaceAll(const GpioConfig *configs, size_t count);

/**
 * @brief Periodic handler for time-based and background GPIO tasks.
 *
 * This function is intended to be called repeatedly inside the main loop().
 * It can be used for:
 * - Software PWM generation
 * - PWM fading effects
 * - State schedulers
 * - Heartbeat signals
 */
void deviceLoop();

/**
 * @brief Returns a pointer to the cached configuration of a specific GPIO pin.
 *
 * This function returns the internally stored state, not the live hardware
 * value.
 *
 * @param pin GPIO number
 * @return Pointer to the cached GpioConfig structure, or nullptr if invalid
 */
GpioConfig *deviceGet(uint8_t pin);

/**
 * @brief Returns a pointer to the complete cached GPIO state table.
 *
 * The returned array contains MAX_GPIO_PINS entries.
 *
 * @return Pointer to the internal GpioConfig state array
 */
GpioConfig *deviceGetAll();

/**
 * @brief Reads the actual hardware state of a GPIO pin.
 *
 * This function bypasses the cached state and directly queries the hardware.
 *
 * @param pin GPIO number
 * @return Current pin value, or -1 if the pin is invalid or not readable
 */
int deviceRead(uint8_t pin);
