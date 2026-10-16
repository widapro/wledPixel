#include "TextEncoding.h"
#include "base64_utils.h"
#include "countdown.h"
#include "SchedulerLogic.h"
#include "stockTicker.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int testsRun = 0;

void fail(const std::string &message) { throw std::runtime_error(message); }

void check(bool condition, const char *expression, int line) {
  if (!condition)
    fail("line " + std::to_string(line) + ": " + expression);
}

void checkString(const String &actual, const char *expected, int line) {
  if (actual != expected) {
    fail("line " + std::to_string(line) + ": expected '" + expected +
         "', got '" + actual.c_str() + "'");
  }
}

void checkByte(uint8_t actual, uint8_t expected, int line) {
  if (actual != expected) {
    fail("line " + std::to_string(line) + ": expected byte " +
         std::to_string(expected) + ", got " + std::to_string(actual));
  }
}

#define CHECK(expression) check((expression), #expression, __LINE__)
#define CHECK_STRING(actual, expected) checkString((actual), (expected), __LINE__)
#define CHECK_BYTE(actual, expected)                                           \
  checkByte(static_cast<uint8_t>(actual), static_cast<uint8_t>(expected),      \
            __LINE__)

unsigned long utcEpoch(int year, int month, int day, int hour = 0,
                       int minute = 0, int second = 0) {
  struct tm value = {};
  value.tm_year = year - 1900;
  value.tm_mon = month - 1;
  value.tm_mday = day;
  value.tm_hour = hour;
  value.tm_min = minute;
  value.tm_sec = second;
  time_t epoch = timegm(&value);
  if (epoch < 0)
    fail("unable to construct test epoch");
  return static_cast<unsigned long>(epoch);
}

void testBase64RfcVectors() {
  struct Vector {
    const char *plain;
    const char *encoded;
  };
  const Vector vectors[] = {{"", ""},       {"f", "Zg=="},
                            {"fo", "Zm8="}, {"foo", "Zm9v"},
                            {"foob", "Zm9vYg=="},
                            {"fooba", "Zm9vYmE="},
                            {"foobar", "Zm9vYmFy"}};
  for (const auto &vector : vectors) {
    CHECK_STRING(base64_utils::encode(vector.plain), vector.encoded);
    CHECK_STRING(base64_utils::decode(vector.encoded), vector.plain);
  }
}

void testBase64RoundTrip() {
  const String source = "wledPixel: MQTT + HTTP / Cyrillic config";
  CHECK(base64_utils::decode(base64_utils::encode(source)) == source);
  CHECK_STRING(base64_utils::decode("Zm9v!ignored"), "foo");
}

void testCountdownParsesSupportedFormats() {
  countdownInit();
  const unsigned long now = utcEpoch(2026, 8, 19, 12, 0, 0);

  CHECK(parseCountdownTarget("2026-12-31T23:59:59", 0, now, 0));
  CHECK(countdownState[0].target == utcEpoch(2026, 12, 31, 23, 59, 59));
  CHECK_STRING(countdownState[0].targetStr, "2026-12-31T23:59:59");

  CHECK(parseCountdownTarget("2026-12-31T23:59", 1, now, 0));
  CHECK(countdownState[1].target == utcEpoch(2026, 12, 31, 23, 59, 0));
  CHECK_STRING(countdownState[1].targetStr, "2026-12-31T23:59:00");

  CHECK(parseCountdownTarget("2026-12-31", 2, now, 0));
  CHECK(countdownState[2].target == utcEpoch(2026, 12, 31));

  CHECK(parseCountdownTarget("23:59:59", 3, now, 0));
  CHECK(countdownState[3].target == utcEpoch(2026, 8, 19, 23, 59, 59));
}

void testCountdownTimezoneAndWhitespace() {
  countdownInit();
  const unsigned long now = utcEpoch(2026, 8, 19, 12, 0, 0);
  CHECK(parseCountdownTarget(" 2026-08-20T05:30:00 ", 0, now, 5.5f));
  CHECK(countdownState[0].target == utcEpoch(2026, 8, 20, 0, 0, 0));
  CHECK_STRING(countdownState[0].targetStr, "2026-08-20T05:30:00");

  CHECK(parseCountdownTarget("20:15:00", 1, now, -4.0f));
  CHECK(countdownState[1].target == utcEpoch(2026, 8, 20, 0, 15, 0));
  CHECK_STRING(countdownState[1].targetStr, "2026-08-19T20:15:00");
}

void testCountdownRejectsInvalidInput() {
  countdownInit();
  const unsigned long now = utcEpoch(2026, 8, 19);
  CHECK(!parseCountdownTarget("", 0, now, 0));
  CHECK(!parseCountdownTarget("2026-12-31", -1, now, 0));
  CHECK(!parseCountdownTarget("2026-12-31", 4, now, 0));
  CHECK(!parseCountdownTarget("2026/12/31T23:59:59", 0, now, 0));
  CHECK(!parseCountdownTarget("2026-02-29T10:00:00", 0, now, 0));
  CHECK(!parseCountdownTarget("2026-02-30T10:00:00", 0, now, 0));
  CHECK(!parseCountdownTarget("2026-12-31T24:00:00", 0, now, 0));
  CHECK(!parseCountdownTarget("2026-12-31T23:60:00", 0, now, 0));
  CHECK(!parseCountdownTarget("2026-12-31T23:59:60", 0, now, 0));
  CHECK(!parseCountdownTarget("2026-12-31T23:59:59junk", 0, now, 0));
  CHECK(parseCountdownTarget("2024-02-29T10:00:00", 0, now, 0));
}

void testCountdownFormattingAndReachedState() {
  countdownInit();
  const unsigned long now = utcEpoch(2026, 8, 19, 12, 0, 0);
  countdownState[0].target = now + 90061;

  CHECK_STRING(getCountdownText(0, "SS", false, "", "", now, 0), "90061");
  CHECK_STRING(getCountdownText(0, "HHMMSS", false, "", "", now, 0),
               "25:01:01");
  CHECK_STRING(getCountdownText(0, "DDHHMMSS", true, "T-", "!", now, 0),
               "T-1d 1h 01m 01s!");
  CHECK(!countdownState[0].reached);

  CHECK_STRING(getCountdownText(0, "SS", true, "", "", now + 90061, 0),
               "0s");
  CHECK(countdownState[0].reached);
  CHECK_STRING(getCountdownText(3, "SS", false, "[", "]", now, 0), "[---]");
}

void testCountdownBlinkModes() {
  countdownInit();
  countdownState[0].reached = true;
  CHECK(countdownHandleBlink(0, "none", 500));
  CHECK(!countdownHandleBlink(0, "blink3", 500));
  CHECK(countdownHandleBlink(0, "blink3", 1000));
  CHECK(!countdownHandleBlink(0, "blink3", 1500));
  CHECK(countdownHandleBlink(0, "blink3", 2000));
  CHECK(!countdownHandleBlink(0, "blink3", 2500));
  CHECK(countdownHandleBlink(0, "blink3", 3000));
  CHECK(countdownHandleBlink(0, "blink3", 3500));
  CHECK(countdownHandleBlink(-1, "blink3", 500));
}

void testStockSymbolParsing() {
  stockParseSymbols(0, " aapl, msft ,, brk.b, nvda, googl, ignored ");
  CHECK(stockZones[0].symbolCount == STOCK_MAX_SYMBOLS);
  CHECK_STRING(stockZones[0].quotes[0].symbol, "AAPL");
  CHECK_STRING(stockZones[0].quotes[1].symbol, "MSFT");
  CHECK_STRING(stockZones[0].quotes[2].symbol, "BRK.B");
  CHECK_STRING(stockZones[0].quotes[3].symbol, "NVDA");
  CHECK_STRING(stockZones[0].quotes[4].symbol, "GOOGL");

  stockZones[0].quotes[0].valid = true;
  stockParseSymbols(0, "");
  CHECK(stockZones[0].symbolCount == 0);
  CHECK(!stockZones[0].quotes[0].valid);
}

void testStockDisplayFormatting() {
  stockParseSymbols(0, "aapl,msft,bad");
  CHECK_STRING(stockGetDisplayString(0, true, "namePrice", "", ""),
               "Loading...");

  stockZones[0].quotes[0].valid = true;
  stockZones[0].quotes[0].price = 182.60f;
  stockZones[0].quotes[0].change = 1.0f;
  stockZones[0].quotes[0].changePct = 2.30f;
  stockZones[0].quotes[1].valid = true;
  stockZones[0].quotes[1].price = 410.0f;
  stockZones[0].quotes[1].change = -2.0f;
  stockZones[0].quotes[1].changePct = -0.50f;
  stockZones[0].quotes[2].error = true;

  String names = stockGetDisplayString(0, true, "namePrice", "$", " USD");
  std::string expectedNames = "$AAPL 182.6";
  expectedNames.push_back(STOCK_ARROW_UP);
  expectedNames += "  MSFT 410";
  expectedNames.push_back(STOCK_ARROW_DOWN);
  expectedNames += "  Err USD";
  CHECK(names == expectedNames.c_str());

  CHECK_STRING(stockGetDisplayString(0, false, "changePct", "", ""),
               "AAPL +2.3%  MSFT -0.5%  Err");
  CHECK_STRING(stockGetDisplayString(0, false, "pctOnly", "", ""),
               "+2.3%  -0.5%  Err");
  CHECK_STRING(stockGetDisplayString(4, false, "priceOnly", "", ""), "err");
}

struct tm scheduleTime(int dayOfMonth, int dayOfWeek, int hour, int minute) {
  struct tm now = {};
  now.tm_mday = dayOfMonth;
  now.tm_wday = dayOfWeek;
  now.tm_hour = hour;
  now.tm_min = minute;
  return now;
}

void testSchedulerMatching() {
  const struct tm monday = scheduleTime(19, 1, 7, 30);
  CHECK(!schedulerTimeMatches(false, 30, 7, 0x7F, 0x7FFFFFFF, monday));
  CHECK(schedulerTimeMatches(true, 30, 7, 0x7F, 0x7FFFFFFF, monday));
  CHECK(schedulerTimeMatches(true, 255, 255, 0x7F, 0x7FFFFFFF, monday));
  CHECK(!schedulerTimeMatches(true, 31, 7, 0x7F, 0x7FFFFFFF, monday));
  CHECK(!schedulerTimeMatches(true, 30, 8, 0x7F, 0x7FFFFFFF, monday));
  CHECK(schedulerTimeMatches(true, 30, 7, 1U << 1, 0x7FFFFFFF,
                             monday));
  CHECK(!schedulerTimeMatches(true, 30, 7, 1U << 2, 0x7FFFFFFF,
                              monday));
  CHECK(schedulerTimeMatches(true, 30, 7, 0x7F, 1UL << 18, monday));
  CHECK(!schedulerTimeMatches(true, 30, 7, 0x7F, 1UL << 17, monday));

  struct tm invalid = monday;
  invalid.tm_wday = 7;
  CHECK(!schedulerTimeMatches(true, 255, 255, 0x7F, 0x7FFFFFFF, invalid));
  invalid = monday;
  invalid.tm_mday = 0;
  CHECK(!schedulerTimeMatches(true, 255, 255, 0x7F, 0x7FFFFFFF, invalid));
}

std::vector<uint8_t> encodedBytes(const char *text, const char *font,
                                  size_t bufferSize = 64) {
  std::vector<char> buffer(bufferSize, static_cast<char>(0x55));
  utf8ToFontEncoding(text, buffer.data(), buffer.size(), font);
  std::vector<uint8_t> result;
  for (char byte : buffer) {
    if (byte == '\0')
      break;
    result.push_back(static_cast<uint8_t>(byte));
  }
  return result;
}

void testTextEncodingAsciiAndCyrillic() {
  auto ascii = encodedBytes("Hello", "default");
  CHECK(ascii == std::vector<uint8_t>({'H', 'e', 'l', 'l', 'o'}));

  auto cyrillic = encodedBytes("АаРрЁё", "wledFont_cyrillic");
  CHECK(cyrillic.size() == 6);
  CHECK_BYTE(cyrillic[0], 128);
  CHECK_BYTE(cyrillic[1], 194);
  CHECK_BYTE(cyrillic[2], 144);
  CHECK_BYTE(cyrillic[3], 195);
  CHECK_BYTE(cyrillic[4], 192);
  CHECK_BYTE(cyrillic[5], 193);

  auto unsupportedFont = encodedBytes("А", "default");
  CHECK(unsupportedFont == std::vector<uint8_t>({'?'}));
}

void testTextEncodingCompactSymbols() {
  auto encoded = encodedBytes("~!~P~@~~X~", "wledFont_CompactSymbols");
  CHECK(encoded.size() == 6);
  CHECK_BYTE(encoded[0], 129);
  CHECK_BYTE(encoded[1], 224);
  CHECK_BYTE(encoded[2], 230);
  CHECK_BYTE(encoded[3], '~');
  CHECK_BYTE(encoded[4], 'X');
  CHECK_BYTE(encoded[5], '~');
}

void testTextEncodingBoundsAndMalformedUtf8() {
  char oneByte[1] = {static_cast<char>(0x55)};
  utf8ToFontEncoding("abc", oneByte, sizeof(oneByte), "default");
  CHECK(oneByte[0] == '\0');

  char untouched = static_cast<char>(0x55);
  utf8ToFontEncoding("abc", &untouched, 0, "default");
  CHECK_BYTE(untouched, 0x55);

  char nullSource[4] = {'x', 'x', 'x', '\0'};
  utf8ToFontEncoding(nullptr, nullSource, sizeof(nullSource), "default");
  CHECK(nullSource[0] == '\0');

  auto truncated = encodedBytes("\xE2", "default");
  CHECK(truncated == std::vector<uint8_t>({'?'}));
  auto badContinuation = encodedBytes("\xD0" "A", "wledFont_cyrillic");
  CHECK(badContinuation == std::vector<uint8_t>({'?', 'A'}));
  auto emoji = encodedBytes("☃😀", "default");
  CHECK(emoji == std::vector<uint8_t>({'?', '?'}));

  char small[4] = {};
  utf8ToFontEncoding("abcdef", small, sizeof(small), "default");
  CHECK(std::strcmp(small, "abc") == 0);
}

void run(const char *name, const std::function<void()> &test) {
  try {
    test();
    testsRun++;
    std::cout << "PASS " << name << '\n';
  } catch (const std::exception &error) {
    std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    std::exit(1);
  }
}
} // namespace

int main() {
  setenv("TZ", "UTC", 1);
  tzset();

  run("base64 RFC 4648 vectors", testBase64RfcVectors);
  run("base64 round trip", testBase64RoundTrip);
  run("countdown supported formats", testCountdownParsesSupportedFormats);
  run("countdown timezone and whitespace", testCountdownTimezoneAndWhitespace);
  run("countdown invalid input", testCountdownRejectsInvalidInput);
  run("countdown formatting", testCountdownFormattingAndReachedState);
  run("countdown blink modes", testCountdownBlinkModes);
  run("stock symbol parsing", testStockSymbolParsing);
  run("stock display formatting", testStockDisplayFormatting);
  run("scheduler matching", testSchedulerMatching);
  run("text encoding ASCII and Cyrillic", testTextEncodingAsciiAndCyrillic);
  run("text encoding compact symbols", testTextEncodingCompactSymbols);
  run("text encoding bounds and malformed UTF-8",
      testTextEncodingBoundsAndMalformedUtf8);

  std::cout << "All " << testsRun << " native test groups passed\n";
  return 0;
}
