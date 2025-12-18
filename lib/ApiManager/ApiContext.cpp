#include "ApiContext.h"
#include <ArduinoJson.h>
#include <Auth.h>

static ESP8266WebServer api(80);

ESP8266WebServer &apiServer() { return api; }

void sendCorsHeaders() {
  ESP8266WebServer &api = apiServer();

  api.sendHeader("Access-Control-Allow-Origin", "*");
  api.sendHeader("Access-Control-Allow-Methods",
                 "GET, POST, PATCH, DELETE, OPTIONS");
  api.sendHeader("Access-Control-Allow-Headers",
                 "Content-Type, X-Nonce, X-Auth");
}

void sendJSON(JsonDocument &doc, int statusCode) {
  String out;
  serializeJson(doc, out);
  sendCorsHeaders();
  api.send(statusCode, "application/json", out);
}

void sendError(const char *msg, int code) {

  JsonDocument doc;
  doc["error"] = msg;
  sendJSON(doc, code);
}

bool checkAuth(const JsonDocument &doc) {
  if (!getAuthEnabled())
    return true; // Authentication disabled

  if (!api.hasHeader("X-Nonce") || !api.hasHeader("X-Auth")) {
    sendError("unauthorized", 401);
    return false;
  }

  IPAddress ip = api.client().remoteIP();
  uint32_t nonce = api.header("X-Nonce").toInt();
  const char *sig = api.header("X-Auth").c_str();

  String payload;

  // serializza solo se il JSON NON è vuoto
  if (!doc.isNull() && doc.size() > 0) {
    serializeJson(doc, payload);
  } else {
    payload = ""; // GET / body assente
  }

  if (!authVerify(ip, nonce, api.uri().c_str(), payload.c_str(), sig)) {
    sendError("unauthorized", 401);
    return false;
  }

  return true;
}
