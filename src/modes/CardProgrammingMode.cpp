#include "CardProgrammingMode.h"

void CardProgrammingMode::begin() {
  buttons.begin();
  rfid.begin();
  display.begin();
  selectedEpisode = 1;
  state = State::Waiting;
  emptyReadings = 0;
  lastPollAt = millis();
  rawNavigation = stableNavigation = buttons.getCurrentState() &
      (ButtonBoard::BTN_G | ButtonBoard::BTN_I);
  navigationChangedAt = lastStepAt = lastPollAt;
  repeating = false;
  Serial.println("[CARDPROG] Modus gestartet");
  showWaiting();
}

void CardProgrammingMode::showWaiting() {
  display.showCardProgrammingWaiting(selectedEpisode);
  const auto card = RFIDManager::makeCardDataForEpisode(selectedEpisode);
  Serial.printf("[CARDPROG] selected episode=%u virtual=%u folder=%u mode=%u special=%u special2=%u\n",
                selectedEpisode, card.mode == 8, card.folder, card.mode,
                card.special, card.special2);
}

void CardProgrammingMode::stepEpisode(uint16_t navigation) {
  if (navigation == ButtonBoard::BTN_I && selectedEpisode < 999) {
    ++selectedEpisode;
  } else if (navigation == ButtonBoard::BTN_G && selectedEpisode > 1) {
    --selectedEpisode;
  } else {
    return;
  }
  showWaiting();
}

void CardProgrammingMode::updateNavigation(unsigned long now) {
  buttons.update();
  const uint16_t navigation = buttons.getCurrentState() &
      (ButtonBoard::BTN_G | ButtonBoard::BTN_I);
  if (navigation != rawNavigation) {
    rawNavigation = navigation;
    navigationChangedAt = now;
  }
  // Debounce locally; leave button behavior in all other modes unchanged.
  if (state != State::Waiting) {
    stableNavigation = rawNavigation;
    lastStepAt = now;
    repeating = false;
    return;
  }
  if (now - navigationChangedAt < 25) return;
  if (stableNavigation != rawNavigation) {
    stableNavigation = rawNavigation;
    lastStepAt = now;
    repeating = false;
    stepEpisode(stableNavigation);
  } else if (stableNavigation == ButtonBoard::BTN_G ||
             stableNavigation == ButtonBoard::BTN_I) {
    if (now - lastStepAt >= (repeating ? 150UL : 500UL)) {
      lastStepAt = now;
      repeating = true;
      stepEpisode(stableNavigation);
    }
  }
}

void CardProgrammingMode::update() {
  unsigned long now = millis();
  updateNavigation(now);
  if (now - lastPollAt < 100) return;
  lastPollAt = now;

  if (state == State::Removing) {
    // Keep the result visible briefly even if the card is removed immediately.
    if (now - resultAt < 800) return;
    if (writeSucceeded && !removalShown) {
      display.showCardProgrammingRemoval(selectedEpisode);
      removalShown = true;
    }
    // Require three actual no-response probes, not just an elapsed delay.
    if (!rfid.isProgrammingFieldEmpty()) {
      emptyReadings = 0;
      return;
    }
    if (++emptyReadings < 3) return;
    emptyReadings = 0;
    if (writeSucceeded && selectedEpisode < 999) ++selectedEpisode;
    state = State::Waiting;
    showWaiting();
    return;
  }

  String error;
  // Synchronous write/verify: navigation cannot change the selected episode here.
  auto result = rfid.writeEpisodeCard(selectedEpisode, error, [this]() {
    display.showCardProgrammingWriting(selectedEpisode);
  });
  if (result == RFIDManager::WriteResult::NoCard) return;
  writeSucceeded = result == RFIDManager::WriteResult::Success;
  if (writeSucceeded) {
    display.showCardProgrammingSuccess(selectedEpisode);
    Serial.printf("[CARDPROG] write OK episode=%u\n", selectedEpisode);
  } else {
    display.showCardProgrammingError(selectedEpisode, error);
    Serial.printf("[CARDPROG] write ERROR episode=%u: %s\n", selectedEpisode, error.c_str());
  }
  emptyReadings = 0;
  removalShown = false;
  resultAt = millis();
  state = State::Removing;
  Serial.println("[CARDPROG] Warte auf Entfernen");
}
