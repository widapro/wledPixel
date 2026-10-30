#pragma once

#include <cstddef>
#include <cstring>

// Keep saved settings and older API/MQTT commands working after the sprite
// replacement. Only the effect identifier is retained for compatibility.
template <typename TString>
inline void migrateScrollEffect(TString &effect) {
  if (effect == "MARIO")
    effect = "ROBOT";
}

template <size_t N>
inline void migrateScrollEffect(char (&effect)[N]) {
  static_assert(N >= sizeof("ROBOT"), "effect buffer is too small");
  if (std::strcmp(effect, "MARIO") == 0) {
    std::memset(effect, 0, N);
    std::memcpy(effect, "ROBOT", sizeof("ROBOT"));
  }
}
