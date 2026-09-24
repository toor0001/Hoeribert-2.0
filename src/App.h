#pragma once

#include <Arduino.h>

#include "hardware/ButtonBoard.h"
#include "modes/HardwareTestMode.h"
#include "modes/CardProgrammingMode.h"
#include "modes/NormalMode.h"

class App {
public:
  void begin();
  void update();

private:
  enum class Mode {
    Normal,
    CardProgramming,
    HardwareTest,
  };

  Mode selectBootMode() const;

  Mode activeMode = Mode::Normal;
  ButtonBoard bootButtons;
  CardProgrammingMode cardProgrammingMode;
  HardwareTestMode hardwareTestMode;
  NormalMode normalMode;
};
