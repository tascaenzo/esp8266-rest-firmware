#include "Debug.h"

#include <stdarg.h>
#include <stdio.h>

#include "EepromConfig.h"

namespace {
bool debugActive = false;
}

void debugInit() {
  bool storedFlag = false;
  if (loadDebugFlag(&storedFlag)) {
    debugActive = storedFlag;
  } else {
    debugActive = false;
  }

  Serial.println(debugActive ? F("[DEBUG] Serial debug ENABLED")
                              : F("[DEBUG] Serial debug DISABLED"));
}

bool debugEnabled() { return debugActive; }

void debugSetEnabled(bool enabled) {
  debugActive = enabled;
  Serial.println(debugActive ? F("[DEBUG] Runtime debug ENABLED")
                              : F("[DEBUG] Runtime debug DISABLED"));
}

void debugPrint(const String &message) {
  if (!debugActive)
    return;

  Serial.print(message);
}

void debugPrint(const __FlashStringHelper *message) {
  if (!debugActive)
    return;

  Serial.print(message);
}

void debugPrintln(const String &message) {
  if (!debugActive)
    return;

  Serial.println(message);
}

void debugPrintln(const __FlashStringHelper *message) {
  if (!debugActive)
    return;

  Serial.println(message);
}

void debugPrintf(const char *format, ...) {
  if (!debugActive)
    return;

  char buffer[196];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);
}
