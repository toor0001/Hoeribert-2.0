#include "ButtonBoard.h"

namespace {

struct ButtonMap {
  const char* name;
  uint16_t mask;
};

constexpr ButtonMap BUTTONS[] = {
  {"A", ButtonBoard::BTN_A}, {"B", ButtonBoard::BTN_B},
  {"C", ButtonBoard::BTN_C}, {"D", ButtonBoard::BTN_D},
  {"E", ButtonBoard::BTN_E}, {"F", ButtonBoard::BTN_F},
  {"G", ButtonBoard::BTN_G}, {"H", ButtonBoard::BTN_H},
  {"I", ButtonBoard::BTN_I}, {"J", ButtonBoard::BTN_J},
  {"K", ButtonBoard::BTN_K}, {"L", ButtonBoard::BTN_L},
  {"M", ButtonBoard::BTN_M}, {"N", ButtonBoard::BTN_N},
};

} // namespace

void ButtonBoard::begin() {
  pinMode(DATA_PIN, INPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);

  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(LOAD_PIN, HIGH);

  currentState = readRaw();
  lastState = currentState;
  newlyPressed = 0;
  lastButtonName = "";
}

void ButtonBoard::update() {
  currentState = readRaw();
  newlyPressed = currentState & ~lastState;
  lastButtonName = namesForMask(newlyPressed);
  lastState = currentState;
}

bool ButtonBoard::wasPressed(uint16_t mask) const {
  return (newlyPressed & mask) != 0;
}

bool ButtonBoard::isHeld(uint16_t mask) const {
  return (currentState & mask) != 0;
}

String ButtonBoard::getLastButtonName() const {
  return lastButtonName;
}

uint16_t ButtonBoard::getCurrentState() const {
  return currentState;
}

uint16_t ButtonBoard::getNewlyPressed() const {
  return newlyPressed;
}

uint16_t ButtonBoard::readRaw() const {
  uint16_t value = 0;

  digitalWrite(LOAD_PIN, LOW);
  delayMicroseconds(20);
  digitalWrite(LOAD_PIN, HIGH);
  delayMicroseconds(20);

  for (int i = 0; i < 16; i++) {
    value <<= 1;

    if (digitalRead(DATA_PIN)) {
      value |= 1;
    }

    digitalWrite(CLOCK_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(CLOCK_PIN, LOW);
    delayMicroseconds(20);
  }

  return value;
}

String ButtonBoard::namesForMask(uint16_t mask) const {
  String names = "";
  bool first = true;

  for (const auto& button : BUTTONS) {
    if (mask & button.mask) {
      if (!first) names += ", ";
      names += button.name;
      first = false;
    }
  }

  return names;
}
