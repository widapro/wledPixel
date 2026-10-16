#include "DisplayRotation.h"
#include <MD_MAX72xx.h>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace displayRotation {
bool flipped = false;
uint8_t moduleCount = 8;
}

void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void checkFrame(MD_MAX72XX &matrix, bool rotated) {
  const uint16_t columns = displayRotation::moduleCount * 8u;
  for (uint16_t col = 0; col < columns; ++col) {
    for (uint8_t row = 0; row < 8; ++row) {
      const uint16_t physicalCol = rotated ? columns - 1 - col : col;
      const uint8_t physicalRow = rotated ? 7 - row : row;
      const bool physical = displayTest::registers[physicalCol / 8][physicalRow + 1] & (1u << (physicalCol % 8));
      check(physical == matrix.getPoint(row, col), "physical pixels do not match rotation");
    }
  }
}

void refreshFrame(MD_MAX72XX &matrix) {
  matrix.update(MD_MAX72XX::OFF);
  for (uint16_t col = 0; col < matrix.getColumnCount(); ++col)
    matrix.setColumn(col, matrix.getColumn(col));
  matrix.update(MD_MAX72XX::ON);
}

void exerciseDriver(bool hardwareSpi, uint8_t modules) {
  displayRotation::moduleCount = modules;
  displayRotation::flipped = false;
  std::unique_ptr<MD_MAX72XX> driver(hardwareSpi
      ? new MD_MAX72XX(MD_MAX72XX::FC16_HW, 5, 16)
      : new MD_MAX72XX(MD_MAX72XX::FC16_HW, 23, 18, 5, 16));
  auto &matrix = *driver;
  check(matrix.begin(), "driver init failed");
  matrix.update(MD_MAX72XX::OFF);
  for (uint16_t col = 0; col < modules * 8u; ++col)
    matrix.setColumn(col, (col * 37u + 3u) & 0xffu);
  matrix.update(MD_MAX72XX::ON);
  checkFrame(matrix, false);

  const uint8_t original = matrix.getColumn(9);
  displayRotation::flipped = true;
  refreshFrame(matrix);
  checkFrame(matrix, true);
  check(matrix.getColumn(9) == original, "rotation corrupted logical framebuffer");

  // A one-pixel update after a full rotated frame exercises dirty-row output.
  matrix.setPoint(2, 9, !matrix.getPoint(2, 9));
  checkFrame(matrix, true);
  matrix.setColumn(0, 0x80); // Progress bar on row 7 must rotate to row 0.
  checkFrame(matrix, true);

  matrix.control(0, 2, MD_MAX72XX::INTENSITY, 5);
  for (uint8_t device = modules - 3; device < modules; ++device)
    check(displayTest::registers[device][10] == 5, "zone intensity mapped incorrectly");
  matrix.control(0, MD_MAX72XX::SHUTDOWN, MD_MAX72XX::ON);
  check(displayTest::registers[modules - 1][12] == 0, "shutdown control changed value");

  displayRotation::flipped = false;
  refreshFrame(matrix);
  checkFrame(matrix, false);
}

int main() {
  try {
    // All MAX7219 registers and byte values must survive two transforms.
    for (uint8_t modules = 1; modules <= 16; ++modules) {
      for (unsigned value = 0; value < 256; ++value) {
        std::array<uint8_t, 32> packet;
        for (uint8_t device = 0; device < 16; ++device) {
          packet[device * 2] = device;
          packet[device * 2 + 1] = value;
        }
        const auto original = packet;
        displayRotation::transformSpiBuffer(packet.data(), 16, modules);
        for (uint8_t i = 0; i < (16 - modules) * 2; ++i)
          check(packet[i] == original[i], "spare module packet changed");
        displayRotation::transformSpiBuffer(packet.data(), 16, modules);
        check(packet == original, "SPI packet was not restored");
      }
    }
    for (const uint8_t modules : {3, 8, 16}) {
      exerciseDriver(true, modules);
      exerciseDriver(false, modules);
    }
    std::cout << "PASS display rotation: real MAX72XX driver, hardware/software SPI, "
                 "pixels, partial updates, control packets and unchanged logical frame\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
