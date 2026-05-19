#include "IRManager.h"

#include <IRremote.hpp>

void IRManager::begin() {
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
}

bool IRManager::update() {
  if (!IrReceiver.decode()) {
    return false;
  }

  lastReading.protocol = String(getProtocolString(IrReceiver.decodedIRData.protocol));
  lastReading.command = IrReceiver.decodedIRData.command;
  lastReading.raw = IrReceiver.decodedIRData.decodedRawData;

  IrReceiver.resume();
  return true;
}

IRReading IRManager::getLastReading() const {
  return lastReading;
}
