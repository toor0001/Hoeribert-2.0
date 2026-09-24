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
  void showCardProgrammingWaiting(uint16_t episode);
  void showCardProgrammingSuccess(uint16_t episode);
  void showCardProgrammingError(uint16_t episode, const String& message);
  void showCardProgrammingWriting(uint16_t episode);
  void showCardProgrammingRemoval(uint16_t episode);
  void showFolderPlaying(uint16_t folder);
  void showFolderPlaying(uint16_t folder, const String& title);
  bool showFolderImage(uint16_t folder);
  void showBookmarkStatus(bool hasBookmark, uint8_t track = 0, uint16_t seconds = 0);
  void showNotification(const String& title, const String& detail = "");
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
  void drawCardProgrammingScreen(uint16_t episode, const String& status,
                                 const String& detail, uint16_t color);
};
