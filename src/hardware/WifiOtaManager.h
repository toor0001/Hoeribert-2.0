#pragma once

#include <Arduino.h>

class DisplayManager;

class WifiOtaManager {
public:
  void begin(const char* ssid, const char* password, const char* hostname, DisplayManager* display = nullptr);
  void update();
  String getStatusText() const;
  String getIpText() const;
  bool isReady() const;

private:
  void logLine(const String& text);

  DisplayManager* display = nullptr;
  bool connected = false;
  bool otaReady = false;
  String statusText = "[WIFI] Nicht gestartet";
  String ipText = "";
};
