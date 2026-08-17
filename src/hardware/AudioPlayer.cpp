#include "AudioPlayer.h"

bool AudioPlayer::begin() {
  dfSerial.begin(9600, SERIAL_8N1, DF_RX_PIN, DF_TX_PIN);
  delay(500);

  ready = dfPlayer.begin(dfSerial);
  playing = false;
  folderPlaybackActive = false;
  folderFinished = false;
  currentFolder = 0;
  currentTrack = 0;
  tracksInFolder = 0;
  trackStartedAt = 0;
  trackElapsedBeforePause = 0;
  playbackGeneration = 0;
  lastPlayCommandAt = 0;
  lastPlayFolder = 0;
  lastPlayTrack = 0;
  lastPlaySource = "NONE";
  resetFinishHistory();
  statusText = ready ? "DFPlayer bereit." : "DFPlayer NICHT gefunden!";

  return ready;
}

void AudioPlayer::update() {
  if (!ready) return;

  while (dfPlayer.available()) {
    uint8_t type = dfPlayer.readType();
    uint16_t value = dfPlayer.read();
    unsigned long now = millis();

    Serial.printf(
      "[%lu] RX %s type=%u value=%u currentFolder=%u currentTrack=%u "
      "tracksInFolder=%d playing=%u folderActive=%u folderFinished=%u "
      "lastPlayAgeMs=%lu lastPlayFolder=%u lastPlayTrack=%u "
      "lastPlaySource=%s generation=%lu\n",
      now,
      eventTypeName(type),
      type,
      value,
      currentFolder,
      currentTrack,
      tracksInFolder,
      playing,
      folderPlaybackActive,
      folderFinished,
      lastPlayCommandAt == 0 ? 0UL : now - lastPlayCommandAt,
      lastPlayFolder,
      lastPlayTrack,
      lastPlaySource,
      static_cast<unsigned long>(playbackGeneration)
    );

    if (type == DFPlayerPlayFinished) {
      unsigned long finishAgeMs = now - lastAcceptedFinishAt;
      bool duplicateFinish = hasAcceptedFinish &&
                             value == lastAcceptedFinishValue &&
                             finishAgeMs <= DUPLICATE_FINISH_WINDOW_MS;

      if (duplicateFinish) {
        Serial.printf(
          "[%lu] RX FINISH DUPLICATE value=%u ageMs=%lu currentFolder=%u "
          "currentTrack=%u generation=%lu ignored\n",
          now,
          value,
          finishAgeMs,
          currentFolder,
          currentTrack,
          static_cast<unsigned long>(playbackGeneration)
        );
        continue;
      }

      hasAcceptedFinish = true;
      lastAcceptedFinishValue = value;
      lastAcceptedFinishAt = now;
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

void AudioPlayer::playFolder(uint8_t folder, const char* source) {
  if (!ready) return;

  resetFinishHistory();
  folderFinished = false;
  tracksInFolder = dfPlayer.readFileCountsInFolder(folder);
  if (tracksInFolder < 1) {
    tracksInFolder = 0;
  }

  folderPlaybackActive = true;
  startFolderTrack(folder, 1, source);
}

void AudioPlayer::playFolderTrack(uint8_t folder, uint8_t track, const char* source) {
  if (!ready) return;

  resetFinishHistory();
  folderFinished = false;
  tracksInFolder = dfPlayer.readFileCountsInFolder(folder);
  if (tracksInFolder < 1) {
    tracksInFolder = 0;
  }

  folderPlaybackActive = true;
  startFolderTrack(folder, track, source);
}

PlaybackPosition AudioPlayer::getPlaybackPosition() const {
  PlaybackPosition position;

  if (currentFolder == 0 || currentTrack == 0) {
    return position;
  }

  position.valid = true;
  position.folder = currentFolder;
  position.track = currentTrack;
  position.seconds = currentTrackSeconds();
  return position;
}

bool AudioPlayer::consumeFolderFinished() {
  bool finished = folderFinished;
  folderFinished = false;
  return finished;
}

void AudioPlayer::stop() {
  if (!ready) return;

  resetFinishHistory();
  dfPlayer.stop();
  playing = false;
  folderPlaybackActive = false;
  folderFinished = false;
  currentFolder = 0;
  currentTrack = 0;
  trackStartedAt = 0;
  trackElapsedBeforePause = 0;
}

void AudioPlayer::pause() {
  if (!ready) return;

  trackElapsedBeforePause = currentTrackSeconds();
  dfPlayer.pause();
  playing = false;
}

void AudioPlayer::resume() {
  if (!ready) return;

  dfPlayer.start();
  playing = true;
  trackStartedAt = millis();
}

void AudioPlayer::next() {
  if (!ready) return;

  resetFinishHistory();
  if (folderPlaybackActive && currentFolder > 0 && currentTrack > 0) {
    if (tracksInFolder > 0 && currentTrack >= tracksInFolder) {
      stop();
      return;
    }

    startFolderTrack(currentFolder, currentTrack + 1, "NEXT_BUTTON");
    return;
  }

  dfPlayer.next();
  playing = true;
  trackStartedAt = millis();
  trackElapsedBeforePause = 0;
}

void AudioPlayer::previous() {
  if (!ready) return;

  resetFinishHistory();
  if (folderPlaybackActive && currentFolder > 0 && currentTrack > 1) {
    startFolderTrack(currentFolder, currentTrack - 1, "PREVIOUS_BUTTON");
    return;
  }

  dfPlayer.previous();
  playing = true;
  trackStartedAt = millis();
  trackElapsedBeforePause = 0;
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

void AudioPlayer::startFolderTrack(uint8_t folder, uint8_t track, const char* source) {
  unsigned long now = millis();
  uint8_t previousFolder = currentFolder;
  uint8_t previousTrack = currentTrack;

  playbackGeneration++;
  lastPlayCommandAt = now;
  lastPlayFolder = folder;
  lastPlayTrack = track;
  lastPlaySource = source;

  Serial.printf(
    "[%lu] TX PLAY_FOLDER source=%s folder=%u track=%u "
    "previousFolder=%u previousTrack=%u generation=%lu\n",
    now,
    source,
    folder,
    track,
    previousFolder,
    previousTrack,
    static_cast<unsigned long>(playbackGeneration)
  );

  dfPlayer.playFolder(folder, track);
  currentFolder = folder;
  currentTrack = track;
  playing = true;
  folderFinished = false;
  trackStartedAt = millis();
  trackElapsedBeforePause = 0;
  statusText = "Ordner " + String(folder) + " Track " + String(track);
}

uint16_t AudioPlayer::currentTrackSeconds() const {
  uint32_t seconds = trackElapsedBeforePause;

  if (playing && trackStartedAt > 0) {
    seconds += (millis() - trackStartedAt) / 1000UL;
  }

  return seconds > 65535UL ? 65535 : static_cast<uint16_t>(seconds);
}

void AudioPlayer::handlePlayFinished() {
  if (!folderPlaybackActive || currentFolder == 0 || currentTrack == 0) {
    playing = false;
    return;
  }

  if (tracksInFolder == 0 && currentTrack < 255) {
    startFolderTrack(currentFolder, currentTrack + 1, "FINISH_EVENT");
    return;
  }

  if (tracksInFolder > 0 && currentTrack < tracksInFolder) {
    startFolderTrack(currentFolder, currentTrack + 1, "FINISH_EVENT");
    return;
  }

  playing = false;
  folderPlaybackActive = false;
  folderFinished = true;
  trackStartedAt = 0;
  trackElapsedBeforePause = 0;
  statusText = "Ordner " + String(currentFolder) + " beendet";
}

const char* AudioPlayer::eventTypeName(uint8_t type) const {
  switch (type) {
    case TimeOut: return "TIMEOUT";
    case WrongStack: return "WRONG_STACK";
    case DFPlayerCardInserted: return "CARD_INSERTED";
    case DFPlayerCardRemoved: return "CARD_REMOVED";
    case DFPlayerCardOnline: return "CARD_ONLINE";
    case DFPlayerUSBInserted: return "USB_INSERTED";
    case DFPlayerUSBRemoved: return "USB_REMOVED";
    case DFPlayerPlayFinished: return "FINISH";
    case DFPlayerError: return "ERROR";
    default: return "OTHER";
  }
}

void AudioPlayer::resetFinishHistory() {
  hasAcceptedFinish = false;
  lastAcceptedFinishValue = 0;
  lastAcceptedFinishAt = 0;
}
