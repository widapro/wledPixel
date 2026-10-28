#include "DisplayRotation.h"
#include "DisplaySnapshot.h"
#include <MD_MAX72xx.h>
#include "../../lib/wledFont/PixelFont3D.h"
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

void exercisePixelFont() {
  displayRotation::moduleCount = 4;
  displayRotation::flipped = false;
  MD_MAX72XX matrix(MD_MAX72XX::FC16_HW, 5, 4);
  check(matrix.begin(), "font test driver init failed");
  check(matrix.setFont(pixelFont3D), "3D font selection failed");
  uint8_t columns[8] = {};
  // Check source BDF rows through the real driver, including both edge rows.
  const std::array<uint8_t, 8> expectedA = {
      0xfc, 0x86, 0xb6, 0x86, 0xb6, 0xb6, 0xfe, 0x7e};
  check(matrix.getChar('A', sizeof(columns), columns) == 7, "wrong 3D A width");
  for (uint8_t row = 0; row < 8; ++row) {
    uint8_t pixels = 0;
    for (uint8_t column = 0; column < 7; ++column)
      if (columns[column] & (1u << row)) pixels |= 0x80u >> column;
    check(pixels == expectedA[row], "3D glyph transposed, flipped or clipped");
  }
  for (uint16_t codepoint = 0; codepoint < 256; ++codepoint) {
    const auto width = matrix.getChar(codepoint, sizeof(columns), columns);
    check(width <= 8, "3D glyph wider than a module");
    if (codepoint >= 32)
      check(width > 0, "3D font missing a printable/fallback glyph");
  }
  const auto colonWidth = matrix.getChar(':', sizeof(columns), columns);
  check(matrix.getChar(160, sizeof(columns), columns) == colonWidth,
        "blinking 3D clock changes text width");
  for (uint8_t column = 0; column < colonWidth; ++column)
    check(columns[column] == 0, "clock blank has lit pixels");
  // Punctuation has a different BDF y-offset from full-height capitals.
  check(matrix.getChar('~', sizeof(columns), columns) == 8, "wrong tilde width");
  for (uint8_t column : columns)
    check((column & 0x81) == 0, "3D punctuation baseline shifted");
  for (uint16_t codepoint : {24, 25, 128, 176}) {
    check(matrix.getChar(codepoint, sizeof(columns), columns) > 0,
          "missing arrow, euro or temperature symbol");
  }
  matrix.setChar(15, 'A');
  checkFrame(matrix, false);
  displayRotation::flipped = true;
  refreshFrame(matrix);
  checkFrame(matrix, true);
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

void exerciseSnapshot() {
  MD_MAX72XX matrix(MD_MAX72XX::FC16_HW, 5, 16);
  check(matrix.begin(), "snapshot driver init failed");
  struct Zone { uint8_t begin, end; };
  // An inactive zone and spare driver buffers must not inflate the preview.
  const Zone zones[] = {{0, 1}, {2, 3}, {0, 15}, {255, 255}};
  displaySnapshot::Frame frame;
  for (const uint8_t modules : {1, 2, 4, 16}) {
    const Zone layout[] = {{0, static_cast<uint8_t>(modules - 1)}, {0, 15}};
    displayRotation::moduleCount = modules;
    for (const bool flipped : {false, true}) {
      displayRotation::flipped = flipped;
      matrix.update(MD_MAX72XX::OFF);
      for (uint16_t col = 0; col < 128; ++col)
        matrix.setColumn(col, (col * 37u + 3u) & 0xffu);
      matrix.update(MD_MAX72XX::ON);
      const auto transactions = displayTest::transactions;
      displaySnapshot::capture(frame, matrix, layout, 1, true, flipped, 1234);
      check(frame.modules == modules, "snapshot includes unconfigured modules");
      check(frame.power && frame.capturedAt == 1234, "snapshot metadata incorrect");
      check(transactions == displayTest::transactions, "snapshot wrote to display");
      for (uint16_t column = 0; column < modules * 8u; ++column) {
        const uint16_t physicalColumn = modules * 8u - 1 - column;
        for (uint8_t row = 0; row < 8; ++row) {
          const bool physical = displayTest::registers[physicalColumn / 8][row + 1] &
                                (1u << (physicalColumn % 8));
          check(physical == bool(frame.data[column] & (1u << row)),
                "snapshot does not match physical display orientation");
        }
      }
      const auto frozen = frame;
      matrix.setColumn(0, 0);
      check(frame.data[flipped ? 0 : modules * 8u - 1] ==
            frozen.data[flipped ? 0 : modules * 8u - 1], "snapshot is not immutable");
    }
  }
  displaySnapshot::capture(frame, matrix, zones, 1, true, false, 1);
  check(frame.modules == 2, "two configured modules must show exactly two modules");
  displaySnapshot::capture(frame, matrix, zones, 2, true, false, 1);
  check(frame.modules == 4, "snapshot missed a second active zone");
  displaySnapshot::capture(frame, matrix, zones, 1, false, true, 2);
  check(!frame.power, "powered-off snapshot reported on");
  for (uint16_t col = 0; col < frame.modules * 8u; ++col)
    check(frame.data[col] == 0, "powered-off snapshot shows stale lit pixels");
  const Zone invalid[] = {{0, 1}, {0, 255}, {5, 3}};
  displaySnapshot::capture(frame, matrix, invalid, 3, true, false, 3);
  check(frame.modules == 2, "invalid zone inflated snapshot geometry");

  struct Profile { char name[12]; };
  Profile profiles[] = {{"Default"}, {"Night"}, {"Third"}, {"Fourth"},
                        {"Fifth"}, {"Sixth"}, {"Seventh"}, {"Eighth"}};
  for (const uint8_t count : {4, 8}) {
    for (int8_t active = -1; active < count; ++active) {
      displaySnapshot::captureProfile(frame, profiles, count, active);
      const uint8_t expected = active < 0 ? 0 : active;
      check(frame.profileIndex == expected, "snapshot active profile differs from ON AIR");
      check(std::strcmp(frame.profileName, profiles[expected].name) == 0,
            "snapshot profile name is incorrect");
    }
  }
  displaySnapshot::captureProfile(frame, profiles, 4, 7);
  check(frame.profileIndex == 0, "invalid active profile did not fall back to default");
  displaySnapshot::captureProfile(frame, profiles, 8, 1);
  profiles[1].name[0] = 'X';
  check(std::strcmp(frame.profileName, "Night") == 0, "captured profile name is not immutable");
  std::memset(profiles[1].name, 'A', sizeof(profiles[1].name));
  displaySnapshot::captureProfile(frame, profiles, 8, 1);
  check(std::strlen(frame.profileName) == 11, "unterminated profile name escaped its buffer");
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
    exercisePixelFont();
    exerciseSnapshot();
    std::cout << "PASS display rotation: real MAX72XX driver, hardware/software SPI, "
                 "pixels, partial updates, control packets, 3D font, snapshots and unchanged logical frame\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
