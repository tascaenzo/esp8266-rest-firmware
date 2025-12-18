/**
 * @file main.cpp
 * @brief Application entry point for WiFi-enabled GPIO controller firmware.
 *
 * This file handles:
 *  - Boot process and WiFi credential loading
 *  - WiFi connection and fallback captive portal
 *  - JSON configuration loading
 *  - Initialization of REST API endpoints
 *  - Registration of a configuration-change callback
 *
 * The application becomes fully operational only after the bootstrap
 * sequence completes, preventing premature hardware updates.
 *
 * @author Enzo Tasca
 * @date 2025
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ApiManager.h"
#include "Auth.h"
#include "BinaryStorage.h"
#include "CronScheduler.h"
#include "DeviceController.h"
#include "EepromConfig.h"
#include "Debug.h"
#include "WebPortal.h"
#include "WifiManager.h"

/* Global state */
String WIFI_SSID;
String WIFI_PASS;

/* Holds the entire JSON configuration */
JsonDocument config;

/* Signals that the system completed initialization
 * and can now notify hardware on config updates */
bool systemBootstrapped = false;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== Device booting ===");

  /* Initialize EEPROM (WiFi credentials & Flags) */
  eepromInit();
  debugInit();
  checkHardwareReset();

  /* Initialize persistent storage FS */
  storageInit();

  /* Initialize WiFi internals */
  wifiInit();

  /* Initialize Device Controller */
  deviceInit();

  /* Load WiFi credentials from EEPROM */
  if (loadWifiCredentials(WIFI_SSID, WIFI_PASS)) {
    debugPrintln("Stored WiFi credentials found: " + WIFI_SSID);

    if (!wifiConnect(WIFI_SSID, WIFI_PASS)) {
      debugPrintln("WiFi connection failed → starting captive portal.");
      portalStart();
    } else {
      debugPrintln("WiFi connected successfully!");
    }
  } else {
    debugPrintln("No WiFi credentials → starting captive portal.");
    portalStart();
  }

  /* Auth config for ApiMenager */
  authInit();

  /* Rest API initialization */
  apiInit();

  /* Cron init */
  cronSchedulerInit();

  systemBootstrapped = true;
  debugPrintln("=== System bootstrap complete ===");
}

void loop() {
  /* If WiFi is not configured yet, run the captive portal */
  if (portalActive()) {
    portalLoop();
    return;
  }

  /* REST API handler */
  apiLoop();

  /* GPIO - read digital and analog inputs */
  deviceLoop();

  /* Cron scheduler */
  cronSchedulerLoop();
}
