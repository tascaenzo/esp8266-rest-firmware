#include "WifiManager.h"
#include <ESP8266WiFi.h>

/**
 * Initializes the WiFi module in Station (STA) mode
 * and forces a clean disconnection from any previous network.
 */
bool wifiInit() {
  WiFi.mode(WIFI_STA);

  // Force disconnection from any previous network
  WiFi.disconnect();
  delay(100);

  Serial.println("WiFi initialized in STA mode.");
  return true;
}

/**
 * Attempts to connect to a WiFi network and waits
 * until the connection is established or a timeout occurs.
 */
bool wifiConnect(const String &ssid, const String &pass) {
  if (ssid.length() == 0)
    return false;

  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  const unsigned long timeout = 15000; // 15 seconds timeout

  // Wait until connected or timeout expires
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected successfully. IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("WiFi connection failed.");
  return false;
}

/**
 * Returns whether the WiFi interface is currently connected.
 */
bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }

/**
 * Returns the local IP address as a string if connected.
 */
String wifiGetIP() {
  if (!wifiIsConnected())
    return String("");

  return WiFi.localIP().toString();
}
