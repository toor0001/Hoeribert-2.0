#include "NormalMode.h"

#include "hardware/AudioPlayer.h"
#include "hardware/ButtonBoard.h"
#include "hardware/DisplayManager.h"
#include "hardware/RFIDManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <math.h>

namespace {

constexpr uint8_t VOL_PIN = 34;
constexpr int VOL_RAW_MIN = 40;
constexpr int VOL_RAW_MAX = 4050;
constexpr int VOL_MAX = 24;

const char* folderTitle(uint8_t folder) {
  switch (folder) {
      case 1: return "Der Super-Papagei";
      case 2: return "Der Phantomsee";
      case 3: return "Der Karpatenhund";
      case 4: return "Die schwarze Katze";
      case 5: return "Der Fluch des Rubins";
      case 6: return "Der sprechende Totenkopf";
      case 7: return "Der unheimliche Drache";
      case 8: return "Der gruene Geist";
      case 9: return "Die raetselhaften Bilder";
      case 10: return "Die fluesternde Mumie";
      case 11: return "Das Gespensterschloss";
      case 12: return "Der seltsame Wecker";
      case 13: return "Der lachende Schatten";
      case 14: return "Das Bergmonster";
      case 15: return "Der rasende Loewe";
      case 16: return "Der Zauberspiegel";
      case 17: return "Die gefaehrliche Erbschaft";
      case 18: return "Die Geisterinsel";
      case 19: return "Der Teufelsberg";
      case 20: return "Die flammende Spur";
      case 21: return "Der tanzende Teufel";
      case 22: return "Der verschwundene Schatz";
      case 23: return "Das Aztekenschwert";
      case 24: return "Die silberne Spinne";
      case 25: return "Die singende Schlange";
      case 26: return "Die Silbermine";
      case 27: return "Der magische Kreis";
      case 28: return "Der Doppelgaenger";
      case 29: return "Die Originalmusik";
      case 30: return "Das Riff der Haie";
      case 31: return "Das Narbengesicht";
      case 32: return "Der Ameisenmensch";
      case 33: return "Die bedrohte Ranch";
      case 34: return "Der rote Pirat";
      case 35: return "Der Hoehlenmensch";
      case 36: return "Der Super-Wal";
      case 37: return "Der heimliche Hehler";
      case 38: return "Der unsichtbare Gegner";
      case 39: return "Die Perlenvoegel";
      case 40: return "Der Automarder";
      case 41: return "Das Volk der Winde";
      case 42: return "Der weinende Sarg";
      case 43: return "Der hoellische Werwolf";
      case 44: return "Der gestohlene Preis";
      case 45: return "Das Gold der Wikinger";
      case 46: return "Der schrullige Millionaer";
      case 47: return "Der giftige Gockel";
      case 48: return "Die gefaehrlichen Faesser";
      case 49: return "Die Comic-Diebe";
      case 50: return "Der verschwundene Filmstar";
      case 51: return "Der riskante Ritt";
      case 52: return "Die Musikpiraten";
      case 53: return "Die Automafia";
      case 54: return "Gefahr im Verzug";
      case 55: return "Gekaufte Spieler";
      case 56: return "Angriff der Computer-Viren";
      case 57: return "Tatort Zirkus";
      case 58: return "Der verrueckte Maler";
      case 59: return "Giftiges Wasser";
      case 60: return "Dopingmixer";
      case 61: return "Die Rache des Tigers";
      case 62: return "Spuk im Hotel";
      case 63: return "Fussball-Gangster";
      case 64: return "Geisterstadt";
      case 65: return "Diamanten- schmuggel";
      case 66: return "Die Schattenmaenner";
      case 67: return "Das Geheimnis der Saerge";
      case 68: return "Der Schatz im Bergsee";
      case 69: return "Spaete Rache";
      case 70: return "Schuesse aus dem Dunkel";
      case 71: return "Die verschwundene Seglerin";
      case 72: return "Dreckiger Deal";
      case 73: return "Poltergeist";
      case 74: return "Das brennende Schwert";
      case 75: return "Die Spur des Raben";
      case 76: return "Stimmen aus dem Nichts";
      case 77: return "Pistenteufel";
      case 78: return "Das leere Grab";
      case 79: return "Im Bann des Voodoo";
      case 80: return "Geheimakte UFO";
      case 81: return "Verdeckte Fouls";
      case 82: return "Die Karten des Boesen";
      case 83: return "Meuterei auf hoher See";
      case 84: return "Die Musik des Teufels";
      case 85: return "Feuerturm";
      case 86: return "Nacht in Angst";
      case 87: return "Wolfsgesicht";
      case 88: return "Vampir im Internet";
      case 89: return "Toedliche Spur";
      case 90: return "Der Feuerteufel";
      case 91: return "Labyrinth der Goetter";
      case 92: return "Todesflug";
      case 93: return "Das Geisterschiff";
      case 94: return "Das schwarze Monster";
      case 95: return "Botschaft von Geisterhand";
      case 96: return "Der rote Raecher";
      case 97: return "Insekten- stachel";
      case 98: return "Tal des Schreckens";
      case 99: return "Rufmord";
      default: return "Unbekannte Folge";
  }
}

DisplayManager display;
ButtonBoard buttonBoard;
RFIDManager rfidManager;
AudioPlayer audioPlayer;

uint8_t currentFolder = 0;
int lastVolume = -1;
int filteredVolumeRaw = -1;
bool preferCoverDisplay = true;
String activeCardUid = "";
unsigned long sleepTimerEndsAt = 0;
unsigned long lastSleepTimerDrawnSecond = 0;
unsigned long lastButtonAPressedAt = 0;
constexpr unsigned long DOUBLE_CLICK_MS = 450;

int readAverageRaw() {
  long sum = 0;
  constexpr int SAMPLES = 8;

  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(VOL_PIN);
    delayMicroseconds(500);
  }

  return sum / SAMPLES;
}

int rawToVolume(int raw) {
  raw = constrain(raw, 0, 4095);

  if (raw <= VOL_RAW_MIN) return 0;
  if (raw >= VOL_RAW_MAX) return VOL_MAX;

  float normalized = static_cast<float>(raw - VOL_RAW_MIN) / static_cast<float>(VOL_RAW_MAX - VOL_RAW_MIN);
  float curved = powf(normalized, 0.75f);
  int volume = static_cast<int>(curved * VOL_MAX + 0.5f);
  return constrain(volume, 0, VOL_MAX);
}

void applyVolume() {
  int raw = readAverageRaw();

  if (filteredVolumeRaw < 0) {
    filteredVolumeRaw = raw;
  } else {
    filteredVolumeRaw = (filteredVolumeRaw * 3 + raw) / 4;
  }

  int volume = rawToVolume(filteredVolumeRaw);

  if (volume != lastVolume) {
    lastVolume = volume;
    audioPlayer.setVolume(volume);
    Serial.println("[NORMAL] Volume " + String(volume) + "/" + String(VOL_MAX) +
                   " RAW=" + String(raw) + " FILT=" + String(filteredVolumeRaw));
  }
}

void disableWifi() {
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  Serial.println("[NORMAL] WLAN aus");
}

void showCurrentFolderDisplay() {
  if (preferCoverDisplay && display.showFolderImage(currentFolder)) {
    return;
  }

  display.showFolderPlaying(currentFolder, folderTitle(currentFolder));
}

void redrawNormalDisplay() {
  if (currentFolder > 0) {
    showCurrentFolderDisplay();
  } else {
    display.showNormalIdle();
  }

  if (sleepTimerEndsAt > 0) {
    unsigned long remainingSeconds = max(1UL, (sleepTimerEndsAt - millis() + 999) / 1000);
    display.showSleepTimerRemaining(remainingSeconds);
  }
}

void addSleepTimerMinutes(uint8_t minutes) {
  if (!audioPlayer.isPlayingNow()) {
    Serial.println("[NORMAL] Sleep Timer ignoriert: keine Wiedergabe");
    return;
  }

  unsigned long addMs = static_cast<unsigned long>(minutes) * 60UL * 1000UL;

  if (sleepTimerEndsAt == 0 || static_cast<long>(sleepTimerEndsAt - millis()) <= 0) {
    sleepTimerEndsAt = millis() + addMs;
  } else {
    sleepTimerEndsAt += addMs;
  }

  lastSleepTimerDrawnSecond = 0;
  unsigned long remainingSeconds = max(1UL, (sleepTimerEndsAt - millis() + 999) / 1000);
  display.showSleepTimerRemaining(remainingSeconds);
  Serial.println("[NORMAL] Sleep Timer +" + String(minutes) + " min");
}

void clearSleepTimer() {
  sleepTimerEndsAt = 0;
  lastSleepTimerDrawnSecond = 0;
  display.clearSleepTimer();
  Serial.println("[NORMAL] Sleep Timer geloescht");
}

void updateSleepTimer() {
  if (sleepTimerEndsAt == 0) {
    return;
  }

  long remainingMs = static_cast<long>(sleepTimerEndsAt - millis());

  if (remainingMs <= 0) {
    sleepTimerEndsAt = 0;
    lastSleepTimerDrawnSecond = 0;
    audioPlayer.stop();
    currentFolder = 0;
    activeCardUid = "";
    display.showNormalIdle();
    Serial.println("[NORMAL] Sleep Timer abgelaufen -> Stop");
    return;
  }

  unsigned long remainingSeconds = (static_cast<unsigned long>(remainingMs) + 999) / 1000;

  if (remainingSeconds != lastSleepTimerDrawnSecond) {
    lastSleepTimerDrawnSecond = remainingSeconds;
    display.showSleepTimerRemaining(remainingSeconds);
  }
}

void handleButtons() {
  buttonBoard.update();
  uint16_t newlyPressed = buttonBoard.getNewlyPressed();

  if (newlyPressed & ButtonBoard::BTN_B) {
    bool nextEnabled = !display.isEnabled();
    display.setEnabled(nextEnabled);
    Serial.println(nextEnabled ? "[NORMAL] Display an" : "[NORMAL] Display aus");

    if (nextEnabled) {
      redrawNormalDisplay();
    }
  }

  if (newlyPressed & ButtonBoard::BTN_A) {
    unsigned long now = millis();

    if (lastButtonAPressedAt != 0 && now - lastButtonAPressedAt <= DOUBLE_CLICK_MS) {
      clearSleepTimer();
      lastButtonAPressedAt = 0;
    } else {
      addSleepTimerMinutes(10);
      lastButtonAPressedAt = now;
    }
  }

  if (newlyPressed & ButtonBoard::BTN_C) {
    preferCoverDisplay = !preferCoverDisplay;
    Serial.println(preferCoverDisplay ? "[NORMAL] Anzeige: Cover" : "[NORMAL] Anzeige: Ordnertext");
    redrawNormalDisplay();
  }

  if (newlyPressed & ButtonBoard::BTN_F) {
    audioPlayer.stop();
    currentFolder = 0;
    activeCardUid = "";
    display.showNormalIdle();
  }

  if (newlyPressed & ButtonBoard::BTN_G) {
    audioPlayer.previous();
  }

  if (newlyPressed & ButtonBoard::BTN_H) {
    if (audioPlayer.isPlayingNow()) {
      audioPlayer.pause();
    } else {
      audioPlayer.resume();
    }
  }

  if (newlyPressed & ButtonBoard::BTN_I) {
    audioPlayer.next();
  }
}

void handleRFID() {
  if (!rfidManager.update()) return;

  TonuinoCardData card = rfidManager.readTonuinoCard();

  if (!card.valid) {
    display.showCardProblem("NICHT LESBAR");
    Serial.println("[NORMAL] Karte nicht lesbar oder Cookie falsch");
    return;
  }

  if (card.mode != 2) {
    display.showCardProblem("MODUS " + String(card.mode));
    Serial.println("[NORMAL] Nicht unterstuetzter Modus: " + String(card.mode));
    return;
  }

  if (!audioPlayer.isReady()) {
    display.showCardProblem("DFPLAYER");
    Serial.println("[NORMAL] DFPlayer nicht bereit");
    return;
  }

  if (card.uid == activeCardUid) {
    return;
  }

  activeCardUid = card.uid;
  currentFolder = card.folder;
  audioPlayer.playFolder(currentFolder);
  showCurrentFolderDisplay();

  if (sleepTimerEndsAt > 0) {
    unsigned long remainingSeconds = max(1UL, (sleepTimerEndsAt - millis() + 999) / 1000);
    display.showSleepTimerRemaining(remainingSeconds);
  }

  Serial.println("[NORMAL] Spiele Ordner " + String(currentFolder));
}

} // namespace

void NormalMode::begin() {
  Serial.println("[NORMAL] Starte NormalMode");

  buttonBoard.begin();
  rfidManager.begin();
  disableWifi();
  analogReadResolution(12);
  analogSetPinAttenuation(VOL_PIN, ADC_11db);
  display.begin();
  display.showNormalIdle();

  display.showNormalIdle();

  if (audioPlayer.begin()) {
    Serial.println("[NORMAL] " + audioPlayer.getStatusText());
    int initialVolume = rawToVolume(readAverageRaw());
    lastVolume = initialVolume;
    audioPlayer.setVolume(initialVolume);
    Serial.println("[NORMAL] Startvolume " + String(initialVolume) + "/" + String(VOL_MAX));
  } else {
    Serial.println("[NORMAL] " + audioPlayer.getStatusText());
    display.showCardProblem("DFPLAYER");
  }
}

void NormalMode::update() {
  audioPlayer.update();
  handleButtons();
  applyVolume();
  handleRFID();
  updateSleepTimer();

  delay(20);
}
