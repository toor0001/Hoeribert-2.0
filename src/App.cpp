#include "App.h"

#include <Arduino.h>

void App::begin() {
  Serial.begin(115200);
  delay(1000);

  bootButtons.begin();

  activeMode = selectBootMode();

  if (activeMode == Mode::CardProgramming) {
    cardProgrammingMode.begin();
  } else if (activeMode == Mode::HardwareTest) {
    hardwareTestMode.begin();
  } else {
    normalMode.begin();
  }
}

void App::update() {
  if (activeMode == Mode::CardProgramming) {
    cardProgrammingMode.update();
  } else if (activeMode == Mode::HardwareTest) {
    hardwareTestMode.update();
  } else {
    normalMode.update();
  }
}

App::Mode App::selectBootMode() const {
  // DISP has priority, including when both boot buttons are held.
  if (bootButtons.isHeld(ButtonBoard::BTN_B)) return Mode::CardProgramming;
  if (bootButtons.isHeld(ButtonBoard::BTN_J)) return Mode::HardwareTest;
  return Mode::Normal;
}
