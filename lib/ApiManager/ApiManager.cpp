#include "ApiManager.h"
#include <ArduinoJson.h>
#include <DeviceController.h>
#include <ESP8266WebServer.h>

static ESP8266WebServer api(80);

/**
 * Utility: Send a JSON document as HTTP response
 */
void sendJSON(JsonDocument &doc, int statusCode) {
  String out;
  serializeJson(doc, out);
  api.send(statusCode, "application/json", out);
}

/**
 * Utility: Send an error JSON
 */
void sendError(const char *msg, int code = 400) {
  JsonDocument doc;
  doc["error"] = msg;
  sendJSON(doc, code);
}

/**
 * Utility: Convert API pin ID to GPIO number
 * Returns -1 if invalid
 */
int apiToGpio(String id) {
  id.toUpperCase();

  if (id.startsWith("GPIO"))
    id = id.substring(4);

  if (id == "A0")
    return A0;

  if (!id.equals(String(id.toInt())))
    return -1; // invalid format

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

/**
 * GET /api/state
 * Returns the entire JSON configuration of all GPIO pins and device info
 */
void handleGetState() {
  JsonDocument doc;
  GpioConfig *pinStates = deviceGetAll();

  JsonObject device = doc["device"].to<JsonObject>();
  device["device"] = "ESP8266";
  device["ip"] = WiFi.localIP().toString();
  device["chip"] = ESP.getChipId();
  device["rssi"] = WiFi.RSSI();
  device["uptime"] = millis() / 1000;

  JsonObject pins = doc["pins"].to<JsonObject>();

  // GPIO 0..16
  for (int pin = 0; pin <= 16; pin++) {

    if (!gpioIsValid(pin))
      continue;

    JsonObject p = pins[gpioApiKey(pin)].to<JsonObject>();

    p["mode"] = pinModeToString(pinStates[pin].mode);
    p["state"] = pinStates[pin].state;

    JsonArray caps = p["capabilities"].to<JsonArray>();
    caps.add("Input");

    if (pin != 16)
      caps.add("InputPullup");

    caps.add("Output");

    if (gpioSupportsPWM(pin))
      caps.add("Pwm");

    p["safety"] = pinSafetyString(pin);
  }

  // A0 — analog
  JsonObject a0 = pins["A0"].to<JsonObject>();
  a0["mode"] = "Analog";
  a0["state"] = analogRead(A0);

  JsonArray capsA0 = a0["capabilities"].to<JsonArray>();
  capsA0.add("Analog");

  sendJSON(doc, 200);
}

/**
 * GET /api/pin?id=N
 * Returns the JSON configuration of a specific GPIO pin
 */
void handleGetPin() {

  if (!api.hasArg("id")) {
    sendError("missing pin");
    return;
  }

  String raw = api.arg("id");
  int pin = apiToGpio(raw);

  if (pin < 0) {
    sendError("invalid pin");
    return;
  }

  JsonDocument doc;
  doc["id"] = gpioApiKey(pin);

  if (pin == A0) {
    doc["mode"] = "Analog";
    doc["state"] = analogRead(A0);
  } else {
    GpioConfig *s = deviceGet(pin);
    doc["mode"] = pinModeToString(s->mode);
    doc["state"] = s->state;
  }

  sendJSON(doc, 200);
}

/**
 * POST /api/config
 * Fully replaces the GPIO configuration
 * Validates pin/mode compatibility
 */
void handleConfig() {

  if (!api.hasArg("plain")) {
    sendError("missing body");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, api.arg("plain"))) {
    sendError("invalid json");
    return;
  }

  JsonObject pinsJson = doc.as<JsonObject>();

  GpioConfig newConfigs[MAX_GPIO_PINS];
  size_t cfgCount = 0;

  // Initialize defaults: everything Disabled
  for (int i = 0; i < MAX_GPIO_PINS; i++) {
    newConfigs[i] = {(uint8_t)i, PinMode::Disabled, 0};
  }

  // Parse JSON entries
  for (JsonPair kv : pinsJson) {

    String key = kv.key().c_str();
    JsonObject obj = kv.value().as<JsonObject>();

    int pin = apiToGpio(key);
    if (pin < 0) {
      sendError("invalid pin id");
      return;
    }

    if (pin == A0) {
      if (obj["mode"].isNull() ||
          obj["mode"].as<String>().compareTo("Analog") != 0) {
        sendError("A0 only supports Analog");
        return;
      }
      continue;
    }

    if (obj["mode"].isNull()) {
      sendError("missing mode");
      return;
    }

    String modeStr = obj["mode"].as<String>();
    modeStr.toLowerCase();

    PinMode mode = stringToPinMode(modeStr);

    if (mode == PinMode::Disabled && modeStr != "disabled") {
      sendError("invalid mode");
      return;
    }

    int state = obj["state"].isNull() ? 0 : obj["state"].as<int>();

    newConfigs[cfgCount++] = {(uint8_t)pin, mode, state};
  }

  if (!deviceReplaceAll(newConfigs, cfgCount)) {
    JsonDocument err;
    err["success"] = false;
    sendJSON(err, 500);
    return;
  }

  JsonDocument ok;
  ok["success"] = true;
  sendJSON(ok, 200);
}

/**
 * PATCH /api/pin/set
 * Validates and applies pin, mode and value from JSON body.
 * Updates hardware using deviceSet().
 */
void handlePatchPin() {

  if (!api.hasArg("plain")) {
    sendError("missing body");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, api.arg("plain"))) {
    sendError("invalid json");
    return;
  }

  JsonObject obj = doc.as<JsonObject>();

  if (obj["id"].isNull()) {
    sendError("missing id");
    return;
  }

  String id = obj["id"].as<String>();
  int pin = apiToGpio(id);

  if (pin < 0) {
    sendError("invalid pin");
    return;
  }

  GpioConfig *existing = deviceGet(pin);
  if (!existing) {
    sendError("internal error", 500);
    return;
  }

  GpioConfig newCfg = *existing;

  // Validate "mode"
  if (!obj["mode"].isNull()) {

    String modeStr = obj["mode"].as<String>();
    modeStr.toLowerCase();

    PinMode mode = stringToPinMode(modeStr);

    if (mode == PinMode::Disabled && modeStr != "disabled") {
      sendError("invalid mode");
      return;
    }

    if (pin == A0 && mode != PinMode::Analog) {
      sendError("A0 only supports Analog");
      return;
    }

    if (pin == 16 && (mode == PinMode::InputPullup || mode == PinMode::Pwm)) {
      sendError("mode not supported on GPIO16");
      return;
    }

    newCfg.mode = mode;
  }

  // Validate "state"
  if (!obj["state"].isNull()) {

    if (!obj["state"].is<int>()) {
      sendError("invalid value type");
      return;
    }

    int value = obj["state"].as<int>();

    if (gpioSupportsPWM(pin)) {
      if (value < 0 || value > 255) {
        sendError("PWM range 0-255");
        return;
      }
    }

    if (!gpioSupportsPWM(pin) && pin != A0) {
      if (!(value == 0 || value == 1)) {
        sendError("digital value must be 0 or 1");
        return;
      }
    }

    newCfg.state = value;
  }

  if (!deviceSet(newCfg)) {
    sendError("apply failed", 500);
    return;
  }

  // Response
  JsonDocument resp;
  resp["id"] = id;
  resp["mode"] = pinModeToString(newCfg.mode);
  resp["state"] = deviceRead(pin);

  sendJSON(resp, 200);
}

/**
 * POST /api/reboot
 */
void handleReboot() {
  JsonDocument doc;
  doc["rebooting"] = true;
  sendJSON(doc, 200);

  Serial.println("Rebooting - /api/reboot");
  delay(300);
  ESP.restart();
}

/**
 * Initialization Web server
 */
bool apiInit() {

  api.begin();
  Serial.println("REST API started on port 80");

  api.on("/api/state", HTTP_GET, handleGetState);
  api.on("/api/pin", HTTP_GET, handleGetPin);
  api.on("/api/config", HTTP_POST, handleConfig);
  api.on("/api/pin/set", HTTP_PATCH, handlePatchPin);
  api.on("/api/reboot", HTTP_POST, handleReboot);

  return true;
}

void apiLoop() { api.handleClient(); }
