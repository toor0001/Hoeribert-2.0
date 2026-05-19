#pragma once

#include <Arduino.h>

struct IRReading {
  String protocol;
  uint32_t command = 0;
  uint32_t raw = 0;
};

class IRManager {
public:
  void begin();
  bool update();
  IRReading getLastReading() const;

private:
  static constexpr uint8_t IR_PIN = 27;

  IRReading lastReading;
};
