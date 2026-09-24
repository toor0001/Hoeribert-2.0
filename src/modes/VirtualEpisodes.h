#pragma once

#include <stdint.h>

namespace VirtualEpisodes {

constexpr uint8_t PHYSICAL_FOLDER = 99;

inline bool mapToTrack(uint16_t episode, uint8_t& track) {
  // Array position is the physical file number in /99/ (starting at 001.mp3).
  static constexpr uint16_t episodes[] = {
    99, 100, 101, 102, 103, 104, 105, 106, 108, 109, 110, 111,
    119, 121, 122, 123, 124, 125, 126, 127, 128, 130, 132, 133,
    134, 135, 137, 138, 139, 140, 142, 143, 148, 153, 154, 155,
    160, 161, 162, 165, 170, 171, 175, 179, 180, 999
  };
  track = 0;
  for (unsigned i = 0; i < sizeof(episodes) / sizeof(episodes[0]); ++i) {
    if (episodes[i] == episode) {
      track = static_cast<uint8_t>(i + 1);
      return true;
    }
  }
  return false;
}

} // namespace VirtualEpisodes
