/*
 * ===========================================================
 *  ESP8266 GPIO REFERENCE
 * ===========================================================
 *
 *  AVAILABLE PINS
 *  -----------------------------------------------------------
 *  GPIO 0–5, 12–16  → usable digital GPIO pins
 *  GPIO 6–11        → RESERVED for SPI flash (DO NOT USE)
 *  A0               → analog input (ADC), not a digital GPIO
 *
 *  BOOT-SENSITIVE PINS  (safety = "critical")
 *  -----------------------------------------------------------
 *    GPIO0  → must be HIGH on boot (LOW enters flash mode)
 *    GPIO2  → must be HIGH on boot
 *    GPIO15 → must be LOW  on boot
 *
 *  Any wrong state during power-up can cause boot failure.
 *
 *  SPECIAL-PURPOSE PINS  (safety = "warn")
 *  -----------------------------------------------------------
 *    GPIO1  → UART0 TX (boot messages printed on startup)
 *    GPIO3  → UART0 RX
 *    GPIO16 → deep-sleep wake pin, NO PWM, NO pull-up support,
 *              uses RTC domain (limited features)
 *
 *  SAFE OUTPUT PINS  (safety = "safe")
 *  -----------------------------------------------------------
 *    GPIO4, GPIO5,
 *    GPIO12, GPIO13, GPIO14
 *    GPIO15 → safe *after* boot has completed
 *
 *  These pins have no boot restrictions and full I/O capability.
 *
 *  PWM SUPPORT
 *  -----------------------------------------------------------
 *    ALL GPIO except GPIO16 support PWM output.
 *
 *
 *  INTERNAL PULL-UP SUPPORT
 *  -----------------------------------------------------------
 *    Supported on all GPIO except GPIO16.
 *
 *  SUMMARY OF SAFETY CLASSES
 *  -----------------------------------------------------------
 *    safety = "Safe"           → fully recommended for Output/PWM
 *    safety = "Warn"           → usable but with restrictions
 *    safety = "BootSensitive"  → affects boot mode; use with caution
 *
 *  NOTE:
 *    Development boards (NodeMCU, Wemos D1 Mini) may buffer
 *    or isolate certain pins (e.g. onboard LED on GPIO2),
 *    reducing danger — BUT raw ESP-12 modules do not.
 *
 * ===========================================================
 */

#include "GpioUtils.h"
#include <Arduino.h>

/**
 * Checks whether a pin is a valid GPIO on ESP8266.
 */
bool gpioIsValid(uint8_t pin) {
  if (pin > 16)
    return false;

  // GPIO 6–11 are Flash SPI pins → NEVER valid
  if (pin >= 6 && pin <= 11)
    return false;

  return true;
}
/**
 *  "Safe" means: recommended, reliable, no boot conflicts.
 *
 *  Fully safe:
 *    GPIO4, GPIO5, GPIO12, GPIO13, GPIO14
 *
 *  Conditionally safe:
 *    GPIO15 → safe as output AFTER BOOT
 *
 *  Not recommended:
 *    GPIO0, GPIO2  → boot-critical (must be HIGH at boot)
 *    GPIO1, GPIO3  → UART pins (TX/RX)
 *    GPIO16        → no PWM, no interrupt (but works)
 */
bool gpioIsSafeOutput(uint8_t pin) {
  if (!gpioIsValid(pin))
    return false;

  switch (pin) {
  case 4:
  case 5:
  case 12:
  case 13:
  case 14:
    return true;

  case 15:
  case 2:
  case 0:
    // Safe as output, but only after boot
    return false;

  default:
    return false; // 1,3,16 (usable but NOT safe)
  }
}

/**
 *  ESP8266 supports PWM on ALL valid GPIO except:
 *    - GPIO16 (no PWM hardware)
 *    - A0 (not a GPIO)
 *
 *  NOTE:
 *    GPIO1 and GPIO3 support PWM electrically,
 *    but using PWM on TX/RX disables serial communication.
 */
bool gpioSupportsPWM(uint8_t pin) {
  if (!gpioIsValid(pin))
    return false;

  if (pin == 16)
    return false;

  return true;
}

/**
 *  These must be in specific states during reset:
 *
 *    GPIO0  → must be HIGH (LOW = flash mode)
 *    GPIO2  → must be HIGH
 *    GPIO15 → must be LOW
 *
 *  Using them as outputs may break boot if improperly set.
 */
bool gpioIsBootSensitive(uint8_t pin) {
  return (pin == 0 || pin == 2 || pin == 15);
}

/**
 *  ESP8266 has ONE analog input:
 *    A0  → 10-bit ADC (0–1V)
 *
 *  A0 is NOT a GPIO and has no digital functions.
 *
 *  NOTE:
 *    In Arduino core, A0 = 17
 *    So the check is correct.
 */
bool gpioSupportsAnalog(uint8_t pin) { return (pin == A0); }

/**
 * ESP8266 supports internal pull-up resistors on all GPIO
 */
bool gpioSupportsPullup(uint8_t pin) {
  if (!gpioIsValid(pin))
    return false;

  if (pin == 16)
    return false;

  return true;
}

/**
 * Converts a PinMode enum value to its string representation.
 */
String pinModeToString(PinMode mode) {
  switch (mode) {
  case PinMode::Input:
    return "Input";
  case PinMode::InputPullup:
    return "InputPullup";
  case PinMode::Output:
    return "Output";
  case PinMode::Pwm:
    return "Pwm";
  case PinMode::Analog:
    return "Analog";
  case PinMode::Disabled:
  default:
    return "Disabled";
  }
}

/**
 * Converts a string representation to a PinMode enum value.
 */
PinMode stringToPinMode(const String &modeStr) {
  String modeLower = modeStr;
  modeLower.toLowerCase();

  if (modeLower == "input")
    return PinMode::Input;
  else if (modeLower == "inputpullup")
    return PinMode::InputPullup;
  else if (modeLower == "output")
    return PinMode::Output;
  else if (modeLower == "pwm")
    return PinMode::Pwm;
  else if (modeLower == "analog")
    return PinMode::Analog;
  else
    return PinMode::Disabled;
}

/**
 *  Returns the safety classification of a GPIO pin.
 *
 *  Safety levels:
 *    - "safe"          → recommended, no boot conflicts
 *    - "warn"          → usable but with limitations
 *    - "boot-sensitive"→ affects boot mode, use with caution
 */
String pinSafetyString(uint8_t pin) {

  // Safe pins (GPIO4,5,12,13,14 and GPIO15 after boot)
  if (gpioIsSafeOutput(pin))
    return "Safe";

  // Boot-sensitive pins (GPIO0, GPIO2, GPIO15)
  if (gpioIsBootSensitive(pin))
    return "BootSensitive";

  // UART pins, GPIO16, or anything unusual
  return "Warn";
}

/**
 * Utility: Convert API pin ID to GPIO number
 * Returns -1 if invalid
 */
int apiToGpio(String id) {
  id.trim();
  id.toUpperCase();

  if (id == "A0")
    return A0;

  if (id.startsWith("GPIO"))
    id = id.substring(4);

  if (id.length() == 0)
    return -1;

  for (char c : id) {
    if (!isDigit(c))
      return -1;
  }

  int pin = id.toInt();
  if (!gpioIsValid(pin))
    return -1;

  return pin;
}

/**
 * Utility: Convert GPIO number to API pin ID
 */
String gpioApiKey(int pin) {
  if (pin == A0)
    return "A0";
  return "GPIO" + String(pin);
}