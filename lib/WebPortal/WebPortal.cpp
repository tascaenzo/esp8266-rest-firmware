#include "WebPortal.h"
#include "EepromConfig.h"

#include <Debug.h>

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

static ESP8266WebServer server(80);
static bool active = false;

/**
 * Scans nearby WiFi networks and generates the HTML <option> list.
 */
String generateNetworkList() {
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    return "<option>No networks found</option>";
  }

  String options;
  for (int i = 0; i < n; i++) {
    options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" +
               String(WiFi.RSSI(i)) + " dBm)" + "</option>";
  }
  return options;
}

/**
 * Generates the HTML page used for WiFi configuration.
 */
String htmlPage() {

  String networks = generateNetworkList();

  String page =
      "<!DOCTYPE html><html><head>"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>"

      "<style>"
      "body{font-family:Arial;background:#f5f5f5;margin:0;padding:20px;}"
      ".card{max-width:400px;margin:auto;background:white;padding:20px;"
      "border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.15);}"
      "h2{text-align:center;color:#333;}"
      "label{font-weight:bold;margin-top:10px;display:block;}"
      "select,input[type=text],input[type=password]{width:100%;padding:10px;"
      "margin-top:5px;border:1px solid #ccc;border-radius:5px;}"
      ".btn{width:100%;padding:12px;background:#2196F3;color:white;border:none;"
      "border-radius:5px;font-size:16px;margin-top:20px;cursor:pointer;}"
      ".btn:hover{background:#1976D2;}"
      "</style>"

      "</head><body>"
      "<div class='card'>"
      "<h2>WiFi Configuration</h2>"

      "<form action='/save' method='POST'>"

      "<label>Select WiFi Network</label>"
      "<select name='ssid'>" +
      networks +
      "</select>"

      "<label>Password</label>"
      "<input type='password' name='pass' placeholder='WiFi Password'>"

      "<button class='btn' type='submit'>Save</button>"
      "</form>"
      "</div></body></html>";

  return page;
}

/**
 * Starts the configuration access point and web server.
 */
bool portalStart() {
  debugPrintln(F("[Portal]"), F("Starting configuration Access Point..."));

  // Dual mode allows both AP mode and WiFi scanning
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP8266-Setup", "12345678");

  debugPrintln(F("[Portal]"), "AP IP address: " + WiFi.softAPIP().toString());

  // Main configuration page
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", htmlPage()); });

  // WiFi credentials saving handler
  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    setWifiCredentials(ssid, pass);

    server.send(200, "text/html",
                "<html><body>"
                "<h2>Credentials saved!</h2>"
                "<p>Device is restarting...</p>"
                "</body></html>");

    delay(1500);
    ESP.restart();
  });

  server.begin();
  active = true;

  debugPrintln(F("[Portal]"), F("Configuration portal started."));
  return true;
}

/**
 * Periodic handler for the web portal.
 */
void portalLoop() {
  if (active)
    server.handleClient();
}

/**
 * Returns whether the configuration portal is currently active.
 */
bool portalActive() { return active; }
