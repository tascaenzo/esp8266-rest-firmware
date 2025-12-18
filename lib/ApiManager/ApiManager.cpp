#include "ApiManager.h"
#include "ApiContext.h"
#include "ApiHandle.h"
#include <ArduinoJson.h>
#include <Auth.h>
#include <CronScheduler.h>
#include <DeviceController.h>
#include <ESP8266WebServer.h>
#include <EepromConfig.h>

bool apiInit() {
  ESP8266WebServer &api = apiServer();

  api.collectHeaders("X-Nonce", "X-Auth");

  api.begin();
  Serial.println("REST API started on port 80");

  api.on("/api/auth/challenge", HTTP_GET, handleAuthChallenge);
  api.on("/api/setup", HTTP_POST, handleSetup);
  api.on("/api/state", HTTP_GET, handleGetState);
  api.on("/api/pin", HTTP_GET, handleGetPin);
  api.on("/api/config", HTTP_POST, handleConfig);
  api.on("/api/pin/set", HTTP_PATCH, handlePatchPin);
  api.on("/api/reboot", HTTP_POST, handleReboot);
  api.on("/api/cron/set", HTTP_PATCH, handleCronSet);
  api.on("/api/cron", HTTP_GET, handleGetCron);
  api.on("/api/cron", HTTP_DELETE, handleDeleteCron);
  api.on("/api/cron/clear", HTTP_DELETE, handleClearCron);

  return true;
}

void apiLoop() {
  ESP8266WebServer &api = apiServer();

  api.handleClient();
}
