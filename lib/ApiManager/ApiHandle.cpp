#include "ApiHandle.h"
#include "ApiContext.h"
#include <Auth.h>
#include <CronScheduler.h>
#include <Crypto.h>
#include <DeviceController.h>
#include <EepromConfig.h>
#include <Debug.h>

void handleAuthChallenge() {
  ESP8266WebServer &api = apiServer();

  if (!getAuthEnabled()) {
    sendError("authentication disabled");
    return;
  }
  IPAddress ip = api.client().remoteIP();

  uint32_t nonce = authGenerateChallenge(ip);

  JsonDocument doc;
  doc["nonce"] = nonce;
  sendJSON(doc, 200);
}

void handleSetup() {
  ESP8266WebServer &api = apiServer();

  if (getAuthEnabled() && !checkAuth(JsonDocument()))
    return;

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

  if (obj["serialDebug"].isNull() || obj["auth"].isNull()) {
    sendError("missing parameters");
    return;
  }

  bool authFlag = obj["auth"].as<bool>();
  bool debugFlag = obj["serialDebug"].as<bool>();

  // Persist flags
  setSerialDebugFlag(debugFlag);
  debugSetEnabled(debugFlag);
  authFlag ? enableAuth() : disableAuth();

  JsonDocument resp;
  resp["serialDebug"] = debugFlag;
  resp["auth"] = authFlag;

  if (authFlag) {
    uint8_t key[32];
    generateAuthKey(key);

    char hex[65];
    bytesToHex(key, 32, hex);
    resp["authKey"] = hex;
  }

  sendJSON(resp, 200);
}

void handleGetState() {
  if (!checkAuth(JsonDocument()))
    return;

  JsonDocument doc;
  GpioConfig *pinStates = deviceGetAll();

  JsonObject device = doc["device"].to<JsonObject>();
  device["device"] = "ESP8266";
  device["ip"] = WiFi.localIP().toString();
  device["chip"] = ESP.getChipId();
  device["rssi"] = WiFi.RSSI();
  device["auth"] = getAuthEnabled();
  device["serialDebug"] = debugEnabled();
  device["uptime"] = millis() / 1000;

  // Cron jobs
  CronJob *cronJobs = cronGetAll();
  JsonObject crons = doc["cronJobs"].to<JsonObject>();
  for (int i = 0; i < MAX_CRON_JOBS; i++) {
    JsonObject cron = crons[String(i)].to<JsonObject>();
    cron["state"] = cronJobs[i].active ? "Active" : "Disabled";
    cron["cron"] = cronJobs[i].cron;
    cron["action"] = cronActionToString(cronJobs[i].action);
    cron["pin"] = gpioApiKey(cronJobs[i].pin);
    cron["value"] = cronJobs[i].value;
  }

  // GPIO 0..16
  JsonObject pins = doc["pins"].to<JsonObject>();
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

void handleGetPin() {
  ESP8266WebServer &api = apiServer();

  if (!checkAuth(JsonDocument()))
    return;

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

void handleConfig() {
  ESP8266WebServer &api = apiServer();

  if (!checkAuth(JsonDocument()))
    return;

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

    if (cfgCount >= MAX_GPIO_PINS) {
      sendError("too many pins");
      return;
    }

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

    int state = obj["state"] | 0;

    if (mode == PinMode::Pwm) {
      if (!gpioSupportsPWM(pin) || state < 0 || state > 255) {
        sendError("PWM range 0-255");
        return;
      }
    } else {
      if (!(state == 0 || state == 1)) {
        sendError("digital value must be 0 or 1");
        return;
      }
    }

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

void handlePatchPin() {
  ESP8266WebServer &api = apiServer();

  if (!checkAuth(JsonDocument()))
    return;

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

  if (pin == A0 && !obj["state"].isNull()) {
    sendError("cannot set state on A0");
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

    if (newCfg.mode == PinMode::Pwm) {
      if (!gpioSupportsPWM(pin) || value < 0 || value > 255) {
        sendError("PWM range 0-255");
        return;
      }
    } else {
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
  resp["state"] = newCfg.state;

  sendJSON(resp, 200);
}

void handleReboot() {
  ESP8266WebServer &api = apiServer();

  if (!checkAuth(JsonDocument()))
    return;

  JsonDocument doc;
  doc["rebooting"] = true;
  sendJSON(doc, 200);

  debugPrintln(F("Rebooting - /api/reboot"));
  api.client().flush();
  delay(100);
  ESP.restart();
}

void handleGetCron() {
  ESP8266WebServer &api = apiServer();

  if (!checkAuth(JsonDocument()))
    return;

  if (!api.hasArg("id")) {
    sendError("missing id");
    return;
  }

  int id = api.arg("id").toInt();
  CronJob *job = cronGet(id);

  if (!job) {
    sendError("invalid id");
    return;
  }

  JsonDocument doc;
  doc["state"] = job->active ? "Active" : "Disabled";
  doc["cron"] = job->cron;
  doc["action"] = cronActionToString(job->action);
  doc["pin"] = gpioApiKey(job->pin);
  doc["value"] = job->value;

  sendJSON(doc, 200);
}

void handleCronSet() {
  ESP8266WebServer &api = apiServer();

  if (!checkAuth(JsonDocument()))
    return;

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

  if (obj["cron"].isNull() || obj["action"].isNull()) {
    sendError("missing cron or action");
    return;
  }

  CronJob job{};
  job.active = true;
  job.lastExecEpoch = 0;

  strlcpy(job.cron, obj["cron"].as<const char *>(), sizeof(job.cron));

  String action = obj["action"].as<String>();
  action.toLowerCase();

  if (action == "set") {
    job.action = SetPinState;
  } else if (action == "toggle") {
    job.action = TogglePinState;
  } else if (action == "reboot") {
    job.action = Reboot;
  } else {
    sendError("invalid action");
    return;
  }

  if (job.action == SetPinState || job.action == TogglePinState) {

    if (obj["pin"].isNull()) {
      sendError("missing pin");
      return;
    }

    int pin = apiToGpio(obj["pin"].as<String>());
    if (pin < 0) {
      sendError("invalid pin");
      return;
    }

    job.pin = pin;
    job.value = obj["value"] | 0;
  }

  // trova slot libero
  for (int i = 0; i < MAX_CRON_JOBS; i++) {
    CronJob *c = cronGet(i);
    if (!c->active) {
      if (!setCronJob(i, job)) {
        sendError("save failed", 500);
        return;
      }

      JsonDocument resp;
      resp["success"] = true;
      resp["id"] = i;
      sendJSON(resp, 200);
      return;
    }
  }

  sendError("no free job slot");
}

void handleDeleteCron() {
  ESP8266WebServer &api = apiServer();

  if (!checkAuth(JsonDocument()))
    return;

  if (!api.hasArg("id")) {
    sendError("missing id");
    return;
  }

  int id = api.arg("id").toInt();
  CronJob *job = cronGet(id);

  if (!job) {
    sendError("invalid id");
    return;
  }

  job->active = false;
  setCronJob(id, *job);

  JsonDocument doc;
  doc["success"] = true;
  sendJSON(doc, 200);
}

void handleClearCron() {
  if (!checkAuth(JsonDocument()))
    return;

  for (int i = 0; i < MAX_CRON_JOBS; i++) {
    CronJob *job = cronGet(i);
    job->active = false;
    setCronJob(i, *job);
  }

  JsonDocument doc;
  doc["success"] = true;
  sendJSON(doc, 200);
}
