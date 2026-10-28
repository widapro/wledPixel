#pragma once

#include "Scheduler.h"

// The display settings survive reboot, so retain the profile that applied them
// too. Missing/invalid records remain unknown for older firmware installations.
template <typename Storage>
int8_t loadLastActiveProfile(Storage &storage) {
  const uint8_t stored = storage.getUChar("lastActive", 255);
  return stored < MAX_PROFILES ? static_cast<int8_t>(stored) : -1;
}

template <typename Storage>
bool storeLastActiveProfile(Storage &storage, int8_t active) {
  const uint8_t stored = active >= 0 && active < MAX_PROFILES
                             ? static_cast<uint8_t>(active) : 255;
  // A schedule can apply the same profile every minute. Avoid flash writes
  // when the identity has not changed.
  if (storage.getUChar("lastActive", 255) == stored) return true;
  return storage.putUChar("lastActive", stored) == 1;
}
