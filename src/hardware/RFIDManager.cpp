#include "RFIDManager.h"

#include <SPI.h>

namespace {

constexpr byte BOOKMARK_MAGIC_0 = 'B';
constexpr byte BOOKMARK_MAGIC_1 = 'M';
constexpr byte BOOKMARK_VERSION = 1;

} // namespace

void RFIDManager::begin() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);

  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    rfidKey.keyByte[i] = 0xFF;
  }
}

bool RFIDManager::update() {
  clearDebugLines();
  lastError = "";
  lastRawData = "";
  lastRawDataAvailable = false;
  lastTonuinoCard = TonuinoCardData{};
  cardPresent = false;

  if (!selectCard()) return false;

  cardPresent = true;
  String uid = uidToString(&rfid.uid);
  unsigned long now = millis();

  bool shouldReport = uid != lastReportedUid || now - lastUidTime > 2000;

  lastUid = uid;
  lastCardType = String(rfid.PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak)));

  if (!shouldReport) {
    finishCard();
    return false;
  }

  lastReportedUid = uid;
  lastUidTime = now;

  if (readTonuinoRawData(lastRawBytes)) {
    lastTonuinoCard = decodeTonuinoCard(lastRawBytes);
  }

  finishCard();
  return true;
}

bool RFIDManager::isCardPresent() const {
  return cardPresent;
}

TonuinoCardData RFIDManager::readTonuinoCard() const {
  return lastTonuinoCard;
}

bool RFIDManager::writeBookmark(const CardBookmark& bookmark) {
  clearDebugLines();
  lastError = "";

  if (!bookmark.valid || bookmark.folder == 0 || bookmark.track == 0) {
    lastError = "Bookmark ungueltig";
    return false;
  }

  byte data[BUFFER_LENGTH] = {};

  if (!selectCard()) {
    lastError = "Keine Karte zum Schreiben";
    return false;
  }

  bool ok = readTonuinoRawData(data);
  if (ok) {
    TonuinoCardData card = decodeTonuinoCard(data);
    if (!card.valid || card.folder != bookmark.folder) {
      lastError = "Bookmark passt nicht zur Karte";
      ok = false;
    } else {
      encodeBookmark(data, bookmark);
      ok = writeTonuinoRawData(data);
    }
  }

  finishCard();
  return ok;
}

bool RFIDManager::clearBookmark() {
  clearDebugLines();
  lastError = "";

  byte data[BUFFER_LENGTH] = {};

  if (!selectCard()) {
    lastError = "Keine Karte zum Schreiben";
    return false;
  }

  bool ok = readTonuinoRawData(data);
  if (ok) {
    clearBookmarkBytes(data);
    ok = writeTonuinoRawData(data);
  }

  finishCard();
  return ok;
}

bool RFIDManager::selectCard() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    return true;
  }

  byte atqa[2];
  byte atqaSize = sizeof(atqa);
  MFRC522::StatusCode status = rfid.PICC_WakeupA(atqa, &atqaSize);
  if (status != MFRC522::STATUS_OK) {
    return false;
  }

  return rfid.PICC_ReadCardSerial();
}

bool RFIDManager::readTonuinoRawData(byte* data) {
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  // ===== MIFARE CLASSIC =====
  if (
    piccType == MFRC522::PICC_TYPE_MIFARE_1K ||
    piccType == MFRC522::PICC_TYPE_MIFARE_4K
  ) {
    byte blockAddr = 4;
    byte trailerBlock = 7;
    byte size = BUFFER_LENGTH;

    if (!authenticateClassicBlock(blockAddr, trailerBlock)) {
      return false;
    }

    MFRC522::StatusCode status = rfid.MIFARE_Read(blockAddr, data, &size);

    if (status != MFRC522::STATUS_OK) {
      lastError = String(rfid.GetStatusCodeName(status));
      addDebugLine("[RFID] Classic Read Fehler");
      addDebugLine(lastError);
      return false;
    }

    memcpy(lastRawBytes, data, BUFFER_LENGTH);
    lastRawData = bytesToHexLine(data, RAW_DATA_LENGTH);
    lastRawDataAvailable = true;
    return true;
  }

  // ===== ULTRALIGHT =====
  if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte size = BUFFER_LENGTH;
    MFRC522::StatusCode status = rfid.MIFARE_Read(8, data, &size);

    if (status == MFRC522::STATUS_OK) {
      memcpy(lastRawBytes, data, BUFFER_LENGTH);
      lastRawData = bytesToHexLine(data, RAW_DATA_LENGTH);
      lastRawDataAvailable = true;
      addDebugLine("[RFID] UL Pages 8-11 gelesen");
      return true;
    }

    lastError = String(rfid.GetStatusCodeName(status));
    addDebugLine("[RFID] UL Pages 8-11 Fehler");
    addDebugLine(lastError);
    addDebugLine("[RFID] UL Dump start");

    for (byte page = 0; page <= 44; page += 4) {
      byte buffer[BUFFER_LENGTH];
      byte size = sizeof(buffer);

      MFRC522::StatusCode status = rfid.MIFARE_Read(page, buffer, &size);

      if (status != MFRC522::STATUS_OK) {
        lastError = String(rfid.GetStatusCodeName(status));
        addDebugLine("[UL] p" + String(page) + " fail");
        addDebugLine(lastError);
        continue;
      }

      addDebugLine("[UL] p" + String(page) + ": " + bytesToHexLine(buffer, RAW_DATA_LENGTH));
    }

    addDebugLine("[RFID] UL Dump end");
    return false;
  }

  lastError = "Unsupported Tag Type";
  addDebugLine("[RFID] Unsupported Tag Type");
  return false;
}

bool RFIDManager::writeTonuinoRawData(const byte* data) {
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  if (
    piccType == MFRC522::PICC_TYPE_MIFARE_1K ||
    piccType == MFRC522::PICC_TYPE_MIFARE_4K
  ) {
    byte blockAddr = 4;
    byte trailerBlock = 7;

    if (!authenticateClassicBlock(blockAddr, trailerBlock)) {
      return false;
    }

    byte block[RAW_DATA_LENGTH];
    memcpy(block, data, RAW_DATA_LENGTH);
    MFRC522::StatusCode status = rfid.MIFARE_Write(blockAddr, block, RAW_DATA_LENGTH);

    if (status != MFRC522::STATUS_OK) {
      lastError = String(rfid.GetStatusCodeName(status));
      addDebugLine("[RFID] Classic Write Fehler");
      addDebugLine(lastError);
      return false;
    }

    addDebugLine("[RFID] Classic Write OK");
    return true;
  }

  if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
    for (byte page = 10; page <= 11; page++) {
      byte pageData[4];
      memcpy(pageData, data + (page - 8) * 4, sizeof(pageData));

      MFRC522::StatusCode status = rfid.MIFARE_Write(page, pageData, sizeof(pageData));
      if (status != MFRC522::STATUS_OK) {
        lastError = String(rfid.GetStatusCodeName(status));
        addDebugLine("[RFID] UL Write Fehler p" + String(page));
        addDebugLine(lastError);
        return false;
      }
    }

    addDebugLine("[RFID] UL Write OK");
    return true;
  }

  lastError = "Unsupported Tag Type";
  addDebugLine("[RFID] Unsupported Tag Type");
  return false;
}

bool RFIDManager::authenticateClassicBlock(byte blockAddr, byte trailerBlock) {
  struct AuthAttempt {
    byte command;
    byte block;
    const char* label;
  };

  AuthAttempt attempts[] = {
    {MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, "Key A Trailer 7"},
    {MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, "Key A Block 4"},
    {MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, "Key B Trailer 7"},
    {MFRC522::PICC_CMD_MF_AUTH_KEY_B, blockAddr, "Key B Block 4"},
  };

  for (const auto& attempt : attempts) {
    MFRC522::StatusCode status = rfid.PCD_Authenticate(
      attempt.command,
      attempt.block,
      &rfidKey,
      &(rfid.uid)
    );

    if (status == MFRC522::STATUS_OK) {
      addDebugLine("[RFID] Classic Auth OK: " + String(attempt.label));
      return true;
    }

    lastError = String(rfid.GetStatusCodeName(status));
    rfid.PCD_StopCrypto1();
  }

  addDebugLine("[RFID] Classic Auth Fehler");
  addDebugLine(lastError);
  return false;
}

TonuinoCardData RFIDManager::decodeTonuinoCard(const byte* data) const {
  TonuinoCardData card;
  card.uid = lastUid;
  card.cardType = lastCardType;

  bool validCookie =
    data[0] == 0x13 &&
    data[1] == 0x37 &&
    data[2] == 0xB3 &&
    data[3] == 0x47;

  if (!validCookie) {
    return card;
  }

  card.version = data[4];
  card.folder = data[5];
  card.mode = data[6];
  card.special = data[7];
  card.special2 = data[8];
  CardBookmark bookmark = decodeBookmark(data, card.folder);
  card.bookmarkValid = bookmark.valid;
  card.bookmarkFolder = bookmark.folder;
  card.bookmarkTrack = bookmark.track;
  card.bookmarkSeconds = bookmark.seconds;
  card.valid = card.folder != 0 && card.mode != 0;

  return card;
}

CardBookmark RFIDManager::decodeBookmark(const byte* data, uint8_t expectedFolder) const {
  CardBookmark bookmark;

  if (data[9] != BOOKMARK_MAGIC_0 ||
      data[10] != BOOKMARK_MAGIC_1 ||
      data[11] != BOOKMARK_VERSION ||
      data[15] != bookmarkChecksum(data)) {
    return bookmark;
  }

  uint8_t track = data[12];
  uint16_t seconds = (static_cast<uint16_t>(data[13]) << 8) | data[14];

  if (expectedFolder == 0 || track == 0) {
    return bookmark;
  }

  bookmark.valid = true;
  bookmark.folder = expectedFolder;
  bookmark.track = track;
  bookmark.seconds = seconds;
  return bookmark;
}

void RFIDManager::encodeBookmark(byte* data, const CardBookmark& bookmark) const {
  data[9] = BOOKMARK_MAGIC_0;
  data[10] = BOOKMARK_MAGIC_1;
  data[11] = BOOKMARK_VERSION;
  data[12] = bookmark.track;
  data[13] = static_cast<byte>(bookmark.seconds >> 8);
  data[14] = static_cast<byte>(bookmark.seconds);
  data[15] = bookmarkChecksum(data);
}

void RFIDManager::clearBookmarkBytes(byte* data) const {
  for (int i = 9; i < RAW_DATA_LENGTH; i++) {
    data[i] = 0;
  }
}

byte RFIDManager::bookmarkChecksum(const byte* data) const {
  byte checksum = 0xA5;

  for (int i = 9; i <= 14; i++) {
    checksum ^= data[i];
  }

  return checksum;
}

String RFIDManager::getLastUid() const {
  return lastUid;
}

String RFIDManager::getLastCardType() const {
  return lastCardType;
}

String RFIDManager::getLastRawData() const {
  return lastRawData;
}

String RFIDManager::getLastError() const {
  return lastError;
}

bool RFIDManager::hasLastRawData() const {
  return lastRawDataAvailable;
}

void RFIDManager::copyLastRawData(byte* data, size_t maxLength) const {
  size_t length = min(maxLength, static_cast<size_t>(BUFFER_LENGTH));
  memcpy(data, lastRawBytes, length);
}

int RFIDManager::getDebugLineCount() const {
  return debugLineCount;
}

String RFIDManager::getDebugLine(int index) const {
  if (index < 0 || index >= debugLineCount) {
    return "";
  }

  return debugLines[index];
}

String RFIDManager::uidToString(MFRC522::Uid* uid) const {
  String s = "";

  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) s += "0";
    s += String(uid->uidByte[i], HEX);
    if (i < uid->size - 1) s += ":";
  }

  s.toUpperCase();
  return s;
}

void RFIDManager::clearDebugLines() {
  debugLineCount = 0;
}

void RFIDManager::addDebugLine(const String& line) {
  if (debugLineCount >= MAX_DEBUG_LINES) {
    return;
  }

  debugLines[debugLineCount] = line;
  debugLineCount++;
}

String RFIDManager::bytesToHexLine(const byte* data, int length) const {
  String line = "";

  for (int i = 0; i < length; i++) {
    if (data[i] < 0x10) line += "0";
    line += String(data[i], HEX);
    line += " ";
  }

  line.toUpperCase();
  return line;
}

void RFIDManager::finishCard() {
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
