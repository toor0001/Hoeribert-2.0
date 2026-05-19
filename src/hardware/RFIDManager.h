#pragma once

#include <Arduino.h>
#include <MFRC522.h>

struct TonuinoCardData {
  bool valid = false;
  uint8_t version = 0;
  uint8_t folder = 0;
  uint8_t mode = 0;
  uint8_t special = 0;
  uint8_t special2 = 0;
  String uid = "";
  String cardType = "";
};

class RFIDManager {
public:
  void begin();
  bool update();
  bool isCardPresent() const;
  TonuinoCardData readTonuinoCard() const;
  String getLastUid() const;
  String getLastCardType() const;
  String getLastRawData() const;
  String getLastError() const;
  bool hasLastRawData() const;
  void copyLastRawData(byte* data, size_t maxLength) const;
  int getDebugLineCount() const;
  String getDebugLine(int index) const;

private:
  static constexpr uint8_t RFID_SS_PIN   = 21;
  static constexpr uint8_t RFID_RST_PIN  = 22;
  static constexpr uint8_t RFID_SCK_PIN  = 14;
  static constexpr uint8_t RFID_MISO_PIN = 23;
  static constexpr uint8_t RFID_MOSI_PIN = 13;
  static constexpr int RAW_DATA_LENGTH = 16;
  static constexpr int BUFFER_LENGTH = 18;
  static constexpr int MAX_DEBUG_LINES = 16;

  bool readTonuinoRawData(byte* data);
  bool authenticateClassicBlock(byte blockAddr, byte trailerBlock);
  TonuinoCardData decodeTonuinoCard(const byte* data) const;
  String uidToString(MFRC522::Uid* uid) const;
  void clearDebugLines();
  void addDebugLine(const String& line);
  String bytesToHexLine(const byte* data, int length) const;
  void finishCard();

  MFRC522 rfid{RFID_SS_PIN, RFID_RST_PIN};
  MFRC522::MIFARE_Key rfidKey;
  String lastUid = "";
  String lastReportedUid = "";
  String lastCardType = "";
  String lastRawData = "";
  String lastError = "";
  unsigned long lastUidTime = 0;
  bool cardPresent = false;
  bool lastRawDataAvailable = false;
  TonuinoCardData lastTonuinoCard;
  byte lastRawBytes[BUFFER_LENGTH] = {};
  String debugLines[MAX_DEBUG_LINES];
  int debugLineCount = 0;
};
