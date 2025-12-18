#include "DeviceController.h"
#include <BinaryStorage.h>
#include <GpioUtils.h>

#include <Debug.h>

#define STORAGE_PATH "/gpio_state.bin"
#define FILE_SIZE sizeof(GpioConfig) * MAX_GPIO_PINS

static GpioConfig gpioState[MAX_GPIO_PINS];

/**
 * Initializes the GPIO subsystem by restoring the last saved configuration
 * from flash memory. If loading fails, all pins are initialized as Disabled.
 */
bool deviceInit() {
  debugPrintln(F("Initializing DeviceController and loading GPIO state..."));

  bool storageOk = storageRead(STORAGE_PATH, (uint8_t *)gpioState, FILE_SIZE);

  if (!storageOk) {
    debugPrintln(F("Failed to load GPIO state from storage. Initializing all "
                   "pins as Disabled."));
    // If loading fails, initialize all pins as Disabled
    for (int i = 0; i < MAX_GPIO_PINS; i++) {
      gpioState[i] = {(uint8_t)(i + 1), PinMode::Disabled, LOW};
    }
  }

  for (int i = 0; i < MAX_GPIO_PINS; i++) {
    applyConfigToHardware(gpioState[i]);
  }

  return true;
}

/**
 * Applies a single GPIO configuration directly to the hardware
 * without updating the internal cached state or persisting it.
 */
void applyConfigToHardware(const GpioConfig &cfg) {

  // Special case: A0 (Analog)
  if (cfg.pin == A0_INDEX || !gpioIsValid(cfg.pin))
    return;

  debugPrintln("Applying config to pin " + String(cfg.pin) + ": " +
               pinModeToString(cfg.mode) + ", state=" + String(cfg.state));

  switch (cfg.mode) {

  case PinMode::Output:
    pinMode(cfg.pin, OUTPUT);
    digitalWrite(cfg.pin, cfg.state ? HIGH : LOW);
    break;

  case PinMode::Pwm:
    pinMode(cfg.pin, OUTPUT);
    analogWrite(cfg.pin, cfg.state);
    break;

  case PinMode::Input:
    pinMode(cfg.pin, INPUT);
    break;

  case PinMode::InputPullup:
    pinMode(cfg.pin, INPUT_PULLUP);
    break;

  case PinMode::Analog:
  case PinMode::Disabled:
  default:
    break;
  }
}

/**
 * Applies a single GPIO configuration to the hardware and updates
 * the internal cached state. The configuration is also persisted
 * to flash storage.
 */
bool deviceSet(GpioConfig &config) {
  debugPrintln("Setting pin " + String(config.pin) + " to mode " +
               pinModeToString(config.mode) + " with state " +
               String(config.state));

  // SPECIAL CASE: A0 (Analog)
  if (config.pin == A0_INDEX) {
    if (!gpioSupportsAnalog(A0))
      return false;

    gpioState[A0_INDEX].mode = PinMode::Analog;
    gpioState[A0_INDEX].state = analogRead(A0);

    storageWrite(STORAGE_PATH, (uint8_t *)gpioState, FILE_SIZE);
    return true;
  }

  // DIGITAL GPIO
  if (!gpioIsValid(config.pin)) {
    return false;
  }

  switch (config.mode) {

  case PinMode::Output:
    if (!gpioIsSafeOutput(config.pin)) {
      return false;
    }
    pinMode(config.pin, OUTPUT);
    digitalWrite(config.pin, config.state ? HIGH : LOW);
    break;

  case PinMode::Pwm:
    if (!gpioSupportsPWM(config.pin)) {
      return false;
    }
    pinMode(config.pin, OUTPUT);
    analogWrite(config.pin, config.state); // 0–1023
    break;

  case PinMode::Analog:
    // Invalid for all except A0 (already handled above)
    return false;

  case PinMode::Input:
    pinMode(config.pin, INPUT);
    config.state = digitalRead(config.pin);
    break;

  case PinMode::InputPullup:
    if (!gpioSupportsPullup(config.pin))
      return false;
    pinMode(config.pin, INPUT_PULLUP);
    config.state = digitalRead(config.pin);
    break;

  default:
    return false;
  }

  // Update cached state and persist it to flash
  gpioState[config.pin] = config;
  storageWrite(STORAGE_PATH, (uint8_t *)gpioState, FILE_SIZE);

  return true;
}

/**
 * Replace ALL GPIO configurations with a new set.
 *
 *  - Every valid GPIO (0..16) is first set to Disabled
 *  - A0 (17) is reset to Disabled
 *  - Then all configs provided in the `configs[]` array are applied
 *  - Pins NOT included in the array end up as Disabled
 *
 * This function applies settings to both:
 *  - hardware (pinMode, digitalWrite, analogWrite…)
 *  - internal cache (gpioState[])
 *
 * Finally, the whole table is persisted to flash in one shot.
 */
bool deviceReplaceAll(const GpioConfig *configs, size_t count) {

  // Disable all digital pins
  for (int pin = 0; pin <= 16; pin++) {
    if (!gpioIsValid(pin))
      continue;

    gpioState[pin].pin = pin;
    gpioState[pin].mode = PinMode::Disabled;
    gpioState[pin].state = 0;

    pinMode(pin, INPUT);
  }

  // Disable A0
  gpioState[A0_INDEX].pin = A0;
  gpioState[A0_INDEX].mode = PinMode::Disabled;
  gpioState[A0_INDEX].state = 0;

  // Apply each provided config
  for (size_t i = 0; i < count; i++) {

    const GpioConfig &c = configs[i];

    // A0 special case
    if (c.pin == A0) {
      gpioState[A0_INDEX].mode = PinMode::Analog;
      gpioState[A0_INDEX].state = analogRead(A0);
      continue;
    }

    if (!gpioIsValid(c.pin)) {
      // Skip invalid pins rather than failing the entire batch
      continue;
    }

    // Validate and apply mode
    switch (c.mode) {

    case PinMode::Output:
      // if (!gpioIsSafeOutput(c.pin))
      //   return false;
      pinMode(c.pin, OUTPUT);
      digitalWrite(c.pin, c.state ? HIGH : LOW);
      break;

    case PinMode::Pwm:
      if (!gpioSupportsPWM(c.pin))
        return false;
      pinMode(c.pin, OUTPUT);
      analogWrite(c.pin, c.state);
      break;

    case PinMode::Input:
      pinMode(c.pin, INPUT);
      break;

    case PinMode::InputPullup:
      if (!gpioSupportsPullup(c.pin))
        return false;
      pinMode(c.pin, INPUT_PULLUP);
      break;

    case PinMode::Analog:
      // only A0 supports analog
      return false;

    case PinMode::Disabled:
    default:
      pinMode(c.pin, INPUT);
      break;
    }

    gpioState[c.pin] = c;
  }

  // Persist entire table to flash
  return storageWrite(STORAGE_PATH, (uint8_t *)gpioState, FILE_SIZE);
}

/**
 * Returns direct access to the full internal GPIO state table
 * stored in RAM.
 */
GpioConfig *deviceGetAll() { return gpioState; }

/**
 * Returns a pointer to the cached configuration of a specific GPIO pin.
 * If the pin is invalid, a null pointer is returned.
 */
GpioConfig *deviceGet(uint8_t pin) {
  if (pin == A0)
    return &gpioState[A0_INDEX];
  if (!gpioIsValid(pin))
    return nullptr;
  return &gpioState[pin];
}

/**
 * Reads the actual hardware state of a GPIO pin,
 * bypassing the cached value.
 */
int deviceRead(uint8_t pin) {

  if (pin == A0) {
    return analogRead(A0);
  }

  if (!gpioIsValid(pin))
    return -1;

  switch (gpioState[pin].mode) {
  case PinMode::Output:
    return digitalRead(pin);

  case PinMode::Pwm:
    return -1; // PWM readback not supported

  case PinMode::Input:
  case PinMode::InputPullup:
    return digitalRead(pin);

  default:
    return -1;
  }
}

/**
 * Periodic handler used to refresh the cached state of digital input
 * and analog input pins.
 */
void deviceLoop() {

  // Refresh digital inputs
  for (int pin = 0; pin <= 16; pin++) {
    if (!gpioIsValid(pin))
      continue;

    if (gpioState[pin].mode == PinMode::Input ||
        gpioState[pin].mode == PinMode::InputPullup) {
      gpioState[pin].state = digitalRead(pin);
    }
  }

  // Refresh A0 analog reading
  if (gpioState[A0_INDEX].mode == PinMode::Analog) {
    gpioState[A0_INDEX].state = analogRead(A0);
  }
}
