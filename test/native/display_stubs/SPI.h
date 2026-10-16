#pragma once
#include <Arduino.h>
struct SPISettings {
  SPISettings(unsigned, int, int) {}
};
struct SPIClass {
  void begin() {}
  void end() {}
  void beginTransaction(SPISettings) {}
  void endTransaction() {}
  uint8_t transfer(uint8_t value) {
    displayTest::receiveByte(value);
    return 0;
  }
};
inline SPIClass SPI;
