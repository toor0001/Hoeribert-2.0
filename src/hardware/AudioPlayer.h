#pragma once

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

struct AudioPlayerStatus {
  int state = 0;
  int volume = 0;
  int currentFile = 0;
  int fileCount = 0;
};

struct PlaybackPosition {
  bool valid = false;
  uint8_t folder = 0;
  uint8_t track = 0;
  uint16_t seconds = 0;
};

class AudioPlayer {
public:
  bool begin();
  void update();
  bool isReady() const;
  bool isPlayingNow() const;
  void playFolder(uint8_t folder, const char* source = "OTHER");
  void playFolderTrack(uint8_t folder, uint8_t track, const char* source = "OTHER");
  PlaybackPosition getPlaybackPosition() const;
  bool consumeFolderFinished();
  void stop();
  void pause();
  void resume();
  void next();
  void previous();
  void setVolume(uint8_t volume);
  AudioPlayerStatus readStatus();
  String getStatusText() const;

private:
  static constexpr uint8_t DF_RX_PIN = 16;
  static constexpr uint8_t DF_TX_PIN = 17;
  static constexpr unsigned long DUPLICATE_FINISH_WINDOW_MS = 500;

  HardwareSerial dfSerial{2};
  DFRobotDFPlayerMini dfPlayer;
  bool ready = false;
  bool playing = false;
  bool folderPlaybackActive = false;
  bool folderFinished = false;
  uint8_t currentFolder = 0;
  uint8_t currentTrack = 0;
  int tracksInFolder = 0;
  unsigned long trackStartedAt = 0;
  uint16_t trackElapsedBeforePause = 0;
  String statusText = "DFPlayer nicht gestartet";
  uint32_t playbackGeneration = 0;
  unsigned long lastPlayCommandAt = 0;
  uint8_t lastPlayFolder = 0;
  uint8_t lastPlayTrack = 0;
  const char* lastPlaySource = "NONE";
  bool hasAcceptedFinish = false;
  uint16_t lastAcceptedFinishValue = 0;
  unsigned long lastAcceptedFinishAt = 0;

  void startFolderTrack(uint8_t folder, uint8_t track, const char* source);
  uint16_t currentTrackSeconds() const;
  void handlePlayFinished();
  const char* eventTypeName(uint8_t type) const;
  void resetFinishHistory();
};
