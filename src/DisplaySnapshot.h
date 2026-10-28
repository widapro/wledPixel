#pragma once

#include "DisplayRotation.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>

namespace displaySnapshot {

constexpr uint8_t kMaxModules = 16;

struct Frame {
  uint8_t modules = 0;
  bool power = false;
  uint32_t capturedAt = 0;
  uint8_t profileIndex = 0; // Zero-based, like the Profiles API.
  char profileName[12] = {};
  // Screen order: left to right, bit 0 is the top row.
  uint8_t data[kMaxModules * 8] = {};
};

struct Request {
  Frame frame;
  std::atomic<bool> ready{false};
};

// Both targets keep profile names in RAM, so capturing them needs no flash I/O.
template <typename Profile>
void captureProfile(Frame &frame, const Profile *profiles, uint8_t profileCount,
                    int8_t lastActive) {
  frame.profileIndex = lastActive >= 0 && lastActive < profileCount
                           ? static_cast<uint8_t>(lastActive) : 0;
  frame.profileName[0] = '\0';
  if (!profileCount)
    return;
  std::memcpy(frame.profileName, profiles[frame.profileIndex].name,
              sizeof(frame.profileName) - 1);
  frame.profileName[sizeof(frame.profileName) - 1] = '\0';
}

// Use only the active layout, never spare driver buffers or saved profiles.
template <typename Matrix, typename Zone>
void capture(Frame &frame, Matrix &matrix, const Zone *zones, uint8_t zoneCount,
             bool power, bool flipped, uint32_t now) {
  const uint8_t capacity = matrix.getDeviceCount() < kMaxModules
                               ? matrix.getDeviceCount() : kMaxModules;
  uint8_t modules = capacity ? 1 : 0;
  for (uint8_t zone = 0; zone < zoneCount && zone < 4; ++zone) {
    if (zones[zone].begin <= zones[zone].end && zones[zone].end < capacity &&
        zones[zone].end + 1 > modules)
      modules = zones[zone].end + 1;
  }
  frame.modules = modules;
  frame.power = power;
  frame.capturedAt = now;
  const uint16_t columns = modules * 8u;
  for (uint16_t column = 0; column < columns; ++column) {
    const uint8_t pixels = power
        ? matrix.getColumn(flipped ? column : columns - 1 - column) : 0;
    frame.data[column] = flipped ? displayRotation::reverseBits(pixels) : pixels;
  }
}

// One-shot capture on the main loop; the response owns its immutable result.
std::shared_ptr<Request> request();
void service();

} // namespace displaySnapshot
