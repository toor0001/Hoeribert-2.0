#include "DisplayManager.h"

#include <Adafruit_GFX.h>
#include <LittleFS.h>
#include <TJpg_Decoder.h>

namespace {

Adafruit_ILI9341* jpgTft = nullptr;

bool jpgOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (jpgTft == nullptr) return false;
  if (y >= jpgTft->height()) return true;

  jpgTft->drawRGBBitmap(x, y, bitmap, w, h);
  return true;
}

String folderImagePath(uint8_t folder) {
  return "/" + String(folder) + ".jpg";
}

String paddedFolderImagePath(uint8_t folder) {
  if (folder >= 10) return folderImagePath(folder);
  return "/0" + String(folder) + ".jpg";
}

} // namespace

void DisplayManager::begin() {
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
  tft.begin();
  tft.setRotation(3);
  jpgTft = &tft;
  TJpgDec.setSwapBytes(false);
  TJpgDec.setCallback(jpgOutput);
  imageFileSystemReady = LittleFS.begin(false);
  enabled = true;
  clear();
}

void DisplayManager::clear() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  screenY = 0;
}

void DisplayManager::showBootScreen() {
  logLine("NOXON BOOT");
}

void DisplayManager::showHardwareTestScreen() {
  logLine("NOXON FRONTPANEL TEST");
  logLine("Buttons + IR + Volume");
  logLine("RFID + TFT + DFPlayer");
}

void DisplayManager::showNormalIdle() {
  if (!enabled) return;

  clear();
  tft.fillRect(0, 0, tft.width(), 34, ILI9341_DARKCYAN);
  tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 9);
  tft.println("HOERIBERT 2.0");

  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(18, 72);
  tft.println("KARTE");
  tft.setCursor(18, 112);
  tft.println("EINSTECKEN");

  tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setCursor(18, 176);
  tft.println("Album-Karten werden automatisch gespielt");
}

void DisplayManager::showFolderPlaying(uint8_t folder) {
  showFolderPlaying(folder, "");
}

void DisplayManager::showFolderPlaying(uint8_t folder, const String& title) {
  if (!enabled) return;

  clear();
  drawPlayingHeader();

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(18, 68);
  tft.println("FOLGE");

  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(6);
  tft.setCursor(28, 104);
  if (folder < 10) tft.print("0");
  tft.println(folder);

  if (title.length() > 0) {
    constexpr int titleX = 150;
    constexpr int titleY = 56;
    constexpr int titleLineHeight = 19;
    constexpr int titleMaxChars = 13;
    constexpr int titleMaxLines = 5;

    String remaining = title;
    remaining.trim();

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);

    for (int line = 0; line < titleMaxLines && remaining.length() > 0; line++) {
      int take = min(titleMaxChars, static_cast<int>(remaining.length()));

      if (remaining.length() > titleMaxChars) {
        int lastSpace = -1;
        for (int i = 0; i < titleMaxChars; i++) {
          if (remaining.charAt(i) == ' ') {
            lastSpace = i;
          }
        }

        if (lastSpace > 0) {
          take = lastSpace;
        }
      }

      String out = remaining.substring(0, take);
      out.trim();
      tft.setCursor(titleX, titleY + line * titleLineHeight);
      tft.print(out);

      remaining = remaining.substring(take);
      remaining.trim();
    }
  }

  tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setCursor(18, 176);
  tft.println("BTN_B: Anzeige an/aus");
}

bool DisplayManager::showFolderImage(uint8_t folder) {
  if (!enabled || !imageFileSystemReady) return false;

  String path = folderImagePath(folder);
  if (!LittleFS.exists(path)) {
    path = paddedFolderImagePath(folder);
  }

  if (!LittleFS.exists(path)) return false;

  clear();
  uint16_t imageWidth = 0;
  uint16_t imageHeight = 0;
  int32_t imageX = 0;

  if (TJpgDec.getFsJpgSize(&imageWidth, &imageHeight, path, LittleFS) == JDR_OK &&
      imageWidth < tft.width()) {
    imageX = (tft.width() - imageWidth) / 2;
  }

  TJpgDec.drawFsJpg(imageX, 0, path, LittleFS);
  return true;
}

void DisplayManager::showCardProblem(const String& text) {
  if (!enabled) return;

  clear();
  tft.fillRect(0, 0, tft.width(), 34, ILI9341_MAROON);
  tft.setTextColor(ILI9341_WHITE, ILI9341_MAROON);
  tft.setTextSize(2);
  tft.setCursor(10, 9);
  tft.println("KARTE");

  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(18, 74);
  tft.println("PROBLEM");

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(18, 132);
  tft.println(text);
}

void DisplayManager::showSleepTimerRemaining(unsigned long remainingSeconds) {
  if (!enabled) return;

  unsigned long minutes = remainingSeconds / 60;
  unsigned long seconds = remainingSeconds % 60;
  String text = String(minutes) + ":";
  if (seconds < 10) text += "0";
  text += String(seconds);

  constexpr int timerWidth = 126;
  constexpr int timerHeight = 34;
  constexpr int timerX = 194;
  constexpr int timerY = 164;
  constexpr int timerRightPadding = 4;
  constexpr int charWidth = 18;

  tft.fillRect(timerX, timerY, timerWidth, timerHeight, ILI9341_BLACK);
  tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(tft.width() - text.length() * charWidth - timerRightPadding, timerY + 6);
  tft.print(text);
}

void DisplayManager::clearSleepTimer() {
  if (!enabled) return;

  tft.fillRect(194, 164, 126, 34, ILI9341_BLACK);
}

void DisplayManager::logLine(const String& text) {
  Serial.println(text);

  if (!enabled) {
    return;
  }

  if (screenY > tft.height() - LINE_HEIGHT) {
    clear();
  }

  tft.setCursor(0, screenY);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.println(text);
  screenY += LINE_HEIGHT;
}

void DisplayManager::showError(const String& text) {
  logLine(text);
}

void DisplayManager::setEnabled(bool isEnabled) {
  enabled = isEnabled;
  digitalWrite(TFT_BACKLIGHT_PIN, enabled ? HIGH : LOW);

  if (!enabled) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    screenY = 0;
  }
}

bool DisplayManager::isEnabled() const {
  return enabled;
}

void DisplayManager::drawPlayingHeader() {
  tft.fillRect(0, 0, tft.width(), 34, ILI9341_DARKGREEN);
  tft.setTextColor(ILI9341_WHITE, ILI9341_DARKGREEN);
  tft.setTextSize(2);
  tft.setCursor(10, 9);
  tft.println("SPIELT");
}
