#include "WifiOtaManager.h"

#include "DisplayManager.h"

#include <ArduinoOTA.h>
#include <WiFi.h>

void WifiOtaManager::begin(const char* ssid, const char* password, const char* hostname, DisplayManager* displayManager) {
  display = displayManager;
  connected = false;
  otaReady = false;
  ipText = "";

  WiFi.mode(WIFI_STA);
  Serial.println("Free heap:");
  Serial.println(ESP.getFreeHeap());

  WiFi.begin(ssid, password);

  statusText = "[WIFI] Verbinde...";
  logLine(statusText);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    statusText = "[WIFI] Keine Verbindung";
    logLine(statusText);
    return;
  }

  connected = true;
  statusText = "[WIFI] Verbunden";
  ipText = WiFi.localIP().toString();

  logLine(statusText);
  logLine("[WIFI] IP: " + ipText);
  Serial.println(WiFi.RSSI());

  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.onStart([this]() {
    logLine("[OTA] Start");
  });

  ArduinoOTA.onEnd([this]() {
    logLine("[OTA] Fertig");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned long last = 0;
    if (millis() - last > 1000) {
      last = millis();
      Serial.printf("[OTA] %u%%\n", (progress * 100) / total);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Fehler: %u\n", error);
  });

  ArduinoOTA.begin();
  otaReady = true;

  logLine("[OTA] Bereit: " + String(hostname) + ".local");
}

void WifiOtaManager::update() {
  ArduinoOTA.handle();
}

String WifiOtaManager::getStatusText() const {
  return statusText;
}

String WifiOtaManager::getIpText() const {
  return ipText;
}

bool WifiOtaManager::isReady() const {
  return connected && otaReady;
}

void WifiOtaManager::logLine(const String& text) {
  if (display != nullptr) {
    display->logLine(text);
  } else {
    Serial.println(text);
  }
}
