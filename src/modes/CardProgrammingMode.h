#pragma once

#include "hardware/ButtonBoard.h"
#include "hardware/DisplayManager.h"
#include "hardware/RFIDManager.h"

class CardProgrammingMode {
public:
  void begin();
  void update();

private:
  enum class State { Waiting, Removing };
  ButtonBoard buttons;
  DisplayManager display;
  RFIDManager rfid;
  State state = State::Waiting;
  uint16_t selectedEpisode = 1;
  uint8_t emptyReadings = 0;
  bool writeSucceeded = false;
  bool removalShown = false;
  unsigned long lastPollAt = 0;
  unsigned long resultAt = 0;
  uint16_t rawNavigation = 0;
  uint16_t stableNavigation = 0;
  unsigned long navigationChangedAt = 0;
  unsigned long lastStepAt = 0;
  bool repeating = false;

  void showWaiting();
  void updateNavigation(unsigned long now);
  void stepEpisode(uint16_t navigation);
};
