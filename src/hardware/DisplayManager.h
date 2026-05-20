#pragma once

#include <Arduino.h>
#include <Adafruit_ILI9341.h>

class DisplayManager {
public:
  void begin();
  void clear();
  void showBootScreen();
  void showHardwareTestScreen();
  void showNormalIdle();
  void showFolderPlaying(uint8_t folder);
  void showFolderPlaying(uint8_t folder, const String& title);
  bool showFolderImage(uint8_t folder);
  void showBookmarkStatus(bool hasBookmark, uint8_t track = 0, uint16_t seconds = 0);
  void showCardProblem(const String& text);
  void showSleepTimerRemaining(unsigned long remainingSeconds);
  void clearSleepTimer();
  void logLine(const String& text);
  void showError(const String& text);
  void setEnabled(bool enabled);
  bool isEnabled() const;

private:
  static constexpr uint8_t TFT_CS_PIN  = 25;
  static constexpr uint8_t TFT_DC_PIN  = 26;
  static constexpr uint8_t TFT_RST_PIN = 33;
  static constexpr uint8_t TFT_BACKLIGHT_PIN = 32;
  static constexpr int LINE_HEIGHT = 10;

  Adafruit_ILI9341 tft{TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN};
  int screenY = 0;
  bool enabled = true;
  bool imageFileSystemReady = false;

  void drawPlayingHeader();
};
