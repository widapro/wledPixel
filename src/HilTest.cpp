#include "HilTest.h"

#if defined(WLED_HIL_TEST)

#include "Globals.h"

#include <cstring>
#if defined(ESP32)
#include <nvs.h>
#endif

namespace {

constexpr char kHilBuildMarker[] = "WLEDPIXEL_HIL_TEST";
constexpr uint16_t kMaxFramebufferColumns = 16U * 8U;
constexpr unsigned long kDefaultPatternHoldMs = 8000UL;
constexpr unsigned long kMinPatternHoldMs = 1000UL;
constexpr unsigned long kMaxPatternHoldMs = 30000UL;

enum HilPattern : uint8_t {
  HIL_PATTERN_NONE = 0,
  HIL_PATTERN_CLEAR,
  HIL_PATTERN_FULL,
  HIL_PATTERN_CHECKER,
  HIL_PATTERN_DIAGONAL,
};

volatile HilPattern pendingPattern = HIL_PATTERN_NONE;
volatile unsigned long pendingHoldMs = kDefaultPatternHoldMs;
volatile bool patternPending = false;
volatile bool releasePending = false;

volatile unsigned long patternHoldUntil = 0;
volatile HilPattern activePattern = HIL_PATTERN_NONE;
uint32_t framebufferSequence = 0;
uint8_t framebufferSnapshot[kMaxFramebufferColumns] = {0};
uint16_t framebufferSnapshotColumns = 0;
uint32_t framebufferSnapshotChecksum = 0;

class ChunkWindowPrint : public Print {
public:
  ChunkWindowPrint(uint8_t *buffer, size_t capacity, size_t skip)
      : buffer_(buffer), capacity_(capacity), skip_(skip) {}

  size_t write(uint8_t value) override { return write(&value, 1); }

  size_t write(const uint8_t *data, size_t len) override {
    const size_t start = total_;
    total_ += len;
    if (!buffer_ || copied_ >= capacity_ || start + len <= skip_)
      return len;

    const size_t sourceOffset = skip_ > start ? skip_ - start : 0;
    const size_t available = len - sourceOffset;
    const size_t room = capacity_ - copied_;
    const size_t count = available < room ? available : room;
    memcpy(buffer_ + copied_, data + sourceOffset, count);
    copied_ += count;
    return len;
  }

  size_t copied() const { return copied_; }

private:
  uint8_t *buffer_;
  size_t capacity_;
  size_t skip_;
  size_t total_ = 0;
  size_t copied_ = 0;
};

const char *patternName(HilPattern pattern) {
  switch (pattern) {
  case HIL_PATTERN_CLEAR:
    return "clear";
  case HIL_PATTERN_FULL:
    return "full";
  case HIL_PATTERN_CHECKER:
    return "checker";
  case HIL_PATTERN_DIAGONAL:
    return "diagonal";
  default:
    return "none";
  }
}

HilPattern parsePattern(const String &value) {
  if (value == "clear")
    return HIL_PATTERN_CLEAR;
  if (value == "full")
    return HIL_PATTERN_FULL;
  if (value == "checker")
    return HIL_PATTERN_CHECKER;
  if (value == "diagonal")
    return HIL_PATTERN_DIAGONAL;
  return HIL_PATTERN_NONE;
}

uint8_t patternColumn(HilPattern pattern, uint16_t column) {
  switch (pattern) {
  case HIL_PATTERN_FULL:
    return 0xff;
  case HIL_PATTERN_CHECKER:
    return (column & 1U) == 0U ? 0xaa : 0x55;
  case HIL_PATTERN_DIAGONAL:
    return static_cast<uint8_t>(1U << (column & 7U));
  case HIL_PATTERN_CLEAR:
  default:
    return 0x00;
  }
}

uint32_t fnv1a(const uint8_t *data, uint16_t length) {
  uint32_t hash = 2166136261UL;
  for (uint16_t i = 0; i < length; ++i) {
    hash ^= data[i];
    hash *= 16777619UL;
  }
  return hash;
}

uint16_t displayColumnCount() {
  MD_MAX72XX *mx = P.getGraphicObject();
  if (!mx)
    return 0;
  const uint16_t columns = mx->getColumnCount();
  return columns < kMaxFramebufferColumns ? columns : kMaxFramebufferColumns;
}

void captureFramebuffer() {
  MD_MAX72XX *mx = P.getGraphicObject();
  framebufferSnapshotColumns = displayColumnCount();
  for (uint16_t column = 0; column < framebufferSnapshotColumns; ++column)
    framebufferSnapshot[column] = mx ? mx->getColumn(column) : 0;
  framebufferSnapshotChecksum =
      fnv1a(framebufferSnapshot, framebufferSnapshotColumns);
  ++framebufferSequence;
}

void applyPattern(HilPattern pattern, unsigned long holdMs,
                  unsigned long now) {
  MD_MAX72XX *mx = P.getGraphicObject();
  const uint16_t columns = displayColumnCount();
  if (!mx || columns == 0)
    return;

  // Arm the hold before touching the framebuffer. The next loop iteration
  // cannot let Parola overwrite a partly applied pattern.
  activePattern = pattern;
  patternHoldUntil = now + holdMs;
  mx->update(MD_MAX72XX::OFF);
  for (uint16_t column = 0; column < columns; ++column)
    mx->setColumn(column, patternColumn(pattern, column));
  mx->update(MD_MAX72XX::ON);
}

void releasePattern() {
  patternHoldUntil = 0;
  activePattern = HIL_PATTERN_NONE;
  for (uint8_t zone = 0; zone < zoneNumbers && zone < 4; ++zone) {
    zones[zone].forceUpdate = true;
    zones[zone].curTime = "";
    P.displayReset(zone);
  }
}

const AsyncWebParameter *findParameter(AsyncWebServerRequest *request,
                                       const char *name) {
  if (request->hasParam(name, true))
    return request->getParam(name, true);
  if (request->hasParam(name))
    return request->getParam(name);
  return nullptr;
}

void sendJson(AsyncWebServerRequest *request, int status, const char *body) {
  AsyncWebServerResponse *response =
      request->beginResponse(status, "application/json", body);
  if (response)
    request->send(response);
}

} // namespace

bool hilTestDisplayHoldActive(unsigned long now) {
  if (patternHoldUntil == 0)
    return false;
  return static_cast<int32_t>(patternHoldUntil - now) > 0;
}

void hilTestService(unsigned long now) {
  if (releasePending) {
    releasePending = false;
    patternPending = false;
    releasePattern();
    return;
  }

  if (patternHoldUntil != 0 &&
      static_cast<int32_t>(patternHoldUntil - now) <= 0) {
    releasePattern();
  }

  if (!patternPending)
    return;

  const HilPattern pattern = pendingPattern;
  const unsigned long holdMs = pendingHoldMs;
  patternPending = false;
  if (pattern != HIL_PATTERN_NONE)
    applyPattern(pattern, holdMs, now);
}

void setupHilTestRoutes() {
#if defined(ESP32)
  // Storage metadata only: never expose configuration or credential values.
  server.on("/api/hil/storage", HTTP_GET,
            [](AsyncWebServerRequest *request) {
              nvs_stats_t stats = {};
              nvs_get_stats(nullptr, &stats);
              String body = "{\"used\":" + String(stats.used_entries) +
                            ",\"free\":" + String(stats.free_entries) +
                            ",\"total\":" + String(stats.total_entries) +
                            ",\"namespaces\":" + String(stats.namespace_count) +
                            ",\"records\":[";
              bool first = true;
              for (nvs_iterator_t it = nvs_entry_find("nvs", nullptr, NVS_TYPE_ANY);
                   it; it = nvs_entry_next(it)) {
                nvs_entry_info_t info;
                nvs_entry_info(it, &info);
                if (strncmp(info.namespace_name, "cronP", 5) != 0 &&
                    strncmp(info.namespace_name, "cronB", 5) != 0) continue;
                if (!first) body += ',';
                first = false;
                size_t size = 0;
                nvs_handle_t handle;
                if (info.type == NVS_TYPE_BLOB &&
                    nvs_open(info.namespace_name, NVS_READONLY, &handle) == ESP_OK) {
                  nvs_get_blob(handle, info.key, nullptr, &size);
                  nvs_close(handle);
                }
                body += "{\"namespace\":\"" + String(info.namespace_name) +
                        "\",\"key\":\"" + String(info.key) +
                        "\",\"bytes\":" + String(size) + "}";
              }
              body += "]}";
              sendJson(request, 200, body.c_str());
            });
#endif
  server.on("/api/hil/status", HTTP_GET,
            [](AsyncWebServerRequest *request) {
              MD_MAX72XX *mx = P.getGraphicObject();
              const bool holdActive = hilTestDisplayHoldActive(millis());
              char body[512];
              const String ip = WiFi.localIP().toString();
#if defined(ESP8266)
              const char *platform = "ESP8266";
              snprintf(
                  body, sizeof(body),
                  "{\"hil\":true,\"build\":\"%s\",\"platform\":\"%s\","
                  "\"firmware\":\"%s\",\"uptimeMs\":%lu,\"heap\":%u,"
                  "\"maxBlock\":%u,\"fragmentation\":%u,"
                  "\"wifiConnected\":%s,\"ip\":\"%s\",\"mqttEnabled\":%s,"
                  "\"mqttConnected\":%s,\"power\":%s,\"intensity\":%u,"
                  "\"zoneCount\":%u,\"displayDevices\":%u,"
                  "\"displayColumns\":%u,\"bootComplete\":%s,"
                  "\"pattern\":\"%s\",\"patternHold\":%s,"
                  "\"patternPending\":%s}",
                  kHilBuildMarker, platform, firmwareVer, millis(),
                  ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                  ESP.getHeapFragmentation(),
                  WiFi.status() == WL_CONNECTED ? "true" : "false", ip.c_str(),
                  mqttEnable ? "true" : "false",
                  mqttClient.connected() ? "true" : "false",
                  power ? "true" : "false", intensity + 1U, zoneNumbers,
                  mx ? mx->getDeviceCount() : 0,
                  mx ? mx->getColumnCount() : 0,
                  allTestsFinish ? "true" : "false", patternName(activePattern),
                  holdActive ? "true" : "false",
                  patternPending ? "true" : "false");
#else
              const char *platform = "ESP32";
              snprintf(
                  body, sizeof(body),
                  "{\"hil\":true,\"build\":\"%s\",\"platform\":\"%s\","
                  "\"firmware\":\"%s\",\"uptimeMs\":%lu,\"heap\":%u,"
                  "\"wifiConnected\":%s,\"ip\":\"%s\",\"mqttEnabled\":%s,"
                  "\"mqttConnected\":%s,\"power\":%s,\"intensity\":%u,"
                  "\"zoneCount\":%u,\"displayDevices\":%u,"
                  "\"displayColumns\":%u,\"bootComplete\":%s,"
                  "\"pattern\":\"%s\",\"patternHold\":%s,"
                  "\"patternPending\":%s}",
                  kHilBuildMarker, platform, firmwareVer, millis(),
                  ESP.getFreeHeap(),
                  WiFi.status() == WL_CONNECTED ? "true" : "false", ip.c_str(),
                  mqttEnable ? "true" : "false",
                  mqttClient.connected() ? "true" : "false",
                  power ? "true" : "false", intensity + 1U, zoneNumbers,
                  mx ? mx->getDeviceCount() : 0,
                  mx ? mx->getColumnCount() : 0,
                  allTestsFinish ? "true" : "false", patternName(activePattern),
                  holdActive ? "true" : "false",
                  patternPending ? "true" : "false");
#endif
              sendJson(request, 200, body);
            });

  server.on("/api/hil/framebuffer", HTTP_GET,
            [](AsyncWebServerRequest *request) {
              captureFramebuffer();
              AsyncWebServerResponse *response = request->beginChunkedResponse(
                  "application/json",
                  [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                    ChunkWindowPrint out(buffer, maxLen, index);
                    out.printf("{\"sequence\":%lu,\"rows\":8,\"columns\":%u,"
                               "\"checksum\":%lu,\"data\":[",
                               static_cast<unsigned long>(framebufferSequence),
                               framebufferSnapshotColumns,
                               static_cast<unsigned long>(
                                   framebufferSnapshotChecksum));
                    for (uint16_t column = 0;
                         column < framebufferSnapshotColumns; ++column) {
                      if (column > 0)
                        out.print(',');
                      out.print(framebufferSnapshot[column]);
                    }
                    out.print("]}");
                    return out.copied();
                  });
              if (response)
                request->send(response);
            });

  server.on("/api/hil/pattern", HTTP_POST,
            [](AsyncWebServerRequest *request) {
              const AsyncWebParameter *patternParam =
                  findParameter(request, "pattern");
              if (!patternParam) {
                sendJson(request, 400,
                         "{\"error\":\"missing_pattern\"}");
                return;
              }

              const HilPattern pattern = parsePattern(patternParam->value());
              if (pattern == HIL_PATTERN_NONE) {
                sendJson(request, 400,
                         "{\"error\":\"unsupported_pattern\"}");
                return;
              }

              unsigned long holdMs = kDefaultPatternHoldMs;
              const AsyncWebParameter *holdParam =
                  findParameter(request, "holdMs");
              if (holdParam) {
                const long parsed = holdParam->value().toInt();
                if (parsed > 0)
                  holdMs = static_cast<unsigned long>(parsed);
              }
              if (holdMs < kMinPatternHoldMs)
                holdMs = kMinPatternHoldMs;
              if (holdMs > kMaxPatternHoldMs)
                holdMs = kMaxPatternHoldMs;

              pendingPattern = pattern;
              pendingHoldMs = holdMs;
              releasePending = false;
              patternPending = true;

              char body[160];
              snprintf(body, sizeof(body),
                       "{\"accepted\":true,\"pattern\":\"%s\","
                       "\"holdMs\":%lu,\"persistent\":false}",
                       patternName(pattern), holdMs);
              sendJson(request, 202, body);
            });

  server.on("/api/hil/release", HTTP_POST,
            [](AsyncWebServerRequest *request) {
              releasePending = true;
              sendJson(request, 202,
                       "{\"accepted\":true,\"persistent\":false}");
            });
}

#endif
