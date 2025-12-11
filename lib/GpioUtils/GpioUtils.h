#pragma once
#include <Arduino.h>

/**
 * @brief Maximum number of GPIO pins handled by the device.
 *
 * GPIO16 is the highest usable pin on ESP8266, so the array size is 18.
 */
#define MAX_GPIO_PINS 18

/**
 * @brief Describes the hardware capabilities of a GPIO pin.
 *
 * A pin may support:
 * - Digital input/output
 * - Analog input (A0 only)
 * - PWM output (supported on most pins except restricted ones)
 */
enum PinCapability {
  DigitalIO,
  AnalogInput,
  PwmOutput,
};

/**
 * @brief Operational mode of a GPIO pin at runtime.
 *
 * Each mode determines how the pin behaves in the firmware:
 * - Disabled: Pin not used
 * - Input: Floating digital input
 * - InputPullup: Digital input with internal pull-up
 * - Output: Digital output (HIGH / LOW)
 * - Pwm: PWM-controlled output
 * - Analog: ADC input (A0 only)
 */
enum PinMode { Disabled = 0, Input, InputPullup, Output, Pwm, Analog };

/**
 * @brief Runtime configuration and state of a GPIO pin.
 *
 * This structure stores:
 * - The pin number
 * - The selected pin mode
 * - The last known logical state:
 *   - For Output/PWM → last written value
 *   - For Input/InputPullup → last read digital value
 *   - For Analog → last ADC reading (0–1023)
 */
struct GpioConfig {
  uint8_t pin;
  PinMode mode;
  int state;
};

/**
 * @brief Checks whether a pin is a valid GPIO on ESP8266.
 *
 * This excludes:
 * - Flash pins (GPIO 6–11)
 * - Non-existent pins (>16)
 *
 * @param pin GPIO number
 * @return true if the pin is usable, false otherwise
 */
bool gpioIsValid(uint8_t pin);

/**
 * @brief Checks whether a pin can be safely used as OUTPUT.
 *
 * @param pin GPIO number
 * @return true if the pin is safe for output use
 */
bool gpioIsSafeOutput(uint8_t pin);

/**
 * @brief Checks whether a pin supports PWM output.
 *
 * @param pin GPIO number
 * @return true if PWM is supported on the pin
 */
bool gpioSupportsPWM(uint8_t pin);

/**
 * @brief Checks whether a pin is boot-sensitive.
 *
 * Boot-sensitive pins (GPIO 0, 2, 15) affect the ESP8266 boot mode
 * and must be handled carefully.
 *
 * @param pin GPIO number
 * @return true if the pin is boot-sensitive
 */
bool gpioIsBootSensitive(uint8_t pin);

/**
 * @brief Checks whether a pin supports analog input.
 *
 * @param pin GPIO number
 * @return true if analog input is supported
 */
bool gpioSupportsAnalog(uint8_t pin);

/**
 * @brief Checks whether a pin supports internal pull-up resistors.
 *
 * @param pin GPIO number
 * @return true if pull-up is supported on the pin
 */
bool gpioSupportsPullup(uint8_t pin);

/**
 * @brief Converts a PinMode enum value to its string representation.
 *
 * @param mode PinMode value
 * @return Corresponding string ("Input", "Output", etc.)
 */
String pinModeToString(PinMode mode);

/**
 * @brief Converts a string representation to a PinMode enum value.
 *
 * @param modeStr String representation of the pin mode
 * @return Corresponding PinMode value
 */
PinMode stringToPinMode(const String &modeStr);

/**
 * @brief Provides a safety description string for a given GPIO pin.
 *
 * The safety description indicates whether the pin is safe to use,
 * boot-sensitive, or has special considerations.
 *
 * @param pin GPIO number
 * @return Safety description string ("Safe", "BootSensitive", "Warn")
 */
String pinSafetyString(uint8_t pin);
