#include "App.h"

#include <Arduino.h>

void App::begin() {
  Serial.begin(115200);
  delay(1000);

  bootButtons.begin();

  activeMode = shouldStartHardwareTest() ? Mode::HardwareTest : Mode::Normal;

  if (activeMode == Mode::HardwareTest) {
    hardwareTestMode.begin();
  } else {
    normalMode.begin();
  }
}

void App::update() {
  if (activeMode == Mode::HardwareTest) {
    hardwareTestMode.update();
  } else {
    normalMode.update();
  }
}

bool App::shouldStartHardwareTest() const {
  return bootButtons.isHeld(ButtonBoard::BTN_J);
}
