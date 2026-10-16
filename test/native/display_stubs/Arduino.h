#pragma once
#include "../stubs/Arduino.h"
#include <vector>

#define bitRead(value, bit) (((value) >> (bit)) & 1u)
#define bitSet(value, bit) ((value) |= (1u << (bit)))
#define bitClear(value, bit) ((value) &= ~(1u << (bit)))
#define bitWrite(value, bit, state) ((state) ? bitSet(value, bit) : bitClear(value, bit))
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define MSBFIRST 1
#define SPI_MODE0 0
using std::min;
using boolean = bool;

namespace displayTest {
inline std::vector<uint8_t> packet;
inline uint8_t registers[16][16] = {};
inline unsigned transactions = 0;
inline void receiveByte(uint8_t value) { packet.push_back(value); }
}

inline void pinMode(int, int) {}
inline void digitalWrite(int, int value) {
  if (value == LOW) {
    displayTest::packet.clear();
  } else if (!displayTest::packet.empty()) {
    const auto &packet = displayTest::packet;
    for (size_t device = 0; device < packet.size() / 2; ++device) {
      const size_t offset = packet.size() - (device + 1) * 2;
      if (packet[offset] != 0)
        displayTest::registers[device][packet[offset]] = packet[offset + 1];
    }
    ++displayTest::transactions;
  }
}
inline void shiftOut(int, int, int, uint8_t value) {
  displayTest::receiveByte(value);
}
