#include "NetworkRequestResult.h"
#include "WeatherData.h"
#include "BackupStream.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

static unsigned backupLoads = 0;
static std::string backupSegment(BackupPart part, uint8_t profile, uint8_t zone) {
  if (part == BackupPart::Prefix)
    return "{\"text\":\"Прогноз\",\"prefix\":\"" + std::string(4000, 'p') + "\"";
  if (part == BackupPart::Profile)
    return ",\"profile" + std::to_string(profile) + "\":\"" + std::string(1800, 'x') + "\"";
  if (part == BackupPart::Bar)
    return ",\"bar" + std::to_string(profile) + std::to_string(zone) + "\":" +
           (zone % 2 ? "null" : "{\"value\":12.345}");
  return "}";
}
struct TestBackupWriter {
  std::string cached;
  void load(BackupPart part, uint8_t profile, uint8_t zone) {
    ++backupLoads;
    cached = backupSegment(part, profile, zone);
  }
  void render(ByteWindow &out, BackupPart, uint8_t, uint8_t) {
    for (size_t i = 0; i < cached.size(); i += 7)
      out.write(reinterpret_cast<const uint8_t *>(cached.data() + i),
                std::min(size_t(7), cached.size() - i));
  }
};

static void backupStreaming() {
  for (uint8_t profiles : {0, 4, 8}) {
    std::string expected = backupSegment(BackupPart::Prefix, 0, 0);
    for (uint8_t p = 0; p < profiles; ++p) {
      expected += backupSegment(BackupPart::Profile, p, 0);
      for (uint8_t z = 0; z < 4; ++z)
        expected += backupSegment(BackupPart::Bar, p, z);
    }
    expected += '}';
    for (size_t chunk : {1u, 17u, 1460u, 10000u}) {
      backupLoads = 0;
      BackupStream<TestBackupWriter> cursor(profiles);
      assert(cursor.fill(nullptr, 0) == 0 && backupLoads == 0);
      std::vector<uint8_t> buffer(chunk + 2, 0xA5);
      std::string actual;
      while (true) {
        auto priorLoads = backupLoads;
        size_t count = cursor.fill(buffer.data() + 1, chunk);
        assert(backupLoads - priorLoads <= 1);
        assert(buffer.front() == 0xA5 && buffer.back() == 0xA5);
        if (!count) break;
        actual.append(reinterpret_cast<const char *>(buffer.data() + 1), count);
      }
      assert(actual == expected);
      assert(backupLoads == 2u + unsigned(profiles) * 5u);
      assert(cursor.fill(buffer.data(), chunk) == 0);
    }
  }
}

bool isFetchingHttps = false;
volatile bool httpsAbortRequested = false;
static bool networkOwned = false, webActive = false, webArrives = false;
static unsigned releases = 0;
unsigned long millis() { return 1000; }
bool beginExternalNetworkRequest(bool) {
  if (networkOwned) return false;
  networkOwned = true;
  if (webArrives) webActive = true;
  return true;
}
void endExternalNetworkRequest() { networkOwned = false; ++releases; }
bool webPriorityIsActive(unsigned long) { return webActive; }
#include "NetworkRequestGuard.h"

static void requestOwnership() {
  {
    HttpsGuard outer(true);
    assert(outer.ready && isFetchingHttps);
    httpsAbortRequested = true;
    {
      HttpsGuard reentrant(false);
      assert(!reentrant.ready);
    }
    assert(httpsAbortRequested && isFetchingHttps && networkOwned);
    assert(releases == 0);
  }
  assert(!isFetchingHttps && !networkOwned && releases == 1);
  webArrives = true;
  {
    HttpsGuard interrupted(true);
    assert(!interrupted.ready);
    assert(httpsAbortRequested && !isFetchingHttps && !networkOwned);
  }
  assert(releases == 2);
  webArrives = webActive = false;
  {
    HttpsGuard next(true);
    assert(next.ready && !httpsAbortRequested && isFetchingHttps);
  }
  assert(!networkOwned && !isFetchingHttps && releases == 3);
}

static void heapBudgets() {
  using namespace networkMemory;
  // Both observed failing states passed the previous 6500/3200 test.
  assert(!canStartTls({8440, 4344}, {20000, 20000}, true));
  assert(!canStartTls({10000, 6800}, {20000, 20000}, true));
  assert(!canStartTls({9408, 9112}, {20000, 20000}, true));
  assert(canStartTls({14000, 10000}, {19000, 19000}, true));
  assert(!canStartTls({14000, 10000}, {6000, 4000}, true));
  assert(!canStartTls({14000, 10000}, {19000, 19000}, false));
  assert(!canStartTls({18000, 10000}, {19000, 19000}, false));
  assert(canStartTls({18000, 15000}, {19000, 19000}, false));
  assert(!canStartTls({18000, 15000}, {19000, 8000}, true));
  assert(!canStartTls({18000, 10000}, {19000, 16000}, false));
  assert(!canStartTls({18000, 6000}, {19000, 19000}, false));
}

static void deferredScheduling() {
  using namespace networkMemory;
  constexpr uint32_t interval = 180000;
  for (uint32_t now : {1000u, 500000u, UINT32_MAX - 3000}) {
    auto retry = pollAnchor(now, interval, NetworkRequestResult::Deferred);
    assert(uint32_t(now + 4999u - retry) < interval);
    assert(uint32_t(now + 5000u - retry) == interval);
    assert(pollAnchor(now, interval, NetworkRequestResult::Success) == now);
    assert(pollAnchor(now, interval, NetworkRequestResult::Failed) == now);
  }
  assert(pollAnchor(10000, 1000, NetworkRequestResult::Deferred) == 10000);
}

static void weatherResponses() {
  StaticJsonDocument<512> filter;
  makeWeatherFilter(filter);
  assert(!filter.overflowed());
  // Large unused fields must not occupy the shared parsing arena.
  std::string response = R"({"main":{"temp":-12.8,"humidity":64,"pressure":1008},)"
                         R"("wind":{"speed":4.7},"weather":[{"icon":"13n","description":")";
  response += std::string(5000, 'x');
  response += R"("}],"unused":[1,2,3]})";
  StaticJsonDocument<512> doc;
  assert(!deserializeJson(doc, response, DeserializationOption::Filter(filter)));
  WeatherData out{};
  assert(decodeWeatherData(doc.as<JsonVariantConst>(), out));
  assert(out.temperature == -12 && out.windSpeed == 4);
  assert(out.pressure == 1008 && out.humidity == 64);
  assert(std::string(out.icon) == "13n");

  auto unchanged = [&]() {
    WeatherData snapshot = out;
    assert(!decodeWeatherData(doc.as<JsonVariantConst>(), out));
    assert(out.temperature == snapshot.temperature && out.pressure == snapshot.pressure);
    assert(out.windSpeed == snapshot.windSpeed && out.humidity == snapshot.humidity);
    assert(std::string(out.icon) == snapshot.icon);
  };
  doc["main"]["humidity"] = 101;
  unchanged();
  doc["main"]["humidity"] = 64;
  doc["weather"][0]["icon"] = "bad-icon";
  unchanged();
  doc["weather"][0]["icon"] = "13n";
  doc["main"]["temp"] = std::numeric_limits<float>::infinity();
  unchanged();
  assert(!deserializeJson(doc, R"({"cod":401,"message":"Invalid API key"})"));
  unchanged();
  assert(!deserializeJson(doc, R"({"main":{"temp":15}})"));
  unchanged();
}

int main() {
  backupStreaming();
  requestOwnership();
  heapBudgets();
  deferredScheduling();
  weatherResponses();
  std::cout << "PASS segmented backups, request ownership, TLS heap budgets, deferred polling, filtered and atomic weather decoding\n";
}
