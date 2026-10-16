#include "WebRoutes.h"
#include "ConfigStore.h"
#include "DisplayUtils.h"
#include "HilTest.h"
#include "MqttHandler.h"
#include "Scheduler.h"
#include "ProfileJson.h"
#include "ProfileRecordStore.h"
#include "WebPriority.h"
#include <cstdio>
#include <cmath>
#include <httpsRequestJson.h>

#if defined(ESP8266)
#include <LittleFS.h>
#endif

namespace {
constexpr uint8_t kZoneCapacity = 4;

// File-scope statics for cronProfile request handling
// (needs to be accessible in both param-processing loop and post-loop save)
static uint8_t s_cronProfIdx = 0;
static bool s_cronDraftActive = false;
static uint8_t s_cronDraftIdx = 0;
static uint32_t s_cronDraftGeneration = 0;

#if defined(ESP8266)
constexpr const char *kRestoreTempPath = "/restore.tmp";
File restoreUploadFile;
bool restoreUploadFailed = false;
size_t restoreUploadBytes = 0;
unsigned long restoreRebootAt = 0;
uint8_t restoreState = 0; // 0 idle, 1 queued, 2 running, 3 succeeded, 4 failed
#endif

// Print adapter used by AsyncChunkedResponse. The JSON writer can render from
// the beginning on every callback while this adapter only copies the requested
// byte window. Responses are never accumulated in a heap-backed String.
class ChunkWindowPrint : public Print {
public:
  ChunkWindowPrint(uint8_t *buffer, size_t capacity, size_t skip)
      : buffer_(buffer), capacity_(capacity), skip_(skip) {}

  size_t write(uint8_t value) override { return write(&value, 1); }

  size_t write(const uint8_t *data, size_t len) override {
    size_t start = total_;
    total_ += len;
    if (!buffer_ || copied_ >= capacity_ || start + len <= skip_)
      return len;

    size_t sourceOffset = (skip_ > start) ? (skip_ - start) : 0;
    size_t available = len - sourceOffset;
    size_t room = capacity_ - copied_;
    size_t count = available < room ? available : room;
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

int parseProfileIndex(const String &value, int limit) {
  if (value.length() == 0) return -1;
  unsigned parsed = 0;
  for (size_t i = 0; i < value.length(); ++i) {
    if (value[i] < '0' || value[i] > '9') return -1;
    parsed = parsed * 10 + (value[i] - '0');
    if (parsed >= static_cast<unsigned>(limit)) return -1;
  }
  return static_cast<int>(parsed);
}
bool ownsProfileEdit(AsyncWebServerRequest *request, uint8_t idx) {
  if (request->hasHeader("X-Profile-Edit") &&
      schedulerOwnsEdit(idx, request->getHeader("X-Profile-Edit")->value().c_str())) return true;
  request->send(409, "application/json", "{\"error\":\"Start a profile edit session first\"}");
  return false;
}

void addApiResponseHeaders(AsyncWebServerResponse *response) {
  // Cache-Control and Connection are installed once through DefaultHeaders.
  // Keeping this helper makes the intent explicit without allocating duplicate
  // header nodes for every response.
  (void)response;
}

void addStaticPageHeaders(AsyncWebServerResponse *response) {
  response->addHeader("Content-Encoding", "gzip");
}

bool canAllocateSmallResponse() {
#if defined(ESP8266)
  return ESP.getFreeHeap() >= 2048 && ESP.getMaxFreeBlockSize() >= 768;
#else
  return true;
#endif
}

bool webHeapIsCritical() { return !canAllocateSmallResponse(); }

void sendSmallBusyResponse(AsyncWebServerRequest *request,
                           const char *contentType, const char *body) {
  if (!request)
    return;

#if defined(ESP8266)
  if (!canAllocateSmallResponse()) {
    return;
  }
#endif

  AsyncWebServerResponse *response =
      request->beginResponse(503, contentType, body);
  if (!response)
    return;
  response->addHeader("Retry-After", "1");
  addApiResponseHeaders(response);
  request->send(response);
}

void sendBusyRetryPage(AsyncWebServerRequest *request) {
  sendSmallBusyResponse(request, "text/plain", "Busy");
}

void sendBusyJson(AsyncWebServerRequest *request) {
  sendSmallBusyResponse(request, "application/json", "{\"error\":\"busy\"}");
}

void sendConcurrentApiJson(AsyncWebServerRequest *request) {
  if (!request || !canAllocateSmallResponse())
    return;
  AsyncWebServerResponse *response = request->beginResponse(
      429, "application/json", "{\"error\":\"api_in_progress\"}");
  if (!response)
    return;
  response->addHeader("Retry-After", "1");
  addApiResponseHeaders(response);
  request->send(response);
}

bool canServeProgmemPage() {
#if defined(ESP8266)
  return ESP.getFreeHeap() >= 1400 && ESP.getMaxFreeBlockSize() >= 768;
#else
  return true;
#endif
}

void sendProgmemPage(AsyncWebServerRequest *request, const char *contentType,
                     const uint8_t *page, size_t pageLen) {
  if (!canServeProgmemPage()) {
#if defined(ESP8266)
    Serial.printf("\n[WEB] Static busy. Heap=%u MaxBlock=%u TLS=%u",
                  ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                  isFetchingHttps ? 1 : 0);
#endif
    sendBusyRetryPage(request);
    return;
  }

  AsyncWebServerResponse *response =
      request->beginResponse(200, contentType, page, pageLen);
  if (!response) {
#if defined(ESP8266)
    Serial.printf("\n[WEB] Static response alloc failed. Heap=%u MaxBlock=%u",
                  ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
#endif
    sendBusyRetryPage(request);
    return;
  }
  addStaticPageHeaders(response);
  request->send(response);
}

bool parseBoolParam(const String &value) {
  return value.equalsIgnoreCase("true") || value.equalsIgnoreCase("on") ||
         value.equalsIgnoreCase("yes") || value == "1";
}

int8_t detectZoneFromParamName(const String &name) {
  int zonePos = name.indexOf("zone");
  if (zonePos < 0)
    zonePos = name.indexOf("Zone");
  if (zonePos < 0 || zonePos + 4 >= (int)name.length())
    return -1;

  char c = name.charAt(zonePos + 4);
  if (c < '0' || c >= static_cast<char>('0' + kZoneCapacity))
    return -1;

  return static_cast<int8_t>(c - '0');
}

bool resolveZoneIndexFromRequest(AsyncWebServerRequest *request,
                                 uint8_t &zoneOut) {
  zoneOut = kZoneCapacity;
  int params = request->params();
  for (int i = 0; i < params; i++) {
    const AsyncWebParameter *p = request->getParam((size_t)i);
    if (!p || !p->isPost() || p->name() == "key")
      continue;

    if (p->name() == "zone") {
      int parsed = p->value().toInt();
      if (parsed >= 0 && parsed < kZoneCapacity) {
        zoneOut = static_cast<uint8_t>(parsed);
        return true;
      }
      continue;
    }

    int8_t fromName = detectZoneFromParamName(p->name());
    if (fromName >= 0)
      zoneOut = static_cast<uint8_t>(fromName);
  }

  return zoneOut < kZoneCapacity;
}

template <typename T>
void assignIfPresent(const JsonDocument &doc, const char *key, T &target) {
  if (doc.containsKey(key))
    target = doc[key];
}

void assignIfPresent(const JsonDocument &doc, const char *key, String &target) {
  if (doc.containsKey(key))
    target = restoredLiveString<String>(doc[key]);
}

template <typename T>
void assignIfPresent(const JsonObjectConst &doc, const char *key, T &target) {
  if (doc.containsKey(key))
    target = doc[key];
}

void writeJsonEscaped(Print &out, const char *value) {
  out.print('"');
  if (value) {
    while (*value) {
      uint8_t c = static_cast<uint8_t>(*value++);
      switch (c) {
      case '"':
        out.print("\\\"");
        break;
      case '\\':
        out.print("\\\\");
        break;
      case '\b':
        out.print("\\b");
        break;
      case '\f':
        out.print("\\f");
        break;
      case '\n':
        out.print("\\n");
        break;
      case '\r':
        out.print("\\r");
        break;
      case '\t':
        out.print("\\t");
        break;
      default:
        if (c < 0x20) {
          char escaped[7];
          snprintf(escaped, sizeof(escaped), "\\u%04x", c);
          out.print(escaped);
        } else {
          out.write(c);
        }
        break;
      }
    }
  }
  out.print('"');
}

void writeJsonEscaped(Print &out, const String &value) {
  writeJsonEscaped(out, value.c_str());
}

void writeJsonKey(Print &out, const char *key, bool &first) {
  if (!first)
    out.print(',');
  first = false;
  out.print('"');
  out.print(key);
  out.print("\":");
}

void writeJsonStringField(Print &out, const char *key, const String &value,
                          bool &first) {
  writeJsonKey(out, key, first);
  writeJsonEscaped(out, value);
}

void writeJsonCStringField(Print &out, const char *key, const char *value,
                           bool &first) {
  writeJsonKey(out, key, first);
  writeJsonEscaped(out, value);
}

void writeJsonBoolField(Print &out, const char *key, bool value, bool &first) {
  writeJsonKey(out, key, first);
  out.print(value ? "true" : "false");
}

template <typename T>
void writeJsonNumberField(Print &out, const char *key, T value, bool &first) {
  writeJsonKey(out, key, first);
  out.print(value);
}

// Preserve float settings in backups instead of Print's two-decimal default.
void writeJsonNumberField(Print &out, const char *key, float value, bool &first) {
  writeJsonKey(out, key, first);
  if (!std::isfinite(value)) {
    out.print("null");
    return;
  }
  char number[32];
  snprintf(number, sizeof(number), "%.9g", static_cast<double>(value));
  out.print(number);
}

void writeScheduleU8Array(Print &out, const char *key,
                          const uint8_t values[4]) {
  out.print('"');
  out.print(key);
  out.print("\":[");
  for (uint8_t i = 0; i < 4; i++) {
    if (i)
      out.print(',');
    out.print(values[i]);
  }
  out.print("],");
}

template <size_t N>
void writeScheduleStringArray(Print &out, const char *key,
                              const char (&values)[4][N]) {
  out.print('"');
  out.print(key);
  out.print("\":[");
  for (uint8_t i = 0; i < 4; i++) {
    if (i)
      out.print(',');
    if ((uint8_t)values[i][0] == SCHED_STR_NO_CHANGE)
      out.print("null");
    else {
      char tmp[N];
      memcpy(tmp, values[i], N);
      tmp[N - 1] = '\0';
      writeJsonEscaped(out, tmp);
    }
  }
  out.print("],");
}

template <size_t N>
void writeScheduleJsonMaybeString(Print &out, const char (&value)[N]) {
  if ((uint8_t)value[0] == SCHED_STR_NO_CHANGE) {
    out.print("null");
    return;
  }
  char tmp[N];
  memcpy(tmp, value, N);
  tmp[N - 1] = '\0';
  writeJsonEscaped(out, tmp);
}

void copyScheduleStringParam(char *target, size_t targetSize,
                             const String &value) {
  if (!target || targetSize == 0)
    return;
  if (value == "null" || value == "\xFF") {
    target[0] = SCHED_STR_NO_CHANGE;
    if (targetSize > 1)
      target[1] = '\0';
    return;
  }
  value.toCharArray(target, targetSize);
}

bool applyCronProfileParam(ScheduleProfile &profile,
                           const AsyncWebParameter &param) {
  const char *name = param.name().c_str();
  const String &value = param.value();

  if (strcmp(name, "idx") == 0)
    return true;
  if (strcmp(name, "reset") == 0)
    return true;
  if (strcmp(name, "enabled") == 0)
    profile.enabled = parseBoolParam(value);
  else if (strcmp(name, "minute") == 0)
    profile.minute = value.toInt();
  else if (strcmp(name, "hour") == 0)
    profile.hour = value.toInt();
  else if (strcmp(name, "dayOfWeek") == 0)
    profile.dayOfWeek = value.toInt();
  else if (strcmp(name, "dayOfMonth") == 0)
    profile.dayOfMonth = strtoul(value.c_str(), nullptr, 10);
  else if (strcmp(name, "power") == 0)
    profile.power = value.toInt();
  else if (strcmp(name, "intensity") == 0)
    profile.intensity = value.toInt();
  else if (strcmp(name, "zoneCount") == 0)
    profile.zoneCount = value.toInt();
  else if (strcmp(name, "pbEnable") == 0)
    profile.pbEnable = value.toInt();
  else if (strcmp(name, "scrollInfinite") == 0)
    profile.zoneScrollInfinite = value.toInt();
  else if (strcmp(name, "countdownShowUnits") == 0)
    profile.zoneCountdownShowUnits = value.toInt();
  else if (strcmp(name, "stockShowArrows") == 0)
    profile.zoneStockShowArrows = value.toInt();
  else if (strcmp(name, "name") == 0) {
    value.toCharArray(profile.name, sizeof(profile.name));
  } else {
    size_t length = strlen(name);
    if (length < 2 || length >= 16)
      return false;
    char zoneChar = name[length - 1];
    if (zoneChar < '0' || zoneChar >= '0' + kZoneCapacity)
      return false;
    uint8_t zone = static_cast<uint8_t>(zoneChar - '0');
    char field[16];
    memcpy(field, name, length - 1);
    field[length - 1] = '\0';

    if (strcmp(field, "zb") == 0)
      profile.zoneBegin[zone] = value.toInt();
    else if (strcmp(field, "ze") == 0)
      profile.zoneEnd[zone] = value.toInt();
    else if (strcmp(field, "zwm") == 0)
      profile.zoneWorkMode[zone] = value.toInt();
    else if (strcmp(field, "zod") == 0)
      profile.zoneOwmDisplay[zone] = value.toInt();
    else if (strcmp(field, "zcf") == 0)
      profile.zoneClockFmt[zone] = value.toInt();
    else if (strcmp(field, "zf") == 0)
      profile.zoneFont[zone] = value.toInt();
    else if (strcmp(field, "zspd") == 0)
      profile.zoneScrollSpeed[zone] = value.toInt();
    else if (strcmp(field, "zpause") == 0)
      profile.zoneScrollPause[zone] = value.toInt();
    else if (strcmp(field, "zchar") == 0)
      profile.zoneCharspacing[zone] = value.toInt();
    else if (strcmp(field, "zhi") == 0)
      copyScheduleStringParam(profile.zoneHaSensorId[zone],
                              sizeof(profile.zoneHaSensorId[zone]), value);
    else if (strcmp(field, "zhp") == 0)
      copyScheduleStringParam(profile.zoneHaPostfix[zone],
                              sizeof(profile.zoneHaPostfix[zone]), value);
    else if (strcmp(field, "zmt") == 0)
      copyScheduleStringParam(profile.zoneMqttTopic[zone],
                              sizeof(profile.zoneMqttTopic[zone]), value);
    else if (strcmp(field, "zalign") == 0)
      copyScheduleStringParam(profile.zoneScrollAlign[zone],
                              sizeof(profile.zoneScrollAlign[zone]), value);
    else if (strcmp(field, "zefin") == 0)
      copyScheduleStringParam(profile.zoneScrollEffectIn[zone],
                              sizeof(profile.zoneScrollEffectIn[zone]), value);
    else if (strcmp(field, "zefout") == 0)
      copyScheduleStringParam(profile.zoneScrollEffectOut[zone],
                              sizeof(profile.zoneScrollEffectOut[zone]), value);
    else if (strcmp(field, "zmp") == 0)
      copyScheduleStringParam(profile.zoneMqttPostfix[zone],
                              sizeof(profile.zoneMqttPostfix[zone]), value);
    else if (strcmp(field, "zds") == 0)
      copyScheduleStringParam(profile.zoneDs18b20Postfix[zone],
                              sizeof(profile.zoneDs18b20Postfix[zone]), value);
    else if (strcmp(field, "zct") == 0)
      copyScheduleStringParam(profile.zoneCountdownTarget[zone],
                              sizeof(profile.zoneCountdownTarget[zone]), value);
    else if (strcmp(field, "zcfmt") == 0)
      copyScheduleStringParam(profile.zoneCountdownFormat[zone],
                              sizeof(profile.zoneCountdownFormat[zone]), value);
    else if (strcmp(field, "zcfin") == 0)
      copyScheduleStringParam(profile.zoneCountdownFinish[zone],
                              sizeof(profile.zoneCountdownFinish[zone]), value);
    else if (strcmp(field, "zcpre") == 0)
      copyScheduleStringParam(profile.zoneCountdownPrefix[zone],
                              sizeof(profile.zoneCountdownPrefix[zone]), value);
    else if (strcmp(field, "zcsuf") == 0)
      copyScheduleStringParam(profile.zoneCountdownSuffix[zone],
                              sizeof(profile.zoneCountdownSuffix[zone]), value);
    else if (strcmp(field, "zstk") == 0)
      copyScheduleStringParam(profile.zoneStockSymbols[zone],
                              sizeof(profile.zoneStockSymbols[zone]), value);
    else if (strcmp(field, "zstfmt") == 0)
      copyScheduleStringParam(profile.zoneStockDisplayFormat[zone],
                              sizeof(profile.zoneStockDisplayFormat[zone]),
                              value);
    else if (strcmp(field, "zstpre") == 0)
      copyScheduleStringParam(profile.zoneStockPrefix[zone],
                              sizeof(profile.zoneStockPrefix[zone]), value);
    else if (strcmp(field, "zstpost") == 0)
      copyScheduleStringParam(profile.zoneStockPostfix[zone],
                              sizeof(profile.zoneStockPostfix[zone]), value);
    else
      return false;
  }
  return true;
}

void writeZoneSettingsFields(Print &out, uint8_t zone, bool &first) {
  char key[48];

  snprintf(key, sizeof(key), "zone%uBegin", zone);
  writeJsonNumberField(out, key, zones[zone].begin, first);
  snprintf(key, sizeof(key), "zone%uEnd", zone);
  writeJsonNumberField(out, key, zones[zone].end, first);

  snprintf(key, sizeof(key), "workModeZone%u", zone);
  writeJsonStringField(out, key, zones[zone].workMode, first);
  snprintf(key, sizeof(key), "scrollSpeedZone%u", zone);
  writeJsonNumberField(out, key, zones[zone].scrollSpeed, first);
  snprintf(key, sizeof(key), "scrollPauseZone%u", zone);
  writeJsonNumberField(out, key, zones[zone].scrollPause, first);
  snprintf(key, sizeof(key), "scrollAlignZone%u", zone);
  writeJsonStringField(out, key, zones[zone].scrollAlign, first);
  snprintf(key, sizeof(key), "scrollEffectZone%uIn", zone);
  writeJsonStringField(out, key, zones[zone].scrollEffectIn, first);
  snprintf(key, sizeof(key), "scrollEffectZone%uOut", zone);
  writeJsonStringField(out, key, zones[zone].scrollEffectOut, first);
  snprintf(key, sizeof(key), "charspacingZone%u", zone);
  writeJsonNumberField(out, key, zones[zone].charspacing, first);
  snprintf(key, sizeof(key), "fontZone%u", zone);
  writeJsonStringField(out, key, zones[zone].font, first);
  snprintf(key, sizeof(key), "mqttTextTopicZone%u", zone);
  writeJsonStringField(out, key, MQTTZones[zone].message, first);
  snprintf(key, sizeof(key), "mqttPostfixZone%u", zone);
  writeJsonStringField(out, key, zones[zone].mqttPostfix, first);
  snprintf(key, sizeof(key), "clockDisplayFormatZone%u", zone);
  writeJsonStringField(out, key, zones[zone].clockDisplayFormat, first);
  snprintf(key, sizeof(key), "owmWhatToDisplayZone%u", zone);
  writeJsonStringField(out, key, zones[zone].owmWhatToDisplay, first);
  snprintf(key, sizeof(key), "haSensorIdZone%u", zone);
  writeJsonStringField(out, key, zones[zone].haSensorId, first);
  snprintf(key, sizeof(key), "haSensorPostfixZone%u", zone);
  writeJsonStringField(out, key, zones[zone].haSensorPostfix, first);
  snprintf(key, sizeof(key), "ds18b20PostfixZone%u", zone);
  writeJsonStringField(out, key, zones[zone].ds18b20Postfix, first);
  snprintf(key, sizeof(key), "scrollInfiniteZone%u", zone);
  writeJsonBoolField(out, key, zones[zone].scrollInfinite, first);

  snprintf(key, sizeof(key), "countdownFormatZone%u", zone);
  writeJsonStringField(out, key, zones[zone].countdownFormat, first);
  snprintf(key, sizeof(key), "countdownFinishZone%u", zone);
  writeJsonStringField(out, key, zones[zone].countdownFinish, first);
  snprintf(key, sizeof(key), "countdownShowUnitsZone%u", zone);
  writeJsonBoolField(out, key, zones[zone].countdownShowUnits, first);
  snprintf(key, sizeof(key), "countdownPrefixZone%u", zone);
  writeJsonStringField(out, key, zones[zone].countdownPrefix, first);
  snprintf(key, sizeof(key), "countdownSuffixZone%u", zone);
  writeJsonStringField(out, key, zones[zone].countdownSuffix, first);
  snprintf(key, sizeof(key), "countdownTargetZone%u", zone);
  writeJsonStringField(out, key, countdownState[zone].targetStr, first);

  snprintf(key, sizeof(key), "stockSymbolsZone%u", zone);
  writeJsonStringField(out, key, zones[zone].stockSymbols, first);
  snprintf(key, sizeof(key), "stockDisplayFormatZone%u", zone);
  writeJsonStringField(out, key, zones[zone].stockDisplayFormat, first);
  snprintf(key, sizeof(key), "stockPrefixZone%u", zone);
  writeJsonStringField(out, key, zones[zone].stockPrefix, first);
  snprintf(key, sizeof(key), "stockPostfixZone%u", zone);
  writeJsonStringField(out, key, zones[zone].stockPostfix, first);
  snprintf(key, sizeof(key), "stockShowArrowsZone%u", zone);
  writeJsonBoolField(out, key, zones[zone].stockShowArrows, first);
}

void writeProgressBarSettingsFields(Print &out, uint8_t zone, bool &first) {
  char key[28];
#define PB_BOOL(name, value)                                                     \
  snprintf(key, sizeof(key), name "Zone%u", zone);                             \
  writeJsonBoolField(out, key, value, first)
#define PB_NUM(name, value)                                                      \
  snprintf(key, sizeof(key), name "Zone%u", zone);                             \
  writeJsonNumberField(out, key, value, first)
#define PB_STR(name, value)                                                      \
  snprintf(key, sizeof(key), name "Zone%u", zone);                             \
  writeJsonStringField(out, key, value, first)

  PB_BOOL("pbEnable", progressBars[zone].enabled);
  PB_STR("pbSrcType", progressBars[zone].dataSourceType);
  PB_STR("pbSrcId", progressBars[zone].dataSourceId);
  PB_NUM("pbMin", progressBars[zone].minValue);
  PB_NUM("pbMax", progressBars[zone].maxValue);
  PB_BOOL("pbCondEn", progressBars[zone].conditionEnabled);
  PB_STR("pbCondSrcT", progressBars[zone].conditionSourceType);
  PB_STR("pbCondSrcI", progressBars[zone].conditionSourceId);
  PB_STR("pbCondVal", progressBars[zone].conditionValue);
  PB_BOOL("pbDynMode", progressBars[zone].dynamicMode);
  PB_NUM("pbDynAnim", progressBars[zone].dynamicAnimation);
  PB_NUM("pbDynSpd", progressBars[zone].dynamicSpeedMs);
  PB_STR("pbDynSrcT", progressBars[zone].dynSourceType);
  PB_STR("pbDynSrcI", progressBars[zone].dynSourceId);
  PB_STR("pbDynR1Val", progressBars[zone].dynRule1Value);
  PB_STR("pbDynR2Val", progressBars[zone].dynRule2Value);
  PB_NUM("pbDynR2Anim", progressBars[zone].dynRule2Animation);
  PB_NUM("pbDynR2Spd", progressBars[zone].dynRule2SpeedMs);

#undef PB_BOOL
#undef PB_NUM
#undef PB_STR
}

void writeScheduleProgressBarJson(Print &out, const ScheduleProgressBar &pb,
                                  bool inherited) {
  bool first = true;
  out.print('{');
  writeJsonBoolField(out, "inherited", inherited, first);
  writeJsonCStringField(out, "dataSourceType", pb.dataSourceType, first);
  writeJsonCStringField(out, "dataSourceId", pb.dataSourceId, first);
  writeJsonNumberField(out, "minValue", pb.minValue, first);
  writeJsonNumberField(out, "maxValue", pb.maxValue, first);
  writeJsonBoolField(out, "conditionEnabled", pb.conditionEnabled, first);
  writeJsonCStringField(out, "conditionSourceType",
                        pb.conditionSourceType, first);
  writeJsonCStringField(out, "conditionSourceId", pb.conditionSourceId, first);
  writeJsonCStringField(out, "conditionValue", pb.conditionValue, first);
  writeJsonBoolField(out, "dynamicMode", pb.dynamicMode, first);
  writeJsonNumberField(out, "dynamicAnimation", pb.dynamicAnimation, first);
  writeJsonNumberField(out, "dynamicSpeedMs", pb.dynamicSpeedMs, first);
  writeJsonCStringField(out, "dynSourceType", pb.dynSourceType, first);
  writeJsonCStringField(out, "dynSourceId", pb.dynSourceId, first);
  writeJsonCStringField(out, "dynRule1Value", pb.dynRule1Value, first);
  writeJsonCStringField(out, "dynRule2Value", pb.dynRule2Value, first);
  writeJsonNumberField(out, "dynRule2Animation", pb.dynRule2Animation, first);
  writeJsonNumberField(out, "dynRule2SpeedMs", pb.dynRule2SpeedMs, first);
  out.print('}');
}

void copyProgressBarParam(char *dest, size_t size,
                          const AsyncWebParameter *param) {
  if (!dest || size == 0 || !param)
    return;
  strncpy(dest, param->value().c_str(), size - 1);
  dest[size - 1] = '\0';
}

void writeServiceSettingsJson(Print &out) {
  bool first = true;
  out.print('{');
  writeJsonStringField(out, "deviceName", shortMACaddr, first);
#if defined(ESP32)
  writeJsonCStringField(out, "platform", "ESP32", first);
#elif defined(ESP8266)
  writeJsonCStringField(out, "platform", "ESP8266", first);
#else
  writeJsonCStringField(out, "platform", "Unknown", first);
#endif
  writeJsonCStringField(out, "firmwareVer", firmwareVer, first);
  writeJsonBoolField(out, "disableServiceMessages", disableServiceMessages,
                     first);
  writeJsonBoolField(out, "displayFlip", displayFlip, first);
  writeJsonBoolField(out, "disableDotsBlink", disableDotsBlink, first);
  writeJsonStringField(out, "wifiSsid", WiFi.SSID(), first);
  writeJsonStringField(out, "wifiIp", WiFi.localIP().toString(), first);
  writeJsonStringField(out, "wifiGateway", WiFi.gatewayIP().toString(), first);
  writeJsonNumberField(out, "wifiRssi", WiFi.RSSI(), first);
  writeJsonBoolField(out, "mqttEnable", mqttEnable, first);
  writeJsonBoolField(out, "mqttConnected", mqttClient.connected(), first);
  writeJsonStringField(out, "mqttServerAddress", mqttServerAddress, first);
  writeJsonNumberField(out, "mqttServerPort", mqttServerPort, first);
  writeJsonStringField(out, "mqttUsername", mqttUsername, first);
  writeJsonCStringField(out, "mqttPassword",
                        mqttPassword.length() ? "********" : "", first);
  writeJsonStringField(out, "mqttDevicePrefix", MQTTGlobalPrefix, first);
  writeJsonNumberField(out, "ntpTimeZone", ntpTimeZone, first);
  writeJsonNumberField(out, "ntpUpdateInterval", ntpUpdateInterval, first);
  writeJsonStringField(out, "ntpServer", ntpServer, first);
  writeJsonCStringField(out, "owmApiToken",
                        owmApiToken.length() ? "********" : "", first);
  writeJsonStringField(out, "owmUnitsFormat", owmUnitsFormat, first);
  writeJsonNumberField(out, "owmUpdateInterval", owmUpdateInterval, first);
  writeJsonStringField(out, "owmCity", owmCity, first);
  writeJsonStringField(out, "haAddr", haAddr, first);
  writeJsonStringField(out, "haApiHttpType", haApiHttpType, first);
  writeJsonCStringField(out, "haApiToken",
                        haApiToken.length() ? "********" : "", first);
  writeJsonNumberField(out, "haApiPort", haApiPort, first);
  writeJsonNumberField(out, "haUpdateInterval", haUpdateInterval, first);
  writeJsonBoolField(out, "ds18b20Enable", ds18b20Enable, first);
  writeJsonStringField(out, "ds18b20Temp",
                       dsTemp == "-127" ? String("Not connected") : dsTemp,
                       first);
  writeJsonNumberField(out, "ds18b20UpdateInterval", ds18b20UpdateInterval,
                       first);
  writeJsonStringField(out, "ds18b20UnitsFormat", ds18b20UnitsFormat, first);
  writeJsonCStringField(out, "stockApiToken",
                        stockApiToken.length() ? "********" : "", first);
  writeJsonNumberField(out, "stockUpdateInterval", stockUpdateInterval, first);
  writeJsonNumberField(out, "zoneNumbers", zoneNumbers, first);
  out.print('}');
}

void writeScheduleProfileJson(Print &out, const ScheduleProfile &p,
                              uint8_t index) {
  out.print('{');
  out.printf("\"enabled\":%s,", p.enabled ? "true" : "false");
  out.printf("\"minute\":%u,\"hour\":%u,\"dayOfWeek\":%u,",
             p.minute, p.hour, p.dayOfWeek);
  out.printf("\"dayOfMonth\":%lu,\"power\":%u,\"intensity\":%u,",
             (unsigned long)p.dayOfMonth, p.power, p.intensity);
  out.printf("\"zoneCount\":%u,\"pbEnable\":%u,", p.zoneCount,
             p.pbEnable);
  out.printf("\"zoneBegin\":[%u,%u,%u,%u],", p.zoneBegin[0], p.zoneBegin[1],
             p.zoneBegin[2], p.zoneBegin[3]);
  out.printf("\"zoneEnd\":[%u,%u,%u,%u],", p.zoneEnd[0], p.zoneEnd[1],
             p.zoneEnd[2], p.zoneEnd[3]);
  out.printf("\"zoneWorkMode\":[%u,%u,%u,%u],", p.zoneWorkMode[0],
             p.zoneWorkMode[1], p.zoneWorkMode[2], p.zoneWorkMode[3]);
  out.printf("\"zoneOwmDisplay\":[%u,%u,%u,%u],", p.zoneOwmDisplay[0],
             p.zoneOwmDisplay[1], p.zoneOwmDisplay[2], p.zoneOwmDisplay[3]);
  out.printf("\"zoneClockFmt\":[%u,%u,%u,%u],", p.zoneClockFmt[0],
             p.zoneClockFmt[1], p.zoneClockFmt[2], p.zoneClockFmt[3]);
  out.printf("\"zoneFont\":[%u,%u,%u,%u],", p.zoneFont[0], p.zoneFont[1],
             p.zoneFont[2], p.zoneFont[3]);
  out.print("\"zoneHaSensorId\":[");
  for (uint8_t z = 0; z < 4; z++) {
    if (z)
      out.print(',');
    writeScheduleJsonMaybeString(out, p.zoneHaSensorId[z]);
  }
  out.print("],\"zoneHaPostfix\":[");
  for (uint8_t z = 0; z < 4; z++) {
    if (z)
      out.print(',');
    writeScheduleJsonMaybeString(out, p.zoneHaPostfix[z]);
  }
  out.print("],\"zoneMqttTopic\":[");
  for (uint8_t z = 0; z < 4; z++) {
    if (z)
      out.print(',');
    writeScheduleJsonMaybeString(out, p.zoneMqttTopic[z]);
  }
  out.print("],");
  writeScheduleU8Array(out, "zoneScrollSpeed", p.zoneScrollSpeed);
  writeScheduleU8Array(out, "zoneScrollPause", p.zoneScrollPause);
  writeScheduleU8Array(out, "zoneCharspacing", p.zoneCharspacing);
  out.printf("\"zoneScrollInfinite\":%u,", p.zoneScrollInfinite);
  out.printf("\"zoneCountdownShowUnits\":%u,", p.zoneCountdownShowUnits);
  out.printf("\"zoneStockShowArrows\":%u,", p.zoneStockShowArrows);
  writeScheduleStringArray(out, "zoneScrollAlign", p.zoneScrollAlign);
  writeScheduleStringArray(out, "zoneScrollEffectIn", p.zoneScrollEffectIn);
  writeScheduleStringArray(out, "zoneScrollEffectOut", p.zoneScrollEffectOut);
  writeScheduleStringArray(out, "zoneMqttPostfix", p.zoneMqttPostfix);
  writeScheduleStringArray(out, "zoneDs18b20Postfix", p.zoneDs18b20Postfix);
  writeScheduleStringArray(out, "zoneCountdownTarget", p.zoneCountdownTarget);
  writeScheduleStringArray(out, "zoneCountdownFormat", p.zoneCountdownFormat);
  writeScheduleStringArray(out, "zoneCountdownFinish", p.zoneCountdownFinish);
  writeScheduleStringArray(out, "zoneCountdownPrefix", p.zoneCountdownPrefix);
  writeScheduleStringArray(out, "zoneCountdownSuffix", p.zoneCountdownSuffix);
  writeScheduleStringArray(out, "zoneStockSymbols", p.zoneStockSymbols);
  writeScheduleStringArray(out, "zoneStockDisplayFormat",
                           p.zoneStockDisplayFormat);
  writeScheduleStringArray(out, "zoneStockPrefix", p.zoneStockPrefix);
  writeScheduleStringArray(out, "zoneStockPostfix", p.zoneStockPostfix);
  out.print("\"name\":");
  writeScheduleJsonMaybeString(out, p.name);
  out.printf(",\"lastRun\":%lu}", (unsigned long)profileLastRun[index]);
}

void writeBase64JsonString(Print &out, const String &value) {
  static const char alphabet[] PROGMEM =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  out.print('"');
  uint32_t accumulator = 0;
  uint8_t bits = 0;
  size_t emitted = 0;
  for (size_t i = 0; i < value.length(); i++) {
    accumulator = (accumulator << 8) | static_cast<uint8_t>(value[i]);
    bits += 8;
    while (bits >= 6) {
      bits -= 6;
      out.write(pgm_read_byte(&alphabet[(accumulator >> bits) & 0x3F]));
      emitted++;
    }
  }
  if (bits) {
    out.write(pgm_read_byte(&alphabet[(accumulator << (6 - bits)) & 0x3F]));
    emitted++;
  }
  while (emitted % 4) {
    out.write('=');
    emitted++;
  }
  out.print('"');
}

void writeBackupJson(Print &out) {
  bool first = true;
  out.print('{');
  writeJsonNumberField(out, "backupVersion", 3, first);
  writeJsonStringField(out, "deviceName", shortMACaddr, first);
  writeJsonBoolField(out, "disableServiceMessages", disableServiceMessages,
                     first);
  writeJsonBoolField(out, "displayFlip", displayFlip, first);
  writeJsonNumberField(out, "zoneNumbers", zoneNumbers, first);
  writeJsonNumberField(out, "intensity", intensity, first);
  writeJsonBoolField(out, "mqttEnable", mqttEnable, first);
  writeJsonStringField(out, "mqttServerAddress", mqttServerAddress, first);
  writeJsonNumberField(out, "mqttServerPort", mqttServerPort, first);
  writeJsonStringField(out, "mqttUsername", mqttUsername, first);
  writeJsonKey(out, "mqttPassword", first);
  writeBase64JsonString(out, mqttPassword);
  writeJsonStringField(out, "mqttDevicePrefix", MQTTGlobalPrefix, first);
  writeJsonNumberField(out, "ntpTimeZone", ntpTimeZone, first);
  writeJsonNumberField(out, "ntpUpdateInterval", ntpUpdateInterval, first);
  writeJsonStringField(out, "ntpServer", ntpServer, first);
  writeJsonBoolField(out, "disableDotsBlink", disableDotsBlink, first);
  writeJsonKey(out, "owmApiToken", first);
  writeBase64JsonString(out, owmApiToken);
  writeJsonStringField(out, "owmUnitsFormat", owmUnitsFormat, first);
  writeJsonNumberField(out, "owmUpdateInterval", owmUpdateInterval, first);
  writeJsonStringField(out, "owmCity", owmCity, first);
  writeJsonStringField(out, "haAddr", haAddr, first);
  writeJsonStringField(out, "haApiHttpType", haApiHttpType, first);
  writeJsonKey(out, "haApiToken", first);
  writeBase64JsonString(out, haApiToken);
  writeJsonNumberField(out, "haApiPort", haApiPort, first);
  writeJsonNumberField(out, "haUpdateInterval", haUpdateInterval, first);
  writeJsonBoolField(out, "ds18b20Enable", ds18b20Enable, first);
  writeJsonNumberField(out, "ds18b20UpdateInterval", ds18b20UpdateInterval,
                       first);
  writeJsonStringField(out, "ds18b20UnitsFormat", ds18b20UnitsFormat, first);
  writeJsonKey(out, "stockApiToken", first);
  writeBase64JsonString(out, stockApiToken);
  writeJsonNumberField(out, "stockUpdateInterval", stockUpdateInterval, first);
  for (uint8_t i = 0; i < kZoneCapacity; i++) {
    writeZoneSettingsFields(out, i, first);
    char key[32];
    snprintf(key, sizeof(key), "woprUpdateIntervalZone%u", i);
    writeJsonNumberField(out, key, woprZones[i].updateInterval, first);
    writeProgressBarSettingsFields(out, i, first);
  }
  writeJsonBoolField(out, "schedulerEnabled", schedulerEnabled, first);
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
    char profileKey[24];
    snprintf(profileKey, sizeof(profileKey), "scheduleProfile%u", i);
    writeJsonKey(out, profileKey, first);
#if defined(ESP8266)
    schedulerLoadProfile(i, schedulerSharedProfileBuffer());
    const ScheduleProfile &p = schedulerSharedProfileBuffer();
#else
    const ScheduleProfile &p = scheduleProfiles[i];
#endif
    writeScheduleProfileJson(out, p, i);
    for (uint8_t z = 0; z < kZoneCapacity; z++) {
      char key[32];
      snprintf(key, sizeof(key), "scheduleBar%uZone%u", i, z);
      writeJsonKey(out, key, first);
      ScheduleProgressBar &pb = schedulerSharedProgressBarBuffer();
      if (schedulerLoadProgressBar(i, z, pb))
        writeScheduleProgressBarJson(out, pb, false);
      else
        out.print("null");
    }
  }
  out.print('}');
}

void __attribute__((unused)) addZoneSettingsToDoc(JsonDocument &doc,
                                                  uint8_t zone,
                                                  bool includeWoprUpdate,
                                                  bool includeProgressBar) {
  char key[48];

  snprintf(key, sizeof(key), "zone%uBegin", zone);
  doc[key] = zones[zone].begin;
  snprintf(key, sizeof(key), "zone%uEnd", zone);
  doc[key] = zones[zone].end;

  snprintf(key, sizeof(key), "workModeZone%u", zone);
  doc[key] = zones[zone].workMode;
  snprintf(key, sizeof(key), "scrollSpeedZone%u", zone);
  doc[key] = zones[zone].scrollSpeed;
  snprintf(key, sizeof(key), "scrollPauseZone%u", zone);
  doc[key] = zones[zone].scrollPause;
  snprintf(key, sizeof(key), "scrollAlignZone%u", zone);
  doc[key] = zones[zone].scrollAlign;
  snprintf(key, sizeof(key), "scrollEffectZone%uIn", zone);
  doc[key] = zones[zone].scrollEffectIn;
  snprintf(key, sizeof(key), "scrollEffectZone%uOut", zone);
  doc[key] = zones[zone].scrollEffectOut;
  snprintf(key, sizeof(key), "charspacingZone%u", zone);
  doc[key] = zones[zone].charspacing;
  snprintf(key, sizeof(key), "fontZone%u", zone);
  doc[key] = zones[zone].font;
  snprintf(key, sizeof(key), "mqttTextTopicZone%u", zone);
  doc[key] = MQTTZones[zone].message;
  snprintf(key, sizeof(key), "mqttPostfixZone%u", zone);
  doc[key] = zones[zone].mqttPostfix;
  snprintf(key, sizeof(key), "clockDisplayFormatZone%u", zone);
  doc[key] = zones[zone].clockDisplayFormat;
  snprintf(key, sizeof(key), "owmWhatToDisplayZone%u", zone);
  doc[key] = zones[zone].owmWhatToDisplay;
  snprintf(key, sizeof(key), "haSensorIdZone%u", zone);
  doc[key] = zones[zone].haSensorId;
  snprintf(key, sizeof(key), "haSensorPostfixZone%u", zone);
  doc[key] = zones[zone].haSensorPostfix;
  snprintf(key, sizeof(key), "ds18b20PostfixZone%u", zone);
  doc[key] = zones[zone].ds18b20Postfix;
  snprintf(key, sizeof(key), "scrollInfiniteZone%u", zone);
  doc[key] = zones[zone].scrollInfinite;

  if (includeWoprUpdate) {
    snprintf(key, sizeof(key), "woprUpdateIntervalZone%u", zone);
    doc[key] = woprZones[zone].updateInterval;
  }

  snprintf(key, sizeof(key), "countdownFormatZone%u", zone);
  doc[key] = zones[zone].countdownFormat;
  snprintf(key, sizeof(key), "countdownFinishZone%u", zone);
  doc[key] = zones[zone].countdownFinish;
  snprintf(key, sizeof(key), "countdownShowUnitsZone%u", zone);
  doc[key] = zones[zone].countdownShowUnits;
  snprintf(key, sizeof(key), "countdownPrefixZone%u", zone);
  doc[key] = zones[zone].countdownPrefix;
  snprintf(key, sizeof(key), "countdownSuffixZone%u", zone);
  doc[key] = zones[zone].countdownSuffix;
  snprintf(key, sizeof(key), "countdownTargetZone%u", zone);
  doc[key] = countdownState[zone].targetStr;

  snprintf(key, sizeof(key), "stockSymbolsZone%u", zone);
  doc[key] = zones[zone].stockSymbols;
  snprintf(key, sizeof(key), "stockDisplayFormatZone%u", zone);
  doc[key] = zones[zone].stockDisplayFormat;
  snprintf(key, sizeof(key), "stockPrefixZone%u", zone);
  doc[key] = zones[zone].stockPrefix;
  snprintf(key, sizeof(key), "stockPostfixZone%u", zone);
  doc[key] = zones[zone].stockPostfix;
  snprintf(key, sizeof(key), "stockShowArrowsZone%u", zone);
  doc[key] = zones[zone].stockShowArrows;

  if (!includeProgressBar)
    return;

  // Progress Bar
  snprintf(key, sizeof(key), "pbEnableZone%u", zone);
  doc[key] = progressBars[zone].enabled;
  snprintf(key, sizeof(key), "pbSrcTypeZone%u", zone);
  doc[key] = progressBars[zone].dataSourceType;
  snprintf(key, sizeof(key), "pbSrcIdZone%u", zone);
  doc[key] = progressBars[zone].dataSourceId;
  snprintf(key, sizeof(key), "pbMinZone%u", zone);
  doc[key] = progressBars[zone].minValue;
  snprintf(key, sizeof(key), "pbMaxZone%u", zone);
  doc[key] = progressBars[zone].maxValue;
  snprintf(key, sizeof(key), "pbCondEnZone%u", zone);
  doc[key] = progressBars[zone].conditionEnabled;
  snprintf(key, sizeof(key), "pbCondSrcTZone%u", zone);
  doc[key] = progressBars[zone].conditionSourceType;
  snprintf(key, sizeof(key), "pbCondSrcIZone%u", zone);
  doc[key] = progressBars[zone].conditionSourceId;
  snprintf(key, sizeof(key), "pbCondValZone%u", zone);
  doc[key] = progressBars[zone].conditionValue;
  snprintf(key, sizeof(key), "pbDynModeZone%u", zone);
  doc[key] = progressBars[zone].dynamicMode;
  snprintf(key, sizeof(key), "pbDynAnimZone%u", zone);
  doc[key] = progressBars[zone].dynamicAnimation;
  snprintf(key, sizeof(key), "pbDynSpdZone%u", zone);
  doc[key] = progressBars[zone].dynamicSpeedMs;
  snprintf(key, sizeof(key), "pbDynSrcTZone%u", zone);
  doc[key] = progressBars[zone].dynSourceType;
  snprintf(key, sizeof(key), "pbDynSrcIZone%u", zone);
  doc[key] = progressBars[zone].dynSourceId;
  snprintf(key, sizeof(key), "pbDynR1ValZone%u", zone);
  doc[key] = progressBars[zone].dynRule1Value;
  snprintf(key, sizeof(key), "pbDynR2ValZone%u", zone);
  doc[key] = progressBars[zone].dynRule2Value;
  snprintf(key, sizeof(key), "pbDynR2AnimZone%u", zone);
  doc[key] = progressBars[zone].dynRule2Animation;
  snprintf(key, sizeof(key), "pbDynR2SpdZone%u", zone);
  doc[key] = progressBars[zone].dynRule2SpeedMs;
}

void restoreZoneSettingsFromDoc(const JsonDocument &doc, uint8_t zone) {
  char key[48];

  snprintf(key, sizeof(key), "zone%uBegin", zone);
  assignIfPresent(doc, key, zones[zone].begin);
  snprintf(key, sizeof(key), "zone%uEnd", zone);
  assignIfPresent(doc, key, zones[zone].end);

  snprintf(key, sizeof(key), "workModeZone%u", zone);
  assignIfPresent(doc, key, zones[zone].workMode);
  snprintf(key, sizeof(key), "scrollSpeedZone%u", zone);
  assignIfPresent(doc, key, zones[zone].scrollSpeed);
  snprintf(key, sizeof(key), "scrollPauseZone%u", zone);
  assignIfPresent(doc, key, zones[zone].scrollPause);
  snprintf(key, sizeof(key), "scrollAlignZone%u", zone);
  assignIfPresent(doc, key, zones[zone].scrollAlign);
  snprintf(key, sizeof(key), "scrollEffectZone%uIn", zone);
  assignIfPresent(doc, key, zones[zone].scrollEffectIn);
  snprintf(key, sizeof(key), "scrollEffectZone%uOut", zone);
  assignIfPresent(doc, key, zones[zone].scrollEffectOut);
  snprintf(key, sizeof(key), "charspacingZone%u", zone);
  assignIfPresent(doc, key, zones[zone].charspacing);
  snprintf(key, sizeof(key), "fontZone%u", zone);
  assignIfPresent(doc, key, zones[zone].font);
  snprintf(key, sizeof(key), "mqttTextTopicZone%u", zone);
  assignIfPresent(doc, key, MQTTZones[zone].message);
  snprintf(key, sizeof(key), "mqttPostfixZone%u", zone);
  assignIfPresent(doc, key, zones[zone].mqttPostfix);
  snprintf(key, sizeof(key), "clockDisplayFormatZone%u", zone);
  assignIfPresent(doc, key, zones[zone].clockDisplayFormat);
  snprintf(key, sizeof(key), "owmWhatToDisplayZone%u", zone);
  assignIfPresent(doc, key, zones[zone].owmWhatToDisplay);
  snprintf(key, sizeof(key), "haSensorIdZone%u", zone);
  assignIfPresent(doc, key, zones[zone].haSensorId);
  snprintf(key, sizeof(key), "haSensorPostfixZone%u", zone);
  assignIfPresent(doc, key, zones[zone].haSensorPostfix);
  snprintf(key, sizeof(key), "ds18b20PostfixZone%u", zone);
  assignIfPresent(doc, key, zones[zone].ds18b20Postfix);
  snprintf(key, sizeof(key), "woprUpdateIntervalZone%u", zone);
  assignIfPresent(doc, key, woprZones[zone].updateInterval);
  snprintf(key, sizeof(key), "scrollInfiniteZone%u", zone);
  assignIfPresent(doc, key, zones[zone].scrollInfinite);

  snprintf(key, sizeof(key), "countdownFormatZone%u", zone);
  assignIfPresent(doc, key, zones[zone].countdownFormat);
  snprintf(key, sizeof(key), "countdownFinishZone%u", zone);
  assignIfPresent(doc, key, zones[zone].countdownFinish);
  snprintf(key, sizeof(key), "countdownShowUnitsZone%u", zone);
  assignIfPresent(doc, key, zones[zone].countdownShowUnits);
  snprintf(key, sizeof(key), "countdownPrefixZone%u", zone);
  assignIfPresent(doc, key, zones[zone].countdownPrefix);
  snprintf(key, sizeof(key), "countdownSuffixZone%u", zone);
  assignIfPresent(doc, key, zones[zone].countdownSuffix);
  snprintf(key, sizeof(key), "countdownTargetZone%u", zone);
  if (doc.containsKey(key)) {
    String target = doc[key].as<String>();
    if (target.length() > 0)
      parseCountdownTarget(target, zone, timeClient.getEpochTime(), ntpTimeZone);
    else
      countdownState[zone] = CountdownState{};
  }

  snprintf(key, sizeof(key), "stockSymbolsZone%u", zone);
  assignIfPresent(doc, key, zones[zone].stockSymbols);
  snprintf(key, sizeof(key), "stockDisplayFormatZone%u", zone);
  assignIfPresent(doc, key, zones[zone].stockDisplayFormat);
  snprintf(key, sizeof(key), "stockPrefixZone%u", zone);
  assignIfPresent(doc, key, zones[zone].stockPrefix);
  snprintf(key, sizeof(key), "stockPostfixZone%u", zone);
  assignIfPresent(doc, key, zones[zone].stockPostfix);
  snprintf(key, sizeof(key), "stockShowArrowsZone%u", zone);
  assignIfPresent(doc, key, zones[zone].stockShowArrows);

  // Progress Bar
  snprintf(key, sizeof(key), "pbEnableZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].enabled);
  snprintf(key, sizeof(key), "pbSrcTypeZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dataSourceType);
  snprintf(key, sizeof(key), "pbSrcIdZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dataSourceId);
  snprintf(key, sizeof(key), "pbMinZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].minValue);
  snprintf(key, sizeof(key), "pbMaxZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].maxValue);
  snprintf(key, sizeof(key), "pbCondEnZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].conditionEnabled);
  snprintf(key, sizeof(key), "pbCondSrcTZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].conditionSourceType);
  snprintf(key, sizeof(key), "pbCondSrcIZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].conditionSourceId);
  snprintf(key, sizeof(key), "pbCondValZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].conditionValue);
  snprintf(key, sizeof(key), "pbDynModeZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynamicMode);
  snprintf(key, sizeof(key), "pbDynAnimZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynamicAnimation);
  snprintf(key, sizeof(key), "pbDynSpdZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynamicSpeedMs);
  snprintf(key, sizeof(key), "pbDynSrcTZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynSourceType);
  snprintf(key, sizeof(key), "pbDynSrcIZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynSourceId);
  snprintf(key, sizeof(key), "pbDynR1ValZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynRule1Value);
  snprintf(key, sizeof(key), "pbDynR2ValZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynRule2Value);
  snprintf(key, sizeof(key), "pbDynR2AnimZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynRule2Animation);
  snprintf(key, sizeof(key), "pbDynR2SpdZone%u", zone);
  assignIfPresent(doc, key, progressBars[zone].dynRule2SpeedMs);
}

void applyRestoreScalarSettings(const JsonDocument &doc) {
  assignIfPresent(doc, "disableServiceMessages", disableServiceMessages);
  assignIfPresent(doc, "displayFlip", displayFlip);
  if (doc.containsKey("zoneNumbers")) {
    int parsedZoneCount = doc["zoneNumbers"];
    if (parsedZoneCount < 1)
      parsedZoneCount = 1;
    if (parsedZoneCount > kZoneCapacity)
      parsedZoneCount = kZoneCapacity;
    zoneNumbers = static_cast<uint8_t>(parsedZoneCount);
  }
  assignIfPresent(doc, "intensity", intensity);

  assignIfPresent(doc, "mqttEnable", mqttEnable);
  assignIfPresent(doc, "mqttServerAddress", mqttServerAddress);
  assignIfPresent(doc, "mqttServerPort", mqttServerPort);
  assignIfPresent(doc, "mqttUsername", mqttUsername);
  if (doc.containsKey("mqttPassword"))
    mqttPassword = base64_utils::decode(restoredLiveString<String>(doc["mqttPassword"]));

  assignIfPresent(doc, "ntpTimeZone", ntpTimeZone);
  assignIfPresent(doc, "ntpUpdateInterval", ntpUpdateInterval);
  assignIfPresent(doc, "ntpServer", ntpServer);
  assignIfPresent(doc, "disableDotsBlink", disableDotsBlink);

  if (doc.containsKey("owmApiToken"))
    owmApiToken = base64_utils::decode(restoredLiveString<String>(doc["owmApiToken"]));
  assignIfPresent(doc, "owmUnitsFormat", owmUnitsFormat);
  assignIfPresent(doc, "owmUpdateInterval", owmUpdateInterval);
  assignIfPresent(doc, "owmCity", owmCity);

  assignIfPresent(doc, "haAddr", haAddr);
  assignIfPresent(doc, "haApiHttpType", haApiHttpType);
  if (doc.containsKey("haApiToken"))
    haApiToken = base64_utils::decode(restoredLiveString<String>(doc["haApiToken"]));
  assignIfPresent(doc, "haApiPort", haApiPort);
  assignIfPresent(doc, "haUpdateInterval", haUpdateInterval);

  assignIfPresent(doc, "ds18b20Enable", ds18b20Enable);
  assignIfPresent(doc, "ds18b20UpdateInterval", ds18b20UpdateInterval);
  assignIfPresent(doc, "ds18b20UnitsFormat", ds18b20UnitsFormat);

  if (doc.containsKey("stockApiToken"))
    stockApiToken = base64_utils::decode(restoredLiveString<String>(doc["stockApiToken"]));
  assignIfPresent(doc, "stockUpdateInterval", stockUpdateInterval);
  assignIfPresent(doc, "schedulerEnabled", schedulerEnabled);
}

void persistRestoredConfiguration() {
  saveVarsToConfFile("systemSettings", 0);
  saveVarsToConfFile("displaySettings", 0);
  saveVarsToConfFile("intensity", 0);
  saveVarsToConfFile("mqttSettings", 0);
  saveVarsToConfFile("wallClockSett", 0);
  saveVarsToConfFile("owmSettings", 0);
  saveVarsToConfFile("haSettings", 0);
  saveVarsToConfFile("ds18b20Settings", 0);
  saveVarsToConfFile("stockSettings", 0);
  for (uint8_t i = 0; i < kZoneCapacity; i++) {
    saveVarsToConfFile("zoneSettings", i);
    saveVarsToConfFile("pbSettings", i);
  }
  saveSchedulerEnabledToConfFile();
}

#if defined(ESP8266)
bool deserializeRestoreSection(JsonDocument &destination,
                               const JsonDocument &filter) {
  File source = LittleFS.open(kRestoreTempPath, "r");
  if (!source)
    return false;
  destination.clear();
  DeserializationError error = deserializeJson(
      destination, source, DeserializationOption::Filter(filter));
  source.close();
  if (error || destination.overflowed()) {
    Serial.printf("\n[Restore] JSON section failed: %s overflow=%u",
                  error.c_str(), destination.overflowed() ? 1 : 0);
    return false;
  }
  return true;
}

void addRestoreFilterKeys(JsonDocument &filter, const char *const *keys,
                          size_t count) {
  for (size_t i = 0; i < count; i++)
    filter[keys[i]] = true;
}

bool restoreConfigurationFromTempFile() {
  // Keep restore's arena private from the background-job document.
  DynamicJsonDocument doc(2048);

  static const char *const scalarKeys[] = {
      "backupVersion",          "disableServiceMessages", "displayFlip", "zoneNumbers",
      "intensity",
      "mqttEnable",            "mqttServerAddress", "mqttServerPort",
      "mqttUsername",          "mqttPassword",      "ntpTimeZone",
      "ntpUpdateInterval",     "ntpServer",         "disableDotsBlink",
      "owmApiToken",           "owmUnitsFormat",    "owmUpdateInterval",
      "owmCity",               "haAddr",            "haApiHttpType",
      "haApiToken",            "haApiPort",         "haUpdateInterval",
      "ds18b20Enable",         "ds18b20UpdateInterval",
      "ds18b20UnitsFormat",    "stockApiToken",     "stockUpdateInterval",
      "schedulerEnabled"};
  uint8_t backupVersion = 1;
  {
    DynamicJsonDocument scalarFilter(1024);
    addRestoreFilterKeys(scalarFilter, scalarKeys,
                         sizeof(scalarKeys) / sizeof(scalarKeys[0]));
    if (!deserializeRestoreSection(doc, scalarFilter))
      return false;
    backupVersion = doc["backupVersion"] | 1;
    applyRestoreScalarSettings(doc);
  }

  static const char *const zoneKeyFormats[] = {
      "zone%uBegin",          "zone%uEnd",          "workModeZone%u",
      "scrollSpeedZone%u",   "scrollPauseZone%u", "scrollAlignZone%u",
      "scrollEffectZone%uIn", "scrollEffectZone%uOut",
      "charspacingZone%u",   "fontZone%u",         "mqttTextTopicZone%u",
      "mqttPostfixZone%u",   "clockDisplayFormatZone%u",
      "owmWhatToDisplayZone%u", "haSensorIdZone%u", "haSensorPostfixZone%u",
      "ds18b20PostfixZone%u", "woprUpdateIntervalZone%u",
      "scrollInfiniteZone%u", "countdownFormatZone%u",
      "countdownFinishZone%u", "countdownShowUnitsZone%u",
      "countdownPrefixZone%u", "countdownSuffixZone%u",
      "countdownTargetZone%u", "stockSymbolsZone%u",
      "stockDisplayFormatZone%u", "stockPrefixZone%u",
      "stockPostfixZone%u", "stockShowArrowsZone%u"};
  static const char *const progressKeyFormats[] = {
      "pbEnableZone%u",   "pbSrcTypeZone%u", "pbSrcIdZone%u",
      "pbMinZone%u",      "pbMaxZone%u",     "pbCondEnZone%u",
      "pbCondSrcTZone%u", "pbCondSrcIZone%u", "pbCondValZone%u",
      "pbDynModeZone%u",  "pbDynAnimZone%u", "pbDynSpdZone%u",
      "pbDynSrcTZone%u",  "pbDynSrcIZone%u", "pbDynR1ValZone%u",
      "pbDynR2ValZone%u", "pbDynR2AnimZone%u", "pbDynR2SpdZone%u"};

  for (uint8_t zone = 0; zone < kZoneCapacity; zone++) {
    char key[48];
    {
      DynamicJsonDocument zoneFilter(1536);
      for (size_t i = 0;
           i < sizeof(zoneKeyFormats) / sizeof(zoneKeyFormats[0]); i++) {
        snprintf(key, sizeof(key), zoneKeyFormats[i], zone);
        zoneFilter[key] = true;
      }
      if (!deserializeRestoreSection(doc, zoneFilter))
        return false;
      restoreZoneSettingsFromDoc(doc, zone);
    }

    {
      DynamicJsonDocument progressFilter(1024);
      for (size_t i = 0;
           i < sizeof(progressKeyFormats) / sizeof(progressKeyFormats[0]);
           i++) {
        snprintf(key, sizeof(key), progressKeyFormats[i], zone);
        progressFilter[key] = true;
      }
      if (!deserializeRestoreSection(doc, progressFilter))
        return false;
      restoreZoneSettingsFromDoc(doc, zone);
    }
    yield();
  }

  for (uint8_t i = 0; i < MAX_PROFILES; ++i) {
    schedulerAbandonStaging(i);
    for (uint8_t z = 0; z < kZoneCapacity; ++z) {
      char key[32];
      snprintf(key, sizeof(key), "scheduleBar%uZone%u", i, z);
      DynamicJsonDocument filter(96);
      filter[key] = true;
      if (!deserializeRestoreSection(doc, filter))
        return false;
      if (!doc.containsKey(key)) continue; // Older backups have no PB records.
      if (doc[key].isNull()) {
        if (!schedulerClearProgressBar(i, z)) return false;
      } else {
        ScheduleProgressBar &pb = schedulerSharedProgressBarBuffer();
        schedulerLoadProgressBar(i, z, pb);
        applyRestoreProgressBar(doc[key].as<JsonObjectConst>(), pb);
        if (!schedulerSaveProgressBar(i, z, pb)) return false;
      }
      yield();
    }
  }

  if (backupVersion >= 2) {
    for (uint8_t i = 0; i < MAX_PROFILES; i++) {
      char profileKey[24];
      snprintf(profileKey, sizeof(profileKey), "scheduleProfile%u", i);
      ScheduleProfile &profile = schedulerSharedProfileBuffer();
      schedulerLoadProfile(i, profile);
      // Read one field at a time: full profiles can exceed the 2KB arena.
      bool present = false;
      for (const char *key : profileRestoreKeys) {
        DynamicJsonDocument profileFilter(192);
        profileFilter[profileKey][key] = true;
        if (!deserializeRestoreSection(doc, profileFilter))
          return false;
        JsonObjectConst po = doc[profileKey].as<JsonObjectConst>();
        present = present || !po.isNull();
        applyRestoreProfile(po, profile);
        yield();
      }
      if (present && !schedulerSaveProfile(i, profile))
        return false;
      yield();
    }
  } else {
    // Compatibility path for backups created before version 2. New backups
    // keep profiles as separate top-level objects so this larger allocation is
    // never needed during normal operation.
    DynamicJsonDocument legacyProfiles(6144);
    DynamicJsonDocument legacyFilter(64);
    legacyFilter["scheduleProfiles"] = true;
    if (!deserializeRestoreSection(legacyProfiles, legacyFilter))
      return false;
    JsonArrayConst profiles =
        legacyProfiles["scheduleProfiles"].as<JsonArrayConst>();
    for (uint8_t i = 0; i < MAX_PROFILES && i < profiles.size(); i++) {
      if (profiles[i].isNull())
        continue;
      ScheduleProfile &profile = schedulerSharedProfileBuffer();
      schedulerLoadProfile(i, profile);
      applyRestoreProfile(profiles[i].as<JsonObjectConst>(), profile);
      if (!schedulerSaveProfile(i, profile))
        return false;
      yield();
    }
  }

  persistRestoredConfiguration();
  doc.clear();
  return true;
}
#endif

void clearPreferencesNamespace(const char *ns) {
#if defined(ESP32)
  if (strncmp(ns, "cronP", 5) == 0 || strncmp(ns, "cronB", 5) == 0)
    ProfileRecordStore::clearNamespace(ns);
#endif
  preferences.begin(ns, false);
  preferences.clear();
  preferences.end();
}

void resetStoredWifiSettings() {
  if (wifiManagerPtr) {
    wifiManagerPtr->resetSettings();
    return;
  }

#if defined(ESP8266)
  WiFi.disconnect(true);
#elif defined(ESP32)
  WiFi.disconnect(true, true);
#endif
}
} // namespace

bool webServicePendingRestore() {
#if defined(ESP8266)
  if (restoreState == 3 && static_cast<long>(millis() - restoreRebootAt) >= 0) {
    shouldReboot = true;
    return false;
  }
  if (restoreState != 1) return false;
  SchedulerWorkGuard work;
  restoreState = 2;
  bool restored = restoreConfigurationFromTempFile();
  LittleFS.remove(kRestoreTempPath);
  restoreState = restored ? 3 : 4;
  if (restored) restoreRebootAt = millis() + 3000UL;
  else initConfig = true;
  Serial.printf("\n[Restore] Deferred restore %s", restored ? "complete" : "failed");
  return true;
#else
  return false;
#endif
}

class OomPreFlightHandler : public AsyncWebHandler {
public:
  virtual bool canHandle(AsyncWebServerRequest *request) const override final {
    bool apiRequest = request->url().startsWith("/api/");
    if (apiRequest && schedulerWorkActive() &&
        !(request->method() == HTTP_GET && request->url() == "/api/restore-status")) {
      sendConcurrentApiJson(request);
      return true;
    }
    if (!beginTrackedWebRequest(apiRequest)) {
      sendConcurrentApiJson(request);
      return true;
    }
    request->onDisconnect(
        [apiRequest]() { endTrackedWebRequest(apiRequest); });
#if defined(ESP8266)
    if (isFetchingHttps)
      requestHttpsAbortForWeb();
    // Block POST requests when memory is critically low
    // (except OTA, reboot, resetWifi, and factory-reset which must always work)
    if (request->method() == HTTP_POST && request->url() != "/api/ota" &&
        request->url() != "/api/reboot" &&
        request->url() != "/api/factory-reset" &&
        request->url() != "/api/resetWifi" &&
        !request->url().startsWith("/api/scheduler/enabled") &&
        (ESP.getFreeHeap() < 4096 || ESP.getMaxFreeBlockSize() < 2048)) {
      sendBusyJson(request);
      return true;
    }
    // Chunked GET responses only need a small fixed TCP buffer.
    if (request->method() == HTTP_GET && request->url().startsWith("/api/")) {
      if (!canAllocateSmallResponse()) {
        sendBusyJson(request);
        return true;
      }
    }
#endif
    return false;
  }
  virtual void handleRequest(AsyncWebServerRequest *request) override final {}
};

void setupWebRoutes() {
  // ESP8266 cannot afford idle keep-alive sockets. Closing every response also
  // gives WebPriority an exact response-drained signal via onDisconnect().
  DefaultHeaders::Instance().addHeader("Connection", "close");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-store");
  // Catch OOM conditions on POST requests before body allocation
  server.addHandler(new OomPreFlightHandler());
  setupHilTestRoutes();
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (webHeapIsCritical())
      return;
    request->send(404, "text/plain", "Not found");
  });

  server.on("/api/diagnostics", HTTP_GET,
            [](AsyncWebServerRequest *request) {
              char body[400];
#if defined(ESP8266)
              snprintf(body, sizeof(body),
                       "{\"heap\":%u,\"maxBlock\":%u,\"fragmentation\":%u,"
                       "\"webRequests\":%u,\"apiRequests\":%u,"
                       "\"webPriority\":%s,\"httpsActive\":%s,"
                       "\"httpsAbortRequested\":%s,\"resetCode\":%u,"
                       "\"uptimeMs\":%lu,\"bootComplete\":%s,"
                       "\"displayFlip\":%s,\"displayFlipApplied\":%s,\"displayModules\":%u}",
                       ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                       ESP.getHeapFragmentation(),
                       webPriorityActiveRequestCount(),
                       webPriorityActiveApiCount(),
                       webPriorityIsActive(millis()) ? "true" : "false",
                       isFetchingHttps ? "true" : "false",
                       httpsAbortWasRequested() ? "true" : "false",
                       ESP.getResetInfoPtr()->reason, millis(),
                       allTestsFinish && !initConfig ? "true" : "false",
                       displayFlip ? "true" : "false",
                       displayRotation::flipped ? "true" : "false",
                       displayRotation::moduleCount);
#else
              const String sketchMd5 = ESP.getSketchMD5();
              Preferences storageStats;
              const unsigned nvsFreeEntries = storageStats.freeEntries();
              snprintf(body, sizeof(body),
                       "{\"heap\":%u,\"webPriority\":false,"
                       "\"httpsActive\":%s,\"uptimeMs\":%lu,\"bootComplete\":%s,"
                       "\"displayFlip\":%s,\"displayFlipApplied\":%s,\"displayModules\":%u,"
                       "\"sketchMD5\":\"%s\",\"nvsFreeEntries\":%u}",
                       ESP.getFreeHeap(), isFetchingHttps ? "true" : "false", millis(),
                       allTestsFinish && !initConfig ? "true" : "false",
                       displayFlip ? "true" : "false",
                       displayRotation::flipped ? "true" : "false",
                       displayRotation::moduleCount, sketchMd5.c_str(), nvsFreeEntries);
#endif
              AsyncWebServerResponse *response =
                  request->beginResponse(200, "application/json", body);
              addApiResponseHeaders(response);
              request->send(response);
            });

  // ─── Static Pages ──────────────────────────────────────────────────────────
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    sendProgmemPage(request, "text/html", indexPage, indexPage_len);
  });

  server.on("/zone-settings", HTTP_ANY, [](AsyncWebServerRequest *request) {
    sendProgmemPage(request, "text/html", zoneSettingsPage,
                    zoneSettingsPage_len);
  });

  server.on("/service-settings", HTTP_ANY, [](AsyncWebServerRequest *request) {
    sendProgmemPage(request, "text/html", serviceSettingsPage,
                    serviceSettingsPage_len);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendProgmemPage(request, "text/html", otaPage, otaPage_len);
  });

  server.on("/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendProgmemPage(request, "text/html", backupPage, backupPage_len);
  });

  server.on("/scheduler", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->redirect("/zone-settings");
  });

  // ─── Scheduler API ─────────────────────────────────────────────────────────
  // Metadata and profiles are separate so the ESP8266 never has to retain the
  // JSON for every profile at once.
  server.on("/api/scheduler/edit", HTTP_POST, [](AsyncWebServerRequest *request) {
    int idx = request->hasParam("idx") ? parseProfileIndex(request->getParam("idx")->value(), MAX_PROFILES) : -1;
    if (idx < 0 || !request->hasHeader("X-Profile-Edit")) {
      request->send(400, "application/json", "{\"error\":\"Invalid edit request\"}"); return;
    }
    bool alreadyOwned = schedulerEditActive();
    if (!schedulerBeginEdit(idx, request->getHeader("X-Profile-Edit")->value().c_str())) {
      request->send(409, "application/json", "{\"error\":\"Another profile edit is in progress\"}"); return;
    }
    if (!alreadyOwned) s_cronDraftActive = false;
    request->send(200, "application/json", "{\"status\":\"editing\"}");
  });
  server.on("/api/scheduler/edit-cancel", HTTP_POST, [](AsyncWebServerRequest *request) {
    int idx = request->hasParam("idx") ? parseProfileIndex(request->getParam("idx")->value(), MAX_PROFILES) : -1;
    if (idx < 0) { request->send(400, "application/json", "{\"error\":\"Invalid idx\"}"); return; }
    if (!ownsProfileEdit(request, idx)) return;
    if (schedulerHasPendingProfileSave() || schedulerHasPendingProgressBarSave()) {
      sendConcurrentApiJson(request); return;
    }
    schedulerEndEdit();
    request->send(200, "application/json", "{\"status\":\"closed\"}");
  });

  server.on("/api/scheduler/meta", HTTP_GET,
            [](AsyncWebServerRequest *request) {
              const bool enabled = schedulerEnabled, synced = ntpSyncedOnce;
              const bool pending = schedulerHasPendingProfileSave() || schedulerHasPendingProgressBarSave();
              const bool failed = schedulerStorageWriteFailed();
              const int active = lastActiveProfile;
              AsyncWebServerResponse *response = request->beginChunkedResponse(
                  "application/json",
                  [enabled, synced, pending, failed, active](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                    ChunkWindowPrint out(buffer, maxLen, index);
                    out.printf("{\"schedulerEnabled\":%s,\"ntpSynced\":%s,",
                               enabled ? "true" : "false",
                               synced ? "true" : "false");
                    out.printf("\"storagePending\":%s,\"storageError\":%s,",
                               pending ? "true" : "false",
                               failed ? "true" : "false");
                    out.print("\"ntpServer\":");
                    writeJsonEscaped(out, ntpServer);
                    out.printf(",\"lastActiveProfile\":%d,\"profileCount\":%u}",
                               active, (unsigned)MAX_PROFILES);
                    return out.copied();
                  });
              if (!response) {
                sendBusyJson(request);
                return;
              }
              addApiResponseHeaders(response);
              request->send(response);
            });

  server.on("/api/scheduler/profile", HTTP_GET,
            [](AsyncWebServerRequest *request) {
              if (!request->hasParam("idx")) {
                request->send(400, "application/json",
                              "{\"error\":\"Missing idx\"}");
                return;
              }
              int parsed = parseProfileIndex(request->getParam("idx")->value(), MAX_PROFILES);
              if (parsed < 0 || parsed >= MAX_PROFILES) {
                request->send(400, "application/json",
                              "{\"error\":\"Invalid idx\"}");
                return;
              }
              uint8_t idx = static_cast<uint8_t>(parsed);
#if defined(ESP8266)
              if (schedulerEditActive() || schedulerHasPendingProfileSave()) {
                sendConcurrentApiJson(request);
                return;
              }
              schedulerLoadProfile(idx, schedulerSharedProfileBuffer());
#endif
              AsyncWebServerResponse *response = request->beginChunkedResponse(
                  "application/json",
                  [idx](uint8_t *buffer, size_t maxLen,
                        size_t index) -> size_t {
                    ChunkWindowPrint out(buffer, maxLen, index);
#if defined(ESP8266)
                    writeScheduleProfileJson(out, schedulerSharedProfileBuffer(),
                                             idx);
#else
                    writeScheduleProfileJson(out, scheduleProfiles[idx], idx);
#endif
                    return out.copied();
                  });
              if (!response) {
                sendBusyJson(request);
                return;
              }
              addApiResponseHeaders(response);
              request->send(response);
            });

  // Full progress-bar settings live in small per-profile/per-zone records.
  // They are transferred separately so a profile request never needs another
  // large buffer on the ESP8266.
  server.on("/api/scheduler/progress-bar", HTTP_GET,
            [](AsyncWebServerRequest *request) {
    if (!request->hasParam("idx") || !request->hasParam("zone")) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing idx or zone\"}");
      return;
    }
    int profileIdx = parseProfileIndex(request->getParam("idx")->value(), MAX_PROFILES);
    int zone = parseProfileIndex(request->getParam("zone")->value(), kZoneCapacity);
    if (profileIdx < 0 || profileIdx >= MAX_PROFILES ||
        zone < 0 || zone >= 4) {
      request->send(400, "application/json",
                    "{\"error\":\"Invalid idx or zone\"}");
      return;
    }
    if (schedulerHasPendingProgressBarSave()) {
      sendConcurrentApiJson(request);
      return;
    }
    ScheduleProgressBar &pb = schedulerSharedProgressBarBuffer();
    bool stored = schedulerLoadProgressBar(profileIdx, zone, pb);
    AsyncWebServerResponse *response = request->beginChunkedResponse(
        "application/json",
        [stored](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
          ChunkWindowPrint out(buffer, maxLen, index);
          writeScheduleProgressBarJson(out,
                                       schedulerSharedProgressBarBuffer(),
                                       !stored);
          return out.copied();
        });
    if (!response) {
      sendBusyJson(request);
      return;
    }
    addApiResponseHeaders(response);
    request->send(response);
  });

  server.on("/api/scheduler/progress-bar", HTTP_POST,
            [](AsyncWebServerRequest *request) {
    if (!request->hasParam("idx", true) ||
        !request->hasParam("zone", true)) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing idx or zone\"}");
      return;
    }
    int profileIdx = parseProfileIndex(request->getParam("idx", true)->value(), MAX_PROFILES);
    int zone = parseProfileIndex(request->getParam("zone", true)->value(), kZoneCapacity);
    if (profileIdx < 0 || profileIdx >= MAX_PROFILES ||
        zone < 0 || zone >= 4) {
      request->send(400, "application/json",
                    "{\"error\":\"Invalid idx or zone\"}");
      return;
    }
    if (!ownsProfileEdit(request, profileIdx)) return;
    // Maintain strict request ordering: the previous profile/PB block must be
    // on flash before this shared buffer is reused.
    if (schedulerHasPendingProfileSave() ||
        schedulerHasPendingProgressBarSave()) {
      sendConcurrentApiJson(request);
      return;
    }

    ScheduleProgressBar &pb = schedulerSharedProgressBarBuffer();
    schedulerLoadProgressBar(profileIdx, zone, pb);
    pb.version = 1;

#define PB_POST_STR(name, field)                                                \
    if (request->hasParam(name, true))                                          \
      copyProgressBarParam(pb.field, sizeof(pb.field),                          \
                           request->getParam(name, true))
#define PB_POST_BOOL(name, field)                                               \
    if (request->hasParam(name, true))                                          \
      pb.field = parseBoolParam(request->getParam(name, true)->value())
#define PB_POST_INT(name, field)                                                \
    if (request->hasParam(name, true))                                          \
      pb.field = request->getParam(name, true)->value().toInt()

    PB_POST_STR("dataSourceType", dataSourceType);
    PB_POST_STR("dataSourceId", dataSourceId);
    if (request->hasParam("minValue", true))
      pb.minValue = request->getParam("minValue", true)->value().toFloat();
    if (request->hasParam("maxValue", true))
      pb.maxValue = request->getParam("maxValue", true)->value().toFloat();
    PB_POST_BOOL("conditionEnabled", conditionEnabled);
    PB_POST_STR("conditionSourceType", conditionSourceType);
    PB_POST_STR("conditionSourceId", conditionSourceId);
    PB_POST_STR("conditionValue", conditionValue);
    PB_POST_BOOL("dynamicMode", dynamicMode);
    PB_POST_INT("dynamicAnimation", dynamicAnimation);
    PB_POST_INT("dynamicSpeedMs", dynamicSpeedMs);
    PB_POST_STR("dynSourceType", dynSourceType);
    PB_POST_STR("dynSourceId", dynSourceId);
    PB_POST_STR("dynRule1Value", dynRule1Value);
    PB_POST_STR("dynRule2Value", dynRule2Value);
    PB_POST_INT("dynRule2Animation", dynRule2Animation);
    PB_POST_INT("dynRule2SpeedMs", dynRule2SpeedMs);

#undef PB_POST_STR
#undef PB_POST_BOOL
#undef PB_POST_INT

    if (pb.dynamicAnimation < 1 || pb.dynamicAnimation > 16)
      pb.dynamicAnimation = 1;
    if (pb.dynRule2Animation < 1 || pb.dynRule2Animation > 16)
      pb.dynRule2Animation = 1;
    if (pb.dynamicSpeedMs < 10)
      pb.dynamicSpeedMs = 10;
    if (pb.dynRule2SpeedMs < 10)
      pb.dynRule2SpeedMs = 10;

    if (!schedulerStageProgressBarSave(profileIdx, zone)) {
      sendConcurrentApiJson(request);
      return;
    }
    request->send(202, "application/json", "{\"status\":\"queued\"}");
  });

  // ─── Scheduler global enable (small no-body endpoint for ESP8266) ───────────
  server.on("/api/scheduler/enabled", HTTP_POST,
            [](AsyncWebServerRequest *request) {
    const AsyncWebParameter *enabledParam = nullptr;
    if (request->hasParam("enabled")) {
      enabledParam = request->getParam("enabled");
    } else if (request->hasParam("schedulerEnabled")) {
      enabledParam = request->getParam("schedulerEnabled");
    }

    if (!enabledParam) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing enabled parameter\"}");
      return;
    }

    schedulerEnabled = parseBoolParam(enabledParam->value());
    request->send(200, "application/json", "{\"status\":\"ok\"}");
    queueConfigSave("cronGlobal", 0);
  });

  // ─── Apply a saved profile immediately ────────────────────────────────────
  // Application is staged for main() so NVS reads and display reconfiguration
  // never run inside the AsyncWebServer callback.
  server.on("/api/scheduler/apply", HTTP_POST,
            [](AsyncWebServerRequest *request) {
    const AsyncWebParameter *idxParam = nullptr;
    if (request->hasParam("idx"))
      idxParam = request->getParam("idx");
    else if (request->hasParam("idx", true))
      idxParam = request->getParam("idx", true);

    if (!idxParam) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing idx parameter\"}");
      return;
    }
    int parsed = parseProfileIndex(idxParam->value(), MAX_PROFILES);
    if (parsed < 0 || parsed >= MAX_PROFILES) {
      request->send(400, "application/json",
                    "{\"error\":\"Invalid profile index\"}");
      return;
    }
    if (!schedulerStageProfileApply(static_cast<uint8_t>(parsed))) {
      sendConcurrentApiJson(request);
      return;
    }
    request->send(202, "application/json",
                  "{\"status\":\"queued\"}");
  });

  // ─── NTP Force Sync ────────────────────────────────────────────────────────
  server.on("/api/ntp-sync", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (ntpServer.length() == 0) {
      request->send(400, "application/json",
                    "{\"success\":false,\"error\":\"NTP server not configured\"}");
      return;
    }
    bool ok = schedulerForceNtpSync();
    if (ok) {
      String t = timeClient.getFormattedTime();
      request->send(200, "application/json",
                    "{\"success\":true,\"time\":\"" + t + "\"}");
    } else {
      request->send(200, "application/json",
                    "{\"success\":false,\"error\":\"NTP sync failed. Check server address and network.\"}");
    }
  });

  // ─── Backup API ────────────────────────────────────────────────────────────
  server.on("/api/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
    if (schedulerEditActive() || schedulerHasPendingProfileSave() || schedulerHasPendingProgressBarSave()) {
      sendConcurrentApiJson(request);
      return;
    }
#endif
    AsyncWebServerResponse *response = request->beginChunkedResponse(
        "application/json",
        [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
          ChunkWindowPrint out(buffer, maxLen, index);
          writeBackupJson(out);
          return out.copied();
        });
    if (!response) {
      sendBusyJson(request);
      return;
    }
    char filename[80];
    snprintf(filename, sizeof(filename),
             "attachment; filename=wledPixel-%s_backup.json",
             shortMACaddr.c_str());
    response->addHeader("Content-Disposition", filename);
    addApiResponseHeaders(response);
    request->send(response);
  });

  server.on("/api/restore-status", HTTP_GET, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
    char body[32];
    snprintf(body, sizeof(body), "{\"state\":%u}", restoreState);
    request->send(200, "application/json", body);
#else
    request->send(200, "application/json", "{\"state\":0}");
#endif
  });

  // ─── Restore API ───────────────────────────────────────────────────────────
  server.on(
      "/api/restore", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        if (schedulerEditActive() || schedulerHasPendingProfileSave() || schedulerHasPendingProgressBarSave()) {
          sendConcurrentApiJson(request); return;
        }
#if defined(ESP8266)
        if (restoreState == 1 || restoreState == 2) { sendConcurrentApiJson(request); return; }
        if (restoreUploadFile)
          restoreUploadFile.close();
        if (restoreUploadFailed || restoreUploadBytes == 0 ||
            !LittleFS.exists(kRestoreTempPath)) {
          LittleFS.remove(kRestoreTempPath);
          request->send(400, "application/json",
                        "{\"error\":\"Backup upload failed or is empty\"}");
          return;
        }

        // Parsing and yield-capable flash writes belong to the main stack,
        // never to the ESP8266 async TCP callback stack.
        restoreState = 1;
        request->send(202, "application/json", "{\"status\":\"queued\"}");
        return;
#else
        if (restoreJsonBuffer.length() == 0) {
          request->send(400, "application/json",
                        "{\"error\":\"Empty backup\"}");
          return;
        }
        // Size the arena for this payload instead of reserving 96 KB even for
        // a small backup; the upload String also occupies contiguous heap.
        const size_t jsonCapacity = restoreJsonBuffer.length() * 2U + 8192U;
        DynamicJsonDocument doc(jsonCapacity < 98304U ? jsonCapacity : 98304U);
        DeserializationError error = deserializeJson(doc, restoreJsonBuffer);
        restoreJsonBuffer = "";

        if (error || doc.overflowed()) {
          request->send(400, "application/json",
                        "{\"error\":\"Invalid JSON\"}");
          return;
        }

        applyRestoreScalarSettings(doc);
        for (uint8_t i = 0; i < kZoneCapacity; i++) {
          restoreZoneSettingsFromDoc(doc, i);
        }
        bool restoredBarsChanged[MAX_PROFILES] = {};
        for (uint8_t i = 0; i < MAX_PROFILES; ++i) {
          schedulerAbandonStaging(i);
          for (uint8_t z = 0; z < kZoneCapacity; ++z) {
            char key[32];
            snprintf(key, sizeof(key), "scheduleBar%uZone%u", i, z);
            if (!doc.containsKey(key)) continue;
            if (doc[key].isNull()) {
              // An already inherited bar needs no record or namespace.
              if (!schedulerLoadProgressBar(i, z, schedulerSharedProgressBarBuffer())) continue;
              if (!schedulerClearProgressBar(i, z)) {
                request->send(500, "text/plain", "Profile storage write failed");
                return;
              }
              restoredBarsChanged[i] = true;
            } else {
              ScheduleProgressBar &pb = schedulerSharedProgressBarBuffer();
              const bool stored = schedulerLoadProgressBar(i, z, pb);
              const ScheduleProgressBar previous = pb;
              applyRestoreProgressBar(doc[key].as<JsonObjectConst>(), pb);
              if (stored && memcmp(&previous, &pb, sizeof(pb)) == 0) continue;
              if (!schedulerSaveProgressBar(i, z, pb)) {
                request->send(500, "text/plain", "Profile storage write failed");
                return;
              }
              restoredBarsChanged[i] = true;
            }
          }
        }
        if (doc.containsKey("scheduleProfiles")) {
          JsonArrayConst profs = doc["scheduleProfiles"].as<JsonArrayConst>();
          for (uint8_t i = 0; i < MAX_PROFILES && i < profs.size(); i++) {
            JsonObjectConst po = profs[i].as<JsonObjectConst>();
            ScheduleProfile sp;
            schedulerLoadProfile(i, sp);
            applyRestoreProfile(po, sp);
            if (!restoredBarsChanged[i] && memcmp(&sp, &scheduleProfiles[i], sizeof(sp)) == 0) continue;
            if (!schedulerSaveProfile(i, sp)) {
              request->send(500, "text/plain", "Profile storage write failed");
              return;
            }
          }
        } else {
          for (uint8_t i = 0; i < MAX_PROFILES; i++) {
            char profileKey[24];
            snprintf(profileKey, sizeof(profileKey), "scheduleProfile%u", i);
            JsonObjectConst po = doc[profileKey].as<JsonObjectConst>();
            if (po.isNull())
              continue;
            ScheduleProfile sp;
            schedulerLoadProfile(i, sp);
            applyRestoreProfile(po, sp);
            if (!restoredBarsChanged[i] && memcmp(&sp, &scheduleProfiles[i], sizeof(sp)) == 0) continue;
            if (!schedulerSaveProfile(i, sp)) {
              request->send(500, "text/plain", "Profile storage write failed");
              return;
            }
          }
        }
        persistRestoredConfiguration();
#endif

        request->send(200, "text/plain", "OK");
        shouldReboot = true;
      },
      [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final) {
#if defined(ESP8266)
        if (restoreState == 1 || restoreState == 2) return;
        if (index == 0) {
          restoreUploadFailed = false;
          restoreUploadBytes = 0;
          if (!LittleFS.begin()) {
            restoreUploadFailed = true;
            return;
          }
          if (restoreUploadFile)
            restoreUploadFile.close();
          LittleFS.remove(kRestoreTempPath);
          restoreUploadFile = LittleFS.open(kRestoreTempPath, "w");
          if (!restoreUploadFile)
            restoreUploadFailed = true;
        }
        if (!restoreUploadFailed && restoreUploadBytes + len <= 98304) {
          size_t written = restoreUploadFile.write(data, len);
          restoreUploadBytes += written;
          if (written != len)
            restoreUploadFailed = true;
        } else if (len > 0) {
          restoreUploadFailed = true;
        }
        if (final || restoreUploadFailed) {
          if (restoreUploadFile)
            restoreUploadFile.close();
          if (restoreUploadFailed)
            LittleFS.remove(kRestoreTempPath);
        }
#else
        if (index == 0) {
          restoreJsonBuffer = "";
          size_t contentLen = request->contentLength();
          if (contentLen > 0 && contentLen < 98304)
            restoreJsonBuffer.reserve(contentLen + 1);
        }
        if (restoreJsonBuffer.length() + len <= 98304)
          restoreJsonBuffer.concat(reinterpret_cast<const char *>(data), len);
#endif
      });

  // ─── Reboot / Reset WiFi ──────────────────────────────────────────────────
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    shouldReboot = true;
  });

  server.on("/api/resetWifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    resetStoredWifiSettings();
    shouldReboot = true;
  });

  // ─── Zone Settings JSON API
  // ──────────────────────────────────────────────────
  server.on("/api/zone-settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginChunkedResponse(
        "application/json",
        [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
          ChunkWindowPrint out(buffer, maxLen, index);
          bool first = true;
          out.print('{');
          writeJsonStringField(out, "deviceName", shortMACaddr, first);
#if defined(ESP32)
          writeJsonCStringField(out, "platform", "ESP32", first);
#elif defined(ESP8266)
          writeJsonCStringField(out, "platform", "ESP8266", first);
#else
          writeJsonCStringField(out, "platform", "Unknown", first);
#endif
          writeJsonCStringField(out, "firmwareVer", firmwareVer, first);
          writeJsonNumberField(out, "zoneNumbers", zoneNumbers, first);
          writeJsonNumberField(out, "intensity", intensity + 1, first);
          writeJsonStringField(out, "wifiSsid", WiFi.SSID(), first);
          writeJsonStringField(out, "wifiIp", WiFi.localIP().toString(), first);
          writeJsonStringField(out, "wifiGateway",
                               WiFi.gatewayIP().toString(), first);
          writeJsonNumberField(out, "wifiRssi", WiFi.RSSI(), first);
          writeJsonStringField(out, "mqttDevicePrefix", MQTTGlobalPrefix,
                               first);
          writeJsonStringField(
              out, "ds18b20Temp",
              dsTemp == "-127" ? String("Not connected") : dsTemp, first);
          for (uint8_t i = 0; i < kZoneCapacity; i++)
            writeZoneSettingsFields(out, i, first);
          writeJsonBoolField(out, "stockShowArrows",
                             zones[0].stockShowArrows, first);
          out.print('}');
          return out.copied();
        });
    if (!response) {
      sendBusyJson(request);
      return;
    }
    addApiResponseHeaders(response);
    request->send(response);
  });

  // ─── Service Settings JSON API ─────────────────────────────────────────────
  server.on(
      "/api/service-settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginChunkedResponse(
            "application/json",
            [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
              ChunkWindowPrint out(buffer, maxLen, index);
              writeServiceSettingsJson(out);
              return out.copied();
            });
        if (!response) {
          sendBusyJson(request);
          return;
        }
        addApiResponseHeaders(response);
        request->send(response);
      });

  // ─── PB Debug Endpoint ────────────────────────────────────────────────────
  server.on("/api/pb-debug", HTTP_GET,
      [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginChunkedResponse(
            "application/json",
            [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
              ChunkWindowPrint out(buffer, maxLen, index);
              out.printf("{\"zoneNumbers\":%u,\"mqttConnected\":%s",
                         zoneNumbers, mqttClient.connected() ? "true" : "false");
              for (uint8_t i = 0; i < zoneNumbers && i < 4; i++) {
                bool first = true;
                out.printf(",\"zone%u\":{", i);
                writeJsonBoolField(out, "enabled", progressBars[i].enabled,
                                   first);
                writeJsonBoolField(out, "dynamicMode",
                                   progressBars[i].dynamicMode, first);
                writeJsonStringField(out, "dataSourceType",
                                     progressBars[i].dataSourceType, first);
                writeJsonStringField(out, "dataSourceId",
                                     progressBars[i].dataSourceId, first);
                writeJsonNumberField(out, "currentValue",
                                     progressBars[i].currentValue, first);
                writeJsonBoolField(out, "conditionEnabled",
                                   progressBars[i].conditionEnabled, first);
                writeJsonBoolField(out, "conditionMet",
                                   progressBars[i].conditionMet, first);
                writeJsonBoolField(out, "barActive", progressBars[i].barActive,
                                   first);
                writeJsonStringField(out, "dynSourceType",
                                     progressBars[i].dynSourceType, first);
                writeJsonStringField(out, "dynSourceId",
                                     progressBars[i].dynSourceId, first);
                writeJsonStringField(out, "dynRule1Value",
                                     progressBars[i].dynRule1Value, first);
                writeJsonNumberField(out, "dynRule1Anim",
                                     progressBars[i].dynamicAnimation, first);
                writeJsonNumberField(out, "dynRule1Speed",
                                     progressBars[i].dynamicSpeedMs, first);
                writeJsonStringField(out, "dynRule2Value",
                                     progressBars[i].dynRule2Value, first);
                writeJsonNumberField(out, "dynRule2Anim",
                                     progressBars[i].dynRule2Animation, first);
                writeJsonNumberField(out, "dynRule2Speed",
                                     progressBars[i].dynRule2SpeedMs, first);
                writeJsonStringField(out, "dynCurrentSensorValue",
                                     progressBars[i].dynCurrentSensorValue,
                                     first);
                writeJsonNumberField(out, "dynActiveAnim",
                                     progressBars[i].dynActiveAnim, first);
                writeJsonNumberField(out, "dynPhase", progressBars[i].dynPhase,
                                     first);
                writeJsonNumberField(out, "dynPosition",
                                     progressBars[i].dynPosition, first);
                out.print('}');
              }
              out.print('}');
              return out.copied();
            });
        if (!response) {
          sendBusyJson(request);
          return;
        }
        addApiResponseHeaders(response);
        request->send(response);
      });

  // ─── OTA Firmware Upload ──────────────────────────────────────────────────
  server.on(
      "/api/ota", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // Check chunk info
        int chunkNum = 0;
        int totalChunks = 1;

        if (request->hasParam("chunk")) {
          chunkNum = request->getParam("chunk")->value().toInt();
        }
        if (request->hasParam("total")) {
          totalChunks = request->getParam("total")->value().toInt();
        }

        // Platform mismatch detected in upload handler — reject immediately
        if (otaPlatformMismatch) {
          otaPlatformMismatch = false;
#if defined(ESP8266)
          const char *myPlatform = "ESP8266";
#elif defined(ESP32)
          const char *myPlatform = "ESP32";
#else
          const char *myPlatform = "Unknown";
#endif
          String errMsg = String("{\"status\":\"error\",\"msg\":\"Platform "
                                 "mismatch! Device is ") +
                          myPlatform +
                          ", firmware is for different platform.\"}";
          request->send(400, "application/json", errMsg);
          return;
        }

        // Check if this is the last chunk
        bool isLastChunk = (chunkNum >= totalChunks - 1);

        if (isLastChunk) {
          if (globalOtaInProgress) {
            // Finalize the update
            bool success = false;
            if (Update.end(true)) {
              Serial.printf("OTA Update Success: %u bytes\n", otaTotalSize);
              success = true;
            } else {
              Update.printError(Serial);
            }

            globalOtaInProgress = false;
            otaTotalSize = 0;

            AsyncWebServerResponse *response = request->beginResponse(
                200, "application/json",
                success ? "{\"status\":\"ok\",\"restart\":true}"
                        : "{\"status\":\"error\"}");
            response->addHeader("Connection", "close");
            request->send(response);

            if (success) {
              Serial.println("OTA Success, setting reboot flag...");
              shouldReboot = true;
            }
          } else {
            // Last chunk but update process failed somewhere
            request->send(500, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Update failed\"}");
          }
        } else {
          // Not the last chunk, just acknowledge
          request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
      },
      [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final) {
        // Get chunk info from query parameters
        int chunkNum = 0;
        size_t totalSize = 0;

        if (request->hasParam("chunk"))
          chunkNum = request->getParam("chunk")->value().toInt();
        if (request->hasParam("size"))
          totalSize = request->getParam("size")->value().toInt();

      // ── Platform detection: wrong-platform marker ─────────────────────────
      // The marker strings are XOR-encoded (key=0x5A) so they do NOT appear
      // as literal text in THIS binary, preventing false-positive self-matches.
      // "WLEDPIXEL_PLATFORM_ESP32"  is encoded for ESP8266 scanner.
      // "WLEDPIXEL_PLATFORM_ESP8266" is encoded for ESP32 scanner.
      // Decoded at runtime into a stack buffer before scanning.
#if defined(ESP8266)
        // XOR-encoded "WLEDPIXEL_PLATFORM_ESP32" (key=0x5A), 24 bytes
        static const uint8_t wrongMarkerEnc[] = {
            0x0D, 0x16, 0x1F, 0x1E, 0x0A, 0x13, 0x02, 0x1F,
            0x16, 0x05, 0x0A, 0x16, 0x1B, 0x0E, 0x1C, 0x15,
            0x08, 0x17, 0x05, 0x1F, 0x09, 0x0A, 0x69, 0x68};
        const char *deviceName = "ESP8266";
        const char *wrongPlatform = "ESP32";
#elif defined(ESP32)
        // XOR-encoded "WLEDPIXEL_PLATFORM_ESP8266" (key=0x5A), 26 bytes
        static const uint8_t wrongMarkerEnc[] = {
            0x0D, 0x16, 0x1F, 0x1E, 0x0A, 0x13, 0x02, 0x1F, 0x16,
            0x05, 0x0A, 0x16, 0x1B, 0x0E, 0x1C, 0x15, 0x08, 0x17,
            0x05, 0x1F, 0x09, 0x0A, 0x62, 0x68, 0x6C, 0x6C};
        const char *deviceName = "ESP32";
        const char *wrongPlatform = "ESP8266";
#endif
        // Decode wrongMarker into stack buffer at runtime
        const size_t wrongMarkerLen = sizeof(wrongMarkerEnc);
        char wrongMarkerBuf[28]; // max marker len + 1, fixed size
        for (size_t i = 0; i < wrongMarkerLen; i++)
          wrongMarkerBuf[i] = wrongMarkerEnc[i] ^ 0x5A;
        wrongMarkerBuf[wrongMarkerLen] = '\0';
        const char *wrongMarker = wrongMarkerBuf;
        const size_t overlapSize = wrongMarkerLen - 1; // max 30 bytes

        if (chunkNum == 0 && index == 0) {
          // Reset ALL OTA state for a fresh upload session.
          // Abort any previous incomplete update so Update.begin() works.
          if (globalOtaInProgress) {
#ifdef ESP32
            Update.abort();
#else
            Update.end(false); // ESP8266: end(false) discards without applying
#endif
            globalOtaInProgress = false;
          }
          otaPlatformOk = true;
          otaPlatformMismatch = false;
          otaScanOverlapLen = 0;
        }

        // Only scan if no mismatch found yet
        if (otaPlatformOk && len > 0) {
          // Build a scan window: [overlap_from_prev_chunk] + [current_data]
          // This catches markers that span two chunks.
          // Use a small stack buffer (overlapSize + len capped) for the join.
          // Since overlapSize <= 30 and data pointer is directly usable,
          // we do two passes:

          // Pass 1: scan overlap+start of current data
          if (otaScanOverlapLen > 0) {
            // Build a joined buffer of overlapSize + min(len, overlapSize)
            // bytes
            uint8_t joinBuf[62]; // 2 * overlapSize max
            size_t joinLen = otaScanOverlapLen + min(len, overlapSize);
            memcpy(joinBuf, otaScanOverlap, otaScanOverlapLen);
            memcpy(joinBuf + otaScanOverlapLen, data, min(len, overlapSize));
            for (size_t i = 0; i + wrongMarkerLen <= joinLen; i++) {
              if (memcmp(joinBuf + i, wrongMarker, wrongMarkerLen) == 0) {
                otaPlatformOk = false;
                break;
              }
            }
          }

          // Pass 2: scan the current chunk itself
          if (otaPlatformOk && len >= wrongMarkerLen) {
            for (size_t i = 0; i <= len - wrongMarkerLen; i++) {
              if (memcmp(data + i, wrongMarker, wrongMarkerLen) == 0) {
                otaPlatformOk = false;
                break;
              }
            }
          }

          // Save tail of current chunk as overlap for next chunk
          size_t newOverlapLen = min(len, overlapSize);
          memcpy(otaScanOverlap, data + len - newOverlapLen, newOverlapLen);
          otaScanOverlapLen = newOverlapLen;

          if (!otaPlatformOk) {
            Serial.printf(
                "OTA REJECTED: binary contains '%s' marker, device is %s!\n",
                wrongPlatform, deviceName);
            // Abort the Updater so next attempt can call begin() cleanly
            if (globalOtaInProgress) {
#ifdef ESP32
              Update.abort();
#else
              Update.end(
                  false); // ESP8266: end(false) discards without applying
#endif
            }
            otaPlatformMismatch = true;
            globalOtaInProgress = false;
            return;
          }
        }

        // If mismatch was already detected in a previous index call — stop
        if (otaPlatformMismatch)
          return;
        // ─────────────────────────────────────────────────────────────────

        if (chunkNum == 0 && index == 0) {
          globalOtaInProgress = true;
          otaTotalSize = 0;

#ifdef ESP8266
          Update.runAsync(true);
#endif
          size_t updateSize = totalSize > 0 ? totalSize : UPDATE_SIZE_UNKNOWN;
          if (!Update.begin(updateSize, U_FLASH)) {
            globalOtaInProgress = false;
            return;
          }
        }

        // Write data to flash
        if (len > 0 && globalOtaInProgress) {
          if (Update.write(data, len) != len) {
            globalOtaInProgress = false;
            return;
          }
          otaTotalSize += len;
        }
      });

  // ─── Temperature API ──────────────────────────────────────────────────────
  server.on("/api/temperature", HTTP_ANY, [](AsyncWebServerRequest *request) {
    if (ds18b20Enable) {
      if (dsTemp == "-127") {
        request->send(200, "text/plain", "Not connected");
      } else {
        request->send(200, "text/plain", dsTemp);
      }
    } else {
      request->send(404, "text/plain", "Sensor disabled");
    }
  });

  // ─── Message API ──────────────────────────────────────────────────────────
  server.on("/api/message", HTTP_ANY, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
    if (ESP.getFreeHeap() < 4096) {
      request->send(503, "application/json",
                    "{\"error\":\"Low memory, try again\"}");
      return;
    }
#endif
    Serial.print(F("API request received "));
    int params = request->params();
    Serial.print(params);
    Serial.println(F(" params sent in"));

    for (int i = 0; i < params; i++) {
      const AsyncWebParameter *p = request->getParam((size_t)i);
      if (p && p->isPost()) {
        Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        int8_t zone = detectZoneFromParamName(p->name());
        if (zone >= 0 && p->name().startsWith("message")) {
          zoneNewMessage(zone, p->value().c_str(), "", true, true);
        }
      }
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ─── Config API ───────────────────────────────────────────────────────────
  server.on("/api/config", HTTP_ANY, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
    // Form parameters already occupy heap by the time this callback runs.
    // Reject only at the genuinely unsafe floor; ordinary low-memory requests
    // must still be allowed to update RAM and defer persistence.
    if (ESP.getFreeHeap() < 3072 || ESP.getMaxFreeBlockSize() < 1024) {
      request->send(503, "application/json",
                    "{\"error\":\"Low memory, try again\"}");
      return;
    }
#endif
    Serial.print(F("\nAPI request received "));
    int params = request->params();
    Serial.print(params);
    Serial.println(F(" params sent in"));

    if (params == 0) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing config group\"}");
      return;
    }

    if (!request->hasParam("key", true)) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing config group\"}");
      return;
    }
    const AsyncWebParameter *key = request->getParam("key", true);

    String keyValue = key->value();
    uint8_t n = kZoneCapacity;
    bool finishRequest = false;
    bool showModeMessage = false;
    bool displayLayoutChanged = false;
    bool zoneUpdate[kZoneCapacity] = {false};

    if (keyValue == "zoneSettings" && !resolveZoneIndexFromRequest(request, n)) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing or invalid zone\"}");
      return;
    }

    // Resolve and load the scheduler profile before applying any other form
    // fields. Multipart/form parameter order is not a reliable API contract.
    if (keyValue == "cronProfile") {
      // Do not let a following chunk overwrite the shared profile buffer while
      // the previous chunk is still waiting to be persisted by main().
      if (schedulerHasPendingProfileSave()) {
        sendConcurrentApiJson(request);
        return;
      }
      if (!request->hasParam("idx", true)) {
        request->send(400, "application/json",
                      "{\"error\":\"Missing profile idx\"}");
        return;
      }
      int parsedProfile = parseProfileIndex(request->getParam("idx", true)->value(), MAX_PROFILES);
      if (parsedProfile < 0 || parsedProfile >= MAX_PROFILES) {
        request->send(400, "application/json",
                      "{\"error\":\"Invalid profile idx\"}");
        return;
      }
      s_cronProfIdx = static_cast<uint8_t>(parsedProfile);
      if (!ownsProfileEdit(request, s_cronProfIdx)) return;
      bool resetProfile =
          request->hasParam("reset", true) &&
          parseBoolParam(request->getParam("reset", true)->value());
      bool draftProfile =
          request->hasParam("draft", true) &&
          parseBoolParam(request->getParam("draft", true)->value());
      bool commitProfile =
          request->hasParam("commit", true) &&
          parseBoolParam(request->getParam("commit", true)->value());
      if (request->hasParam("beginDraft", true) &&
          parseBoolParam(request->getParam("beginDraft", true)->value()))
        s_cronDraftActive = false;
      ScheduleProfile &profile = schedulerSharedProfileBuffer();
      if (!resetProfile && (draftProfile || commitProfile) &&
          s_cronDraftActive &&
          (s_cronDraftIdx != s_cronProfIdx ||
           s_cronDraftGeneration != schedulerProfileBufferGeneration())) {
        request->send(409, "application/json",
                      "{\"error\":\"Profile draft expired; reload and retry\"}");
        return;
      }
      if (resetProfile) {
        schedulerInitProfileForEdit(s_cronProfIdx, profile);
        s_cronDraftActive = draftProfile;
        s_cronDraftIdx = s_cronProfIdx;
      } else if (!((draftProfile || commitProfile) &&
                   s_cronDraftActive &&
                   s_cronDraftIdx == s_cronProfIdx)) {
        schedulerLoadProfile(s_cronProfIdx, profile);
        // A large delta is split across several draft requests. The first
        // draft starts from the stored profile; following drafts reuse this
        // same RAM image until the explicit commit request.
        if (draftProfile) {
          s_cronDraftActive = true;
          s_cronDraftIdx = s_cronProfIdx;
        } else {
          s_cronDraftActive = false;
        }
      }
    }

    if (keyValue == "cronProfile")
      s_cronDraftGeneration = schedulerProfileBufferGeneration();

    for (int i = 0; i < params; i++) {
      const AsyncWebParameter *p = request->getParam((size_t)i);
      if (!p || !p->isPost() || p->name() == "key")
        continue;

      if (keyValue == "cronProfile") {
        ScheduleProfile &profile = schedulerSharedProfileBuffer();
        if (applyCronProfileParam(profile, *p))
          finishRequest = true;
        continue;
      }

        if (keyValue == "systemSettings") {
          if (p->name() == "disableServiceMessages")
            disableServiceMessages = parseBoolParam(p->value());
          if (p->name() == "displayFlip")
            displayFlip = parseBoolParam(p->value());
          if (p->name() == "deviceName")
            shortMACaddr = p->value().c_str();

          finishRequest = true;
        }

        if (keyValue == "displaySettings") {
          if (p->name() == "zoneNumbers") {
            int parsedZoneCount = p->value().toInt();
            if (parsedZoneCount < 1)
              parsedZoneCount = 1;
            if (parsedZoneCount > kZoneCapacity)
              parsedZoneCount = kZoneCapacity;
            if (zoneNumbers != static_cast<uint8_t>(parsedZoneCount))
              displayLayoutChanged = true;
            zoneNumbers = static_cast<uint8_t>(parsedZoneCount);
          }
          for (uint8_t zone = 0; zone < kZoneCapacity; zone++) {
            char field[16];
            snprintf(field, sizeof(field), "zone%uBegin", zone);
            if (p->name() == field) {
              uint8_t value = p->value().toInt();
              if (zones[zone].begin != value)
                displayLayoutChanged = true;
              zones[zone].begin = value;
            }
            snprintf(field, sizeof(field), "zone%uEnd", zone);
            if (p->name() == field) {
              uint8_t value = p->value().toInt();
              if (zones[zone].end != value)
                displayLayoutChanged = true;
              zones[zone].end = value;
            }
          }
          if (p->name() == "intensity")
            P.setIntensity(p->value().toInt());

          finishRequest = true;
        }

        if (keyValue == "zoneSettings") {
          if (n < kZoneCapacity) {
            if (p->name() == "workMode") {
              String oldWorkMode = zones[n].workMode;
              zones[n].workMode = p->value().c_str();
              if (oldWorkMode != zones[n].workMode) {
                if (zones[n].workMode == "owmWeather")
                  owmLastTime = -1000000;
                if (zones[n].workMode == "haClient")
                  haLastTime = -1000000;
                if (zones[n].workMode == "stockTicker")
                  stockLastTime = 0;
                if (zones[n].workMode == "wopr") {
                  woprZones[n].active = true;
                  strcpy(zoneMessages[n], ""); // Clear active text
                  P.displayClear(n);
                  P.displayReset(n);
                  P.setTextEffect(n, PA_NO_EFFECT, PA_NO_EFFECT);
                } else {
                  woprZones[n].active = false;
                }

                if (!disableServiceMessages)
                  showModeMessage = true;
                if (zones[n].workMode == "wallClock") {
                  timeClient.setTimeOffset(ntpTimeZone * 3600);
                  shouldUpdateNtp = true;
                  zoneUpdate[n] = true;
                }
                if (zones[n].workMode != "wallClock" &&
                    zones[n].workMode != "owmWeather" &&
                    zones[n].workMode != "haClient" &&
                    zones[n].workMode != "intTempSensor" &&
                    zones[n].workMode != "wopr" &&
                    zones[n].workMode != "stockTicker") {
                  P.displayReset(n);
                }
                zones[n].updateFont = true;
              }
            }
            if (p->name() == "scrollSpeed") {
              zones[n].scrollSpeed = p->value().toInt();
              P.setSpeed(n, zones[n].scrollSpeed);
            }
            if (p->name() == "scrollPause") {
              zones[n].scrollPause = p->value().toInt();
              P.setPause(n, zones[n].scrollPause * 1000);
            }
            if (p->name() == "scrollAlign") {
              zones[n].scrollAlign = p->value().c_str();
              P.setTextAlignment(n,
                                 stringToTextPositionT(zones[n].scrollAlign));
            }
            if (p->name() == "scrollEffectIn") {
              zones[n].scrollEffectIn = p->value().c_str();
              updateSpriteData(n);
            }
            if (p->name() == "scrollEffectOut") {
              zones[n].scrollEffectOut = p->value().c_str();
              updateSpriteData(n);
            }

            if (zones[n].workMode == "wallClock" ||
                zones[n].workMode == "owmWeather" ||
                zones[n].workMode == "wopr" ||
                zones[n].workMode == "countdown") {
              if (zones[n].workMode == "wallClock") {
                strcpy(zoneMessages[n], ""); // Clear active text
                P.displayClear(n);
                P.displayReset(n);
              }
              if (zones[n].workMode == "countdown") {
                strcpy(zoneMessages[n], "");
                P.displayClear(n);
                P.displayReset(n);
                zones[n].previousMillis = -1000000;
                zones[n].curTime = "";
                countdownState[n].reached = false;
                countdownState[n].blinkCount = 0;
              }
              zones[n].newMessageAvailable = false;
              zoneUpdate[n] = true;
            } else {
              // For MQTT/Manual: just store new effects, don't reset display
              if (zones[n].workMode != "mqttClient" &&
                  zones[n].workMode != "manualInput") {
                textEffect_t effOut =
                    stringToTextEffectT(zones[n].scrollEffectOut);
                P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                                effOut);
                P.displayReset(n);
              }
              zones[n].restoreEffects = true;
            }
          }
          if (p->name() == "mqttTextTopic") {
            if (MQTTZones[n].message != p->value().c_str() && mqttEnable) {
              MQTTZones[n].message = p->value().c_str();
              shouldMqttPublish = true;
            }
          }
          if (p->name() == "mqttPostfix") {
            if (zones[n].mqttPostfix != p->value().c_str() && mqttEnable) {
              zones[n].mqttPostfix = p->value().c_str();
              shouldMqttPublish = true;
            }
          }
          if (p->name() == "clockDisplayFormat") {
            zones[n].clockDisplayFormat = p->value().c_str();
            zoneUpdate[n] = true;
            zones[n].curTime = "";
          }
          if (p->name() == "owmWhatToDisplay") {
            if (zones[n].workMode == "owmWeather") {
              zones[n].owmWhatToDisplay = p->value().c_str();
              if (zones[n].owmWhatToDisplay == "owmWeatherIcon") {
                zones[n].updateFont = true;
                P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                                PA_NO_EFFECT);
              }
            }
          }
          if (p->name() == "font") {
            if (zones[n].font != p->value().c_str()) {
              zones[n].font = p->value().c_str();
              zones[n].updateFont = true;
            }
          }
          if (p->name() == "charspacing") {
            zones[n].charspacing = p->value().toInt();
            P.setCharSpacing(n, zones[n].charspacing);
          }
          if (p->name() == "ds18b20Postfix") {
            zones[n].ds18b20Postfix = p->value().c_str();
            previousDsMillis = -1000;
          }
          if (p->name() == "haSensorId")
            zones[n].haSensorId = p->value().c_str();
          if (p->name() == "haSensorPostfix")
            zones[n].haSensorPostfix = p->value().c_str();
          if (p->name() == "scrollInfinite") {
            zones[n].scrollInfinite = parseBoolParam(p->value());
          }
          // Countdown settings
          if (p->name() == "countdownTarget") {
            const String &val = p->value();
            if (val.length() > 0) {
              parseCountdownTarget(val, n, timeClient.getEpochTime(),
                                   ntpTimeZone);
              zones[n].curTime = "";
              zones[n].forceUpdate = true;
            }
          }
          if (p->name() == "countdownFormat") {
            zones[n].countdownFormat = p->value().c_str();
            zones[n].curTime = "";
          }
          if (p->name() == "countdownFinish")
            zones[n].countdownFinish = p->value().c_str();
          if (p->name() == "countdownShowUnits") {
            zones[n].countdownShowUnits = parseBoolParam(p->value());
            zones[n].curTime = "";
          }
          if (p->name() == "countdownPrefix") {
            zones[n].countdownPrefix = p->value().c_str();
            zones[n].curTime = "";
          }
          if (p->name() == "countdownSuffix") {
            zones[n].countdownSuffix = p->value().c_str();
            zones[n].curTime = "";
          }
          // Stock ticker settings
          if (p->name() == "stockSymbols") {
            zones[n].stockSymbols = p->value().c_str();
            stockParseSymbols(n, zones[n].stockSymbols);
            stockLastTime = 0; // Force immediate update
          }
          if (p->name() == "stockDisplayFormat") {
            zones[n].stockDisplayFormat = p->value().c_str();
            zones[n].curTime = ""; // Force redraw
          }
          if (p->name() == "stockPrefix") {
            zones[n].stockPrefix = p->value().c_str();
            zones[n].curTime = ""; // Force redraw
          }
          if (p->name() == "stockPostfix") {
            zones[n].stockPostfix = p->value().c_str();
            zones[n].curTime = ""; // Force redraw
          }
          if (p->name() == "stockShowArrows") {
            zones[n].stockShowArrows = parseBoolParam(p->value());
          }

          finishRequest = true;
        }
      if (keyValue == "mqttSettings") {
        if (p->name() == "mqttEnable")
          mqttEnable = parseBoolParam(p->value());
        if (p->name() == "mqttServerAddress")
          mqttServerAddress = p->value().c_str();
        if (p->name() == "mqttServerPort")
          mqttServerPort = p->value().toInt();
        if (p->name() == "mqttUsername")
          mqttUsername = p->value().c_str();
        if (p->name() == "mqttPassword") {
          String val = p->value();
          Serial.print("API Config: Received mqttPassword: ");
          Serial.println(val);
          if (val != "********") {
            mqttPassword = val.c_str();
          } else {
            Serial.println("API Config: Ignoring masked password");
          }
        }

        finishRequest = true;
        shouldMqttDisconnect = true;
      }

      if (keyValue == "wallClockSett") {
        if (p->name() == "ntpTimeZone")
          ntpTimeZone = p->value().toFloat();
        if (p->name() == "disableDotsBlink")
          disableDotsBlink = parseBoolParam(p->value());
        if (p->name() == "ntpUpdateInterval")
          ntpUpdateInterval = p->value().toInt();
        if (p->name() == "ntpServer") {
          ntpServer = p->value().c_str();
          timeClient.setPoolServerName(ntpServer.c_str());
        }

        // Apply timezone change to timeClient and schedule NTP update
        timeClient.setTimeOffset(ntpTimeZone * 3600);
        timeClient.setUpdateInterval(ntpUpdateInterval * 3600);
        shouldUpdateNtp = true;

        finishRequest = true;
      }

      if (keyValue == "owmSettings") {
        if (p->name() == "owmApiToken") {
          String val = p->value();
          if (val != "********") {
            owmApiToken = val.c_str();
          }
        }
        if (p->name() == "owmUnitsFormat")
          owmUnitsFormat = p->value().c_str();
        if (p->name() == "owmUpdateInterval")
          owmUpdateInterval = p->value().toInt();
        if (p->name() == "owmCity")
          owmCity = p->value().c_str();

        finishRequest = true;
      }
      if (keyValue == "haSettings") {
        if (p->name() == "haAddr")
          haAddr = p->value().c_str();
        if (p->name() == "haUpdateInterval")
          haUpdateInterval = p->value().toInt();
        if (p->name() == "haApiToken") {
          String val = p->value();
          if (val != "********") {
            haApiToken = val.c_str();
          }
        }
        if (p->name() == "haApiHttpType")
          haApiHttpType = p->value().c_str();
        if (p->name() == "haApiPort")
          haApiPort = p->value().toInt();

        finishRequest = true;
      }

      if (keyValue == "ds18b20Settings") {
        if (p->name() == "ds18b20Enable")
          ds18b20Enable = parseBoolParam(p->value());
        if (p->name() == "ds18b20UpdateInterval")
          ds18b20UpdateInterval = p->value().toInt();
        if (p->name() == "ds18b20UnitsFormat") {
          ds18b20UnitsFormat = p->value().c_str();
          previousDsMillis = -1000;
        }

        finishRequest = true;
      }

      if (keyValue == "stockSettings") {
        if (p->name() == "stockApiToken") {
          String val = p->value();
          if (val != "********") {
            stockApiToken = val.c_str();
          }
        }
        if (p->name() == "stockUpdateInterval")
          stockUpdateInterval = p->value().toInt();
        finishRequest = true;
      }

      if (keyValue == "pbSettings") {
        // Parse per-zone progress bar parameters
        // Expected param names: pbEnable0, pbSrcType0, pbSrcId0, pbMin0,
        // pbMax0, pbCondEn0, pbCondSrcT0, pbCondSrcI0, pbCondVal0, etc.
        for (uint8_t z = 0; z < kZoneCapacity; z++) {
          char pname[24];
          snprintf(pname, sizeof(pname), "pbEnable%u", z);
          if (p->name() == pname)
            progressBars[z].enabled = parseBoolParam(p->value());
          snprintf(pname, sizeof(pname), "pbSrcType%u", z);
          if (p->name() == pname)
            progressBars[z].dataSourceType = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbSrcId%u", z);
          if (p->name() == pname)
            progressBars[z].dataSourceId = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbMin%u", z);
          if (p->name() == pname)
            progressBars[z].minValue = p->value().toFloat();
          snprintf(pname, sizeof(pname), "pbMax%u", z);
          if (p->name() == pname)
            progressBars[z].maxValue = p->value().toFloat();
          snprintf(pname, sizeof(pname), "pbCondEn%u", z);
          if (p->name() == pname)
            progressBars[z].conditionEnabled = parseBoolParam(p->value());
          snprintf(pname, sizeof(pname), "pbCondSrcT%u", z);
          if (p->name() == pname)
            progressBars[z].conditionSourceType = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbCondSrcI%u", z);
          if (p->name() == pname)
            progressBars[z].conditionSourceId = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbCondVal%u", z);
          if (p->name() == pname)
            progressBars[z].conditionValue = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbDynMode%u", z);
          if (p->name() == pname)
            progressBars[z].dynamicMode = parseBoolParam(p->value());
          snprintf(pname, sizeof(pname), "pbDynAnim%u", z);
          if (p->name() == pname)
            progressBars[z].dynamicAnimation = p->value().toInt();
          snprintf(pname, sizeof(pname), "pbDynSpd%u", z);
          if (p->name() == pname)
            progressBars[z].dynamicSpeedMs = p->value().toInt();
          snprintf(pname, sizeof(pname), "pbDynSrcT%u", z);
          if (p->name() == pname)
            progressBars[z].dynSourceType = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbDynSrcI%u", z);
          if (p->name() == pname)
            progressBars[z].dynSourceId = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbDynR1Val%u", z);
          if (p->name() == pname)
            progressBars[z].dynRule1Value = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbDynR2Val%u", z);
          if (p->name() == pname)
            progressBars[z].dynRule2Value = p->value().c_str();
          snprintf(pname, sizeof(pname), "pbDynR2Anim%u", z);
          if (p->name() == pname)
            progressBars[z].dynRule2Animation = p->value().toInt();
          snprintf(pname, sizeof(pname), "pbDynR2Spd%u", z);
          if (p->name() == pname)
            progressBars[z].dynRule2SpeedMs = p->value().toInt();
        }
        finishRequest = true;
      }

      if (keyValue == "intensity") {
        if (p->name() == "intensity") {
          if ((p->value()).toInt() > 0) {
            intensity = (p->value()).toInt() - 1;
            if (!power) {
              power = true;
              P.displayShutdown(0);
            }
            P.setIntensity(intensity);
          }
          if ((p->value()).toInt() == 0) {
            P.displayShutdown(1);
            power = false;
          }
        }
        finishRequest = true;
      }

      // ── Scheduler global enable ──
      if (keyValue == "cronGlobal") {
        if (p->name() == "schedulerEnabled")
          schedulerEnabled = parseBoolParam(p->value());
        finishRequest = true;
      }

    }

    if (!finishRequest) {
      request->send(400, "application/json",
                    "{\"error\":\"No valid config fields\"}");
      return;
    }

    // Apply batched updates
    for (uint8_t z = 0; z < kZoneCapacity; z++) {
      if (zoneUpdate[z])
        zones[z].forceUpdate = true;
    }

    if (finishRequest) {
      if (showModeMessage && !disableServiceMessages && n < kZoneCapacity) {
        if (zones[n].workMode == "mqttClient")
          zoneShowModeMessage(n, "MQTT");
        if (zones[n].workMode == "manualInput")
          zoneShowModeMessage(n, "Manual");
        if (zones[n].workMode == "haClient") {
          zoneShowModeMessage(n, "HA");
          zones[n].previousMillis = -1000000;
        }
        if (zones[n].workMode == "owmWeather") {
          zoneShowModeMessage(n, "OWM");
          zones[n].previousMillis = -1000000;
        }
        if (zones[n].workMode == "intTempSensor")
          zoneShowModeMessage(n, "TempS");
        if (zones[n].workMode == "stockTicker") {
          zoneShowModeMessage(n, "Stock");
          stockLastTime = 0;
        }
      }
      if (keyValue == "zoneSettings" && n < kZoneCapacity &&
          zones[n].workMode == "mqttClient") {
        if (mqttClient.connected() && MQTTZones[n].message != "" &&
            MQTTZones[n].message != " ") {
          mqttClient.subscribe(MQTTZones[n].message.c_str());
        }
      }
      if (keyValue == "cronProfile") {
        bool draftOnly =
            request->hasParam("draft", true) &&
            parseBoolParam(request->getParam("draft", true)->value()) &&
            !(request->hasParam("commit", true) &&
              parseBoolParam(request->getParam("commit", true)->value()));
        if (!draftOnly) {
          if (!schedulerProfileValid(schedulerSharedProfileBuffer(), MAX_DEVICES)) {
            request->send(400, "application/json", "{\"error\":\"Invalid profile values or zone layout\"}");
            return;
          }
          if (!schedulerStageProfileSave(s_cronProfIdx)) {
            sendConcurrentApiJson(request);
            return;
          }
          if (s_cronDraftActive && s_cronDraftIdx == s_cronProfIdx)
            s_cronDraftActive = false;
        }
      }
      request->send(keyValue == "cronProfile" && schedulerHasPendingProfileSave() ? 202 : 200,
                    "application/json", "{\"status\":\"ok\"}");
      if (keyValue == "zoneSettings") {
        queueConfigSave("zoneSettings", n);
      } else if (keyValue == "cronGlobal") {
        queueConfigSave("cronGlobal", 0);
      } else if (keyValue == "cronProfile") {
        // Saved individually by main(), only after the explicit commit.
      } else {
        queueConfigSave(keyValue.c_str(), 0);
      }

      // Immediate progress bar update after settings change
      if (keyValue == "pbSettings") {
        MD_MAX72XX *mx = P.getGraphicObject();
        for (uint8_t z = 0; z < zoneNumbers && z < 4; z++) {
          // Re-evaluate condition state (Regular mode only)
          if (!progressBars[z].dynamicMode) {
            if (progressBars[z].conditionEnabled) {
              progressBars[z].conditionMet = false;
            } else {
              progressBars[z].conditionMet = true;
            }
          }
          // Reset update timers
          progressBars[z].lastDataUpdate = 0;
          progressBars[z].lastCondUpdate = 0;
          // Reset dynamic animation state
          progressBars[z].dynCurrentSensorValue = "";
          progressBars[z].lastDynSrcUpdate = 0;
          progressBars[z].dynPhase = 0;
          progressBars[z].dynPosition = 0;
          progressBars[z].dynLastStep = 0;
          progressBars[z].dynActiveAnim = 0;
          // Clear row 7 for this zone if bar is disabled or condition not met
          if ((!progressBars[z].enabled ||
               (progressBars[z].conditionEnabled && !progressBars[z].conditionMet)) && mx) {
            int startCol = zones[z].begin * 8;
            int endCol = (zones[z].end * 8) + 7;
            for (int col = startCol; col <= endCol; col++) {
              mx->setPoint(7, col, false);
            }
          }
        }

        // Resubscribe to MQTT PB topics (device is already connected)
        if (mqttClient.connected()) {
          for (uint8_t z = 0; z < zoneNumbers && z < 4; z++) {
            if (!progressBars[z].enabled) continue;
            if (!progressBars[z].dynamicMode) {
              // Regular mode: data + condition sources
              if (progressBars[z].dataSourceType == "mqtt" &&
                  progressBars[z].dataSourceId.length() > 0) {
                mqttClient.subscribe(progressBars[z].dataSourceId.c_str());
              }
              if (progressBars[z].conditionEnabled &&
                  progressBars[z].conditionSourceType == "mqtt" &&
                  progressBars[z].conditionSourceId.length() > 0) {
                mqttClient.subscribe(progressBars[z].conditionSourceId.c_str());
              }
            } else {
              // Dynamic mode: rules source
              if (progressBars[z].dynSourceType == "mqtt" &&
                  progressBars[z].dynSourceId.length() > 0) {
                mqttClient.subscribe(progressBars[z].dynSourceId.c_str());
                Serial.printf("\n[PB] Subscribe dyn src: %s",
                              progressBars[z].dynSourceId.c_str());
              }
            }
          }
        }

        // Immediately redraw active bars
        updateProgressBars();
      }

      shouldMqttPublish = true;
      // Reinforce power state after zone settings to prevent display
      // turning off
      if (keyValue == "zoneSettings" && power) {
        P.displayShutdown(0);
      }
      if (keyValue == "displaySettings" && displayLayoutChanged) {
        Serial.println(F("[WEB] Display layout changed; restart scheduled"));
        restartESP = true;
      }
    }
  });

  // ─── Factory Reset ────────────────────────────────────────────────────────
  server.on("/api/factory-reset", HTTP_POST,
            [](AsyncWebServerRequest *request) {
              if (schedulerEditActive() || hasDeferredConfigSaves() || schedulerHasPendingProfileSave() ||
                  schedulerHasPendingProgressBarSave() || schedulerHasPendingProfileApply()) {
                sendConcurrentApiJson(request);
                return;
              }
              Serial.println(F("Factory reset requested!"));

              // Clear all preferences namespaces
              clearPreferencesNamespace("systemSettings");
              clearPreferencesNamespace("displaySettings");
              clearPreferencesNamespace("mqttSettings");
              clearPreferencesNamespace("wallClockSett");
              clearPreferencesNamespace("owmSettings");
              clearPreferencesNamespace("haSettings");
              clearPreferencesNamespace("ds18b20Settings");
              clearPreferencesNamespace("intensity");
              clearPreferencesNamespace("zoneSettings");
              clearPreferencesNamespace("pbSettings");
              clearPreferencesNamespace("cronSettings");
              clearPreferencesNamespace("stockSettings");
              for (uint8_t i = 0; i < MAX_PROFILES; i++) {
                char ns[12];
                snprintf(ns, sizeof(ns), "cronP%u", i);
                clearPreferencesNamespace(ns);
                for (uint8_t z = 0; z < kZoneCapacity; z++) {
                  snprintf(ns, sizeof(ns), "cronB%u%u", i, z);
                  clearPreferencesNamespace(ns);
                }
              }

              resetStoredWifiSettings();

              request->send(200, "application/json",
                            "{\"status\":\"ok\",\"message\":\"Factory reset "
                            "complete. Restarting...\"}");

              // Schedule restart
              restartESP = true;
            });
}
