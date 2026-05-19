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

class AudioPlayer {
public:
  bool begin();
  void update();
  bool isReady() const;
  bool isPlayingNow() const;
  void playFolder(uint8_t folder);
  void playFolderTrack(uint8_t folder, uint8_t track);
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

  HardwareSerial dfSerial{2};
  DFRobotDFPlayerMini dfPlayer;
  bool ready = false;
  bool playing = false;
  bool folderPlaybackActive = false;
  uint8_t currentFolder = 0;
  uint8_t currentTrack = 0;
  int tracksInFolder = 0;
  String statusText = "DFPlayer nicht gestartet";

  void startFolderTrack(uint8_t folder, uint8_t track);
  void handlePlayFinished();
};
