#pragma once

#include <Arduino.h>

class ButtonBoard {
public:
  static constexpr uint16_t BTN_A = 0x0200;
  static constexpr uint16_t BTN_B = 0x0100;
  static constexpr uint16_t BTN_C = 0x0008;
  static constexpr uint16_t BTN_D = 0x0004;
  static constexpr uint16_t BTN_E = 0x0002;
  static constexpr uint16_t BTN_F = 0x0010;
  static constexpr uint16_t BTN_G = 0x0400;
  static constexpr uint16_t BTN_H = 0x0020;
  static constexpr uint16_t BTN_I = 0x0080;
  static constexpr uint16_t BTN_J = 0x0040;
  static constexpr uint16_t BTN_K = 0x4000;
  static constexpr uint16_t BTN_L = 0x2000;
  static constexpr uint16_t BTN_M = 0x1000;
  static constexpr uint16_t BTN_N = 0x8000;

  void begin();
  void update();
  bool wasPressed(uint16_t mask) const;
  bool isHeld(uint16_t mask) const;
  String getLastButtonName() const;
  uint16_t getCurrentState() const;
  uint16_t getNewlyPressed() const;
  uint16_t readRaw() const;
  String namesForMask(uint16_t mask) const;

private:
  static constexpr uint8_t DATA_PIN  = 19;
  static constexpr uint8_t CLOCK_PIN = 18;
  static constexpr uint8_t LOAD_PIN  = 5;

  uint16_t currentState = 0;
  uint16_t lastState = 0;
  uint16_t newlyPressed = 0;
  String lastButtonName = "";
};
