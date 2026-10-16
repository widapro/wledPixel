#pragma once

#include <cstdint>

namespace displayRotation {

// Output state is updated on the main loop, separately from saved settings.
extern bool flipped;
extern uint8_t moduleCount;

inline uint8_t reverseBits(uint8_t value) {
  value = ((value & 0x55u) << 1) | ((value >> 1) & 0x55u);
  value = ((value & 0x33u) << 2) | ((value >> 2) & 0x33u);
  return (value << 4) | (value >> 4);
}

// MAX7219 packets contain register/value pairs, farthest device first.
// Rotate only the connected chain; the driver may allocate spare devices.
// Keep control values intact, but send them to the corresponding rotated zone.
// This operation is its own inverse, so the driver's packet and framebuffer
// retain their original coordinates after transmission.
inline void transformSpiBuffer(uint8_t *data, uint8_t driverDevices,
                               uint8_t connectedDevices) {
  if (!data || connectedDevices == 0 || connectedDevices > driverDevices)
    return;
  const uint16_t first = (driverDevices - connectedDevices) * 2u;
  for (uint16_t i = first; i < driverDevices * 2u; i += 2) {
    if (data[i] >= 1 && data[i] <= 8) {
      data[i] = 9 - data[i];
      data[i + 1] = reverseBits(data[i + 1]);
    }
  }
  for (uint8_t device = 0; device < connectedDevices / 2; ++device) {
    const uint16_t left = first + device * 2u;
    const uint16_t right = (driverDevices - 1u - device) * 2u;
    for (uint8_t byte = 0; byte < 2; ++byte) {
      const uint8_t temp = data[left + byte];
      data[left + byte] = data[right + byte];
      data[right + byte] = temp;
    }
  }
}

} // namespace displayRotation
