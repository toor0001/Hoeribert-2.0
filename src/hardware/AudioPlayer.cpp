#include "AudioPlayer.h"

bool AudioPlayer::begin() {
  dfSerial.begin(9600, SERIAL_8N1, DF_RX_PIN, DF_TX_PIN);
  delay(500);

  ready = dfPlayer.begin(dfSerial);
  playing = false;
  folderPlaybackActive = false;
  currentFolder = 0;
  currentTrack = 0;
  tracksInFolder = 0;
  statusText = ready ? "DFPlayer bereit." : "DFPlayer NICHT gefunden!";

  return ready;
}

void AudioPlayer::update() {
  if (!ready) return;

  while (dfPlayer.available()) {
    uint8_t type = dfPlayer.readType();
    uint16_t value = dfPlayer.read();

    if (type == DFPlayerPlayFinished) {
      handlePlayFinished();
    } else if (type == DFPlayerError) {
      statusText = "DFPlayer Fehler " + String(value);
      if (folderPlaybackActive) {
        playing = false;
        folderPlaybackActive = false;
      }
    }
  }
}

bool AudioPlayer::isReady() const {
  return ready;
}

bool AudioPlayer::isPlayingNow() const {
  return playing;
}

void AudioPlayer::playFolder(uint8_t folder) {
  if (!ready) return;

  tracksInFolder = dfPlayer.readFileCountsInFolder(folder);
  if (tracksInFolder < 1) {
    tracksInFolder = 0;
  }

  folderPlaybackActive = true;
  startFolderTrack(folder, 1);
}

void AudioPlayer::playFolderTrack(uint8_t folder, uint8_t track) {
  if (!ready) return;

  tracksInFolder = dfPlayer.readFileCountsInFolder(folder);
  if (tracksInFolder < 1) {
    tracksInFolder = 0;
  }

  folderPlaybackActive = true;
  startFolderTrack(folder, track);
}

void AudioPlayer::stop() {
  if (!ready) return;

  dfPlayer.stop();
  playing = false;
  folderPlaybackActive = false;
  currentFolder = 0;
  currentTrack = 0;
}

void AudioPlayer::pause() {
  if (!ready) return;

  dfPlayer.pause();
  playing = false;
}

void AudioPlayer::resume() {
  if (!ready) return;

  dfPlayer.start();
  playing = true;
}

void AudioPlayer::next() {
  if (!ready) return;

  if (folderPlaybackActive && currentFolder > 0 && currentTrack > 0) {
    if (tracksInFolder > 0 && currentTrack >= tracksInFolder) {
      stop();
      return;
    }

    startFolderTrack(currentFolder, currentTrack + 1);
    return;
  }

  dfPlayer.next();
  playing = true;
}

void AudioPlayer::previous() {
  if (!ready) return;

  if (folderPlaybackActive && currentFolder > 0 && currentTrack > 1) {
    startFolderTrack(currentFolder, currentTrack - 1);
    return;
  }

  dfPlayer.previous();
  playing = true;
}

void AudioPlayer::setVolume(uint8_t volume) {
  if (!ready) return;

  dfPlayer.volume(volume);
}

AudioPlayerStatus AudioPlayer::readStatus() {
  AudioPlayerStatus status;

  if (!ready) {
    return status;
  }

  status.state = dfPlayer.readState();
  status.volume = dfPlayer.readVolume();
  status.currentFile = dfPlayer.readCurrentFileNumber();
  status.fileCount = dfPlayer.readFileCounts();

  return status;
}

String AudioPlayer::getStatusText() const {
  return statusText;
}

void AudioPlayer::startFolderTrack(uint8_t folder, uint8_t track) {
  dfPlayer.playFolder(folder, track);
  currentFolder = folder;
  currentTrack = track;
  playing = true;
  statusText = "Ordner " + String(folder) + " Track " + String(track);
}

void AudioPlayer::handlePlayFinished() {
  if (!folderPlaybackActive || currentFolder == 0 || currentTrack == 0) {
    playing = false;
    return;
  }

  if (tracksInFolder == 0 && currentTrack < 255) {
    startFolderTrack(currentFolder, currentTrack + 1);
    return;
  }

  if (tracksInFolder > 0 && currentTrack < tracksInFolder) {
    startFolderTrack(currentFolder, currentTrack + 1);
    return;
  }

  playing = false;
  folderPlaybackActive = false;
  statusText = "Ordner " + String(currentFolder) + " beendet";
}
