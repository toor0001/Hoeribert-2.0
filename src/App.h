#pragma once

#include <Arduino.h>

#include "hardware/ButtonBoard.h"
#include "modes/HardwareTestMode.h"
#include "modes/NormalMode.h"

class App {
public:
  void begin();
  void update();

private:
  enum class Mode {
    Normal,
    HardwareTest,
  };

  bool shouldStartHardwareTest() const;

  Mode activeMode = Mode::Normal;
  ButtonBoard bootButtons;
  HardwareTestMode hardwareTestMode;
  NormalMode normalMode;
};
