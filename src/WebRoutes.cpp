#include "WebRoutes.h"
#include "ConfigStore.h"
#include "DisplayUtils.h"
#include "MqttHandler.h"
#include <cstdio>
#include <httpsRequestJson.h>

namespace {
constexpr uint8_t kZoneCapacity = 4;

void addStaticPageHeaders(AsyncWebServerResponse *response) {
  response->addHeader("Content-Encoding", "gzip");
  response->addHeader("Cache-Control", "max-age=3600");
  response->addHeader("Connection", "close");
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
    target = doc[key].as<String>();
}

void addZoneSettingsToDoc(JsonDocument &doc, uint8_t zone,
                          bool includeWoprUpdate) {
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
}

void clearPreferencesNamespace(const char *ns) {
  preferences.begin(ns, false);
  preferences.clear();
  preferences.end();
}
} // namespace

class OomPreFlightHandler : public AsyncWebHandler {
public:
  virtual bool canHandle(AsyncWebServerRequest *request) const override final {
#if defined(ESP8266)
    // Block POST requests when memory is critically low or TLS is active
    // (except OTA, reboot, resetWifi, and factory-reset which must always work)
    if (request->method() == HTTP_POST && request->url() != "/api/ota" &&
        request->url() != "/api/reboot" &&
        request->url() != "/api/factory-reset" &&
        request->url() != "/api/resetWifi" &&
        (isFetchingHttps || ESP.getFreeHeap() < 15360 ||
         ESP.getMaxFreeBlockSize() < 8192)) {
      request->send(503, "application/json",
                    "{\"error\":\"Device busy, try again\"}");
      request->client()->close();
      return true;
    }
    // Block heavy GET /api/ requests ONLY during active TLS fetch
    // (memory thresholds are too tight for normal ESP8266 operation)
    if (request->method() == HTTP_GET && request->url().startsWith("/api/") &&
        isFetchingHttps) {
      request->send(503, "application/json",
                    "{\"error\":\"Device busy, try again\"}");
      request->client()->close();
      return true;
    }
#endif
    return false;
  }
  virtual void handleRequest(AsyncWebServerRequest *request) override final {}
};

void setupWebRoutes() {
  // Catch OOM conditions on POST requests before body allocation
  server.addHandler(new OomPreFlightHandler());

  // ─── Static Pages ──────────────────────────────────────────────────────────
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response =
        request->beginResponse_P(200, "text/html", indexPage, indexPage_len);
    addStaticPageHeaders(response);
    request->send(response);
  });

  server.on("/zone-settings", HTTP_ANY, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "text/html", zoneSettingsPage, zoneSettingsPage_len);
    addStaticPageHeaders(response);
    request->send(response);
  });

  server.on("/service-settings", HTTP_ANY, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "text/html", serviceSettingsPage, serviceSettingsPage_len);
    addStaticPageHeaders(response);
    request->send(response);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
    if (ESP.getFreeHeap() < 12288 || isFetchingHttps) {
      request->send(503, "text/plain",
                    "Device busy, please try again in a few seconds.");
      return;
    }
#endif
    AsyncWebServerResponse *response =
        request->beginResponse_P(200, "text/html", otaPage, otaPage_len);
    addStaticPageHeaders(response);
    request->send(response);
  });

  server.on("/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response =
        request->beginResponse_P(200, "text/html", backupPage, backupPage_len);
    addStaticPageHeaders(response);
    request->send(response);
  });

  // ─── Backup API ────────────────────────────────────────────────────────────
  server.on("/api/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
    if (ESP.getFreeHeap() < 12288 || isFetchingHttps) { // Require at least 10KB
      request->send(503, "application/json",
                    "{\"error\":\"Device busy, try again\"}");
      return;
    }
#endif
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    String filename = "attachment; filename=wledPixel-";
    filename += shortMACaddr;
    filename += "_backup.json";
    response->addHeader("Content-Disposition", filename);
    DynamicJsonDocument doc(6144);

    // System
    doc["deviceName"] = shortMACaddr;
    doc["disableServiceMessages"] = disableServiceMessages;
    doc["zoneNumbers"] = zoneNumbers;
    doc["intensity"] = intensity;

    // MQTT
    doc["mqttEnable"] = mqttEnable;
    doc["mqttServerAddress"] = mqttServerAddress;
    doc["mqttServerPort"] = mqttServerPort;
    doc["mqttUsername"] = mqttUsername;
    doc["mqttPassword"] = base64_utils::encode(mqttPassword); // Encode
    doc["mqttDevicePrefix"] = MQTTGlobalPrefix;

    // NTP
    doc["ntpTimeZone"] = ntpTimeZone;
    doc["ntpUpdateInterval"] = ntpUpdateInterval;
    doc["ntpServer"] = ntpServer;
    doc["disableDotsBlink"] = disableDotsBlink;

    // OWM
    doc["owmApiToken"] = base64_utils::encode(owmApiToken); // Encode
    doc["owmUnitsFormat"] = owmUnitsFormat;
    doc["owmUpdateInterval"] = owmUpdateInterval;
    doc["owmCity"] = owmCity;

    // HA
    doc["haAddr"] = haAddr;
    doc["haApiHttpType"] = haApiHttpType;
    doc["haApiToken"] = base64_utils::encode(haApiToken); // Encode
    doc["haApiPort"] = haApiPort;
    doc["haUpdateInterval"] = haUpdateInterval;

    // DS18B20
    doc["ds18b20Enable"] = ds18b20Enable;
    doc["ds18b20UpdateInterval"] = ds18b20UpdateInterval;
    doc["ds18b20UnitsFormat"] = ds18b20UnitsFormat;

    // Stock Ticker
    doc["stockApiToken"] = base64_utils::encode(stockApiToken);
    doc["stockUpdateInterval"] = stockUpdateInterval;

    // Zones
    for (uint8_t i = 0; i < kZoneCapacity; i++) {
      addZoneSettingsToDoc(doc, i, true);
    }

    serializeJson(doc, *response);
    request->send(response);
  });

  // ─── Restore API ───────────────────────────────────────────────────────────
  server.on(
      "/api/restore", HTTP_POST,
      [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
        if (ESP.getFreeHeap() < 12288 || isFetchingHttps) {
          request->send(503, "application/json",
                        "{\"error\":\"Device busy, try again\"}");
          return;
        }
#endif
        if (restoreJsonBuffer.length() == 0) {
          request->send(400, "text/plain", "Empty backup");
          return;
        }
        DynamicJsonDocument doc(6144);
        DeserializationError error = deserializeJson(doc, restoreJsonBuffer);
        restoreJsonBuffer = ""; // Free memory

        if (error) {
          request->send(400, "text/plain", "Invalid JSON");
          return;
        }

        // Restore System
        if (doc.containsKey("disableServiceMessages"))
          disableServiceMessages = doc["disableServiceMessages"];
        if (doc.containsKey("zoneNumbers")) {
          int parsedZoneCount = doc["zoneNumbers"];
          if (parsedZoneCount < 1)
            parsedZoneCount = 1;
          if (parsedZoneCount > kZoneCapacity)
            parsedZoneCount = kZoneCapacity;
          zoneNumbers = static_cast<uint8_t>(parsedZoneCount);
        }
        if (doc.containsKey("intensity"))
          intensity = doc["intensity"];

        saveVarsToConfFile("systemSettings", 0);
        saveVarsToConfFile("displaySettings", 0);
        saveVarsToConfFile("intensity", 0);

        // Restore MQTT
        if (doc.containsKey("mqttEnable"))
          mqttEnable = doc["mqttEnable"];
        if (doc.containsKey("mqttServerAddress"))
          mqttServerAddress = doc["mqttServerAddress"].as<String>();
        if (doc.containsKey("mqttServerPort"))
          mqttServerPort = doc["mqttServerPort"];
        if (doc.containsKey("mqttUsername"))
          mqttUsername = doc["mqttUsername"].as<String>();
        if (doc.containsKey("mqttPassword")) {
          String enc = doc["mqttPassword"].as<String>();
          mqttPassword = base64_utils::decode(enc);
        }
        saveVarsToConfFile("mqttSettings", 0);

        // Restore NTP
        if (doc.containsKey("ntpTimeZone"))
          ntpTimeZone = doc["ntpTimeZone"];
        if (doc.containsKey("ntpUpdateInterval"))
          ntpUpdateInterval = doc["ntpUpdateInterval"];
        if (doc.containsKey("ntpServer"))
          ntpServer = doc["ntpServer"].as<String>();
        if (doc.containsKey("disableDotsBlink"))
          disableDotsBlink = doc["disableDotsBlink"];
        saveVarsToConfFile("wallClockSett", 0);

        // Restore OWM
        if (doc.containsKey("owmApiToken")) {
          String enc = doc["owmApiToken"].as<String>();
          owmApiToken = base64_utils::decode(enc);
        }
        if (doc.containsKey("owmUnitsFormat"))
          owmUnitsFormat = doc["owmUnitsFormat"].as<String>();
        if (doc.containsKey("owmUpdateInterval"))
          owmUpdateInterval = doc["owmUpdateInterval"];
        if (doc.containsKey("owmCity"))
          owmCity = doc["owmCity"].as<String>();
        saveVarsToConfFile("owmSettings", 0);

        // Restore HA
        if (doc.containsKey("haAddr"))
          haAddr = doc["haAddr"].as<String>();
        if (doc.containsKey("haApiHttpType"))
          haApiHttpType = doc["haApiHttpType"].as<String>();
        if (doc.containsKey("haApiToken")) {
          String enc = doc["haApiToken"].as<String>();
          haApiToken = base64_utils::decode(enc);
        }
        if (doc.containsKey("haApiPort"))
          haApiPort = doc["haApiPort"];
        if (doc.containsKey("haUpdateInterval"))
          haUpdateInterval = doc["haUpdateInterval"];
        saveVarsToConfFile("haSettings", 0);

        // Restore DS18B20
        if (doc.containsKey("ds18b20Enable"))
          ds18b20Enable = doc["ds18b20Enable"];
        if (doc.containsKey("ds18b20UpdateInterval"))
          ds18b20UpdateInterval = doc["ds18b20UpdateInterval"];
        if (doc.containsKey("ds18b20UnitsFormat"))
          ds18b20UnitsFormat = doc["ds18b20UnitsFormat"].as<String>();
        saveVarsToConfFile("ds18b20Settings", 0);

        // Restore Stock Ticker
        if (doc.containsKey("stockApiToken")) {
          String enc = doc["stockApiToken"].as<String>();
          stockApiToken = base64_utils::decode(enc);
        }
        if (doc.containsKey("stockUpdateInterval"))
          stockUpdateInterval = doc["stockUpdateInterval"];
        saveVarsToConfFile("stockSettings", 0);

        // Restore Zones
        for (uint8_t i = 0; i < kZoneCapacity; i++) {
          restoreZoneSettingsFromDoc(doc, i);
          saveVarsToConfFile("zoneSettings", i);
        }

        saveVarsToConfFile("displaySettings", 0);

        request->send(200, "text/plain", "OK");
        shouldReboot = true;
      },
      [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final) {
        if (index == 0) {
          restoreJsonBuffer = "";
          size_t contentLen = request->contentLength();
          if (contentLen > 0 && contentLen < 8192) {
            restoreJsonBuffer.reserve(contentLen + 1);
          }
        }
        for (size_t i = 0; i < len; i++) {
          restoreJsonBuffer += (char)data[i];
        }
      });

  // ─── Reboot / Reset WiFi ──────────────────────────────────────────────────
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    shouldReboot = true;
  });

  server.on("/api/resetWifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    if (wifiManagerPtr)
      wifiManagerPtr->resetSettings();
    shouldReboot = true;
  });

  // ─── Zone Settings JSON API
  // ──────────────────────────────────────────────────
  server.on("/api/zone-settings", HTTP_GET, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
    if (ESP.getFreeHeap() < 12288 || isFetchingHttps) {
      request->send(503, "application/json",
                    "{\"error\":\"Device busy, try again\"}");
      return;
    }
#endif
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument doc(6144);

    doc["deviceName"] = shortMACaddr;
#if defined(ESP32)
    doc["platform"] = "ESP32";
#elif defined(ESP8266)
    doc["platform"] = "ESP8266";
#else
    doc["platform"] = "Unknown";
#endif
    doc["firmwareVer"] = firmwareVer;
    doc["zoneNumbers"] = zoneNumbers;
    doc["intensity"] = intensity + 1;

    // Info Table
    doc["wifiSsid"] = WiFi.SSID();
    doc["wifiIp"] = WiFi.localIP().toString();
    doc["wifiGateway"] = WiFi.gatewayIP().toString();
    doc["wifiRssi"] = WiFi.RSSI();
    doc["mqttDevicePrefix"] = MQTTGlobalPrefix;
    doc["ds18b20Temp"] = (dsTemp == "-127") ? "Not connected" : dsTemp;

    // Zones
    for (uint8_t i = 0; i < kZoneCapacity; i++) {
      addZoneSettingsToDoc(doc, i, false);
    }

    doc["stockShowArrows"] = zones[0].stockShowArrows;

    serializeJson(doc, *response);
    request->send(response);
  });

  // ─── Service Settings JSON API ─────────────────────────────────────────────
  server.on(
      "/api/service-settings", HTTP_GET, [](AsyncWebServerRequest *request) {
#if defined(ESP8266)
        if (ESP.getFreeHeap() < 12288 || isFetchingHttps) {
          request->send(503, "application/json",
                        "{\"error\":\"Device busy, try again\"}");
          return;
        }
#endif
        AsyncResponseStream *response =
            request->beginResponseStream("application/json");
        DynamicJsonDocument doc(3072);

        // System Settings
        doc["deviceName"] = shortMACaddr;
#if defined(ESP32)
        doc["platform"] = "ESP32";
#elif defined(ESP8266)
        doc["platform"] = "ESP8266";
#else
        doc["platform"] = "Unknown";
#endif
        doc["firmwareVer"] = firmwareVer;
        doc["disableServiceMessages"] = disableServiceMessages;
        doc["disableDotsBlink"] = disableDotsBlink;

        // WiFi
        doc["wifiSsid"] = WiFi.SSID();
        doc["wifiIp"] = WiFi.localIP().toString();
        doc["wifiGateway"] = WiFi.gatewayIP().toString();
        doc["wifiRssi"] = WiFi.RSSI();

        // MQTT
        doc["mqttEnable"] = mqttEnable;
        doc["mqttServerAddress"] = mqttServerAddress;
        doc["mqttServerPort"] = mqttServerPort;
        doc["mqttUsername"] = mqttUsername;
        doc["mqttPassword"] = (mqttPassword == "") ? "" : "********";
        doc["mqttDevicePrefix"] = MQTTGlobalPrefix;

        // NTP
        doc["ntpTimeZone"] = ntpTimeZone;
        doc["ntpUpdateInterval"] = ntpUpdateInterval;
        doc["ntpServer"] = ntpServer;

        // OWM
        doc["owmApiToken"] = (owmApiToken == "") ? "" : "********";
        doc["owmUnitsFormat"] = owmUnitsFormat;
        doc["owmUpdateInterval"] = owmUpdateInterval;
        doc["owmCity"] = owmCity;

        // HA
        doc["haAddr"] = haAddr;
        doc["haApiHttpType"] = haApiHttpType;
        doc["haApiToken"] = (haApiToken == "") ? "" : "********";
        doc["haApiPort"] = haApiPort;
        doc["haUpdateInterval"] = haUpdateInterval;

        // DS18B20
        doc["ds18b20Enable"] = ds18b20Enable;
        doc["ds18b20Temp"] = (dsTemp == "-127") ? "Not connected" : dsTemp;
        doc["ds18b20UpdateInterval"] = ds18b20UpdateInterval;
        doc["ds18b20UnitsFormat"] = ds18b20UnitsFormat;

        // Stock Ticker
        doc["stockApiToken"] = (stockApiToken == "") ? "" : "********";
        doc["stockUpdateInterval"] = stockUpdateInterval;

        // Zone count (needed for PB form generation)
        doc["zoneNumbers"] = zoneNumbers;

        // Progress Bar
        for (uint8_t i = 0; i < kZoneCapacity; i++) {
          char key[24];
          snprintf(key, sizeof(key), "pbEnableZone%u", i);
          doc[key] = progressBars[i].enabled;
          snprintf(key, sizeof(key), "pbSrcTypeZone%u", i);
          doc[key] = progressBars[i].dataSourceType;
          snprintf(key, sizeof(key), "pbSrcIdZone%u", i);
          doc[key] = progressBars[i].dataSourceId;
          snprintf(key, sizeof(key), "pbMinZone%u", i);
          doc[key] = progressBars[i].minValue;
          snprintf(key, sizeof(key), "pbMaxZone%u", i);
          doc[key] = progressBars[i].maxValue;
          snprintf(key, sizeof(key), "pbCondEnZone%u", i);
          doc[key] = progressBars[i].conditionEnabled;
          snprintf(key, sizeof(key), "pbCondSrcTZone%u", i);
          doc[key] = progressBars[i].conditionSourceType;
          snprintf(key, sizeof(key), "pbCondSrcIZone%u", i);
          doc[key] = progressBars[i].conditionSourceId;
          snprintf(key, sizeof(key), "pbCondValZone%u", i);
          doc[key] = progressBars[i].conditionValue;
        }

        serializeJson(doc, *response);
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
    if (ESP.getFreeHeap() < 12288 || isFetchingHttps) {
      request->send(503, "application/json",
                    "{\"error\":\"Device busy, try again\"}");
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
    if (ESP.getFreeHeap() < 12288 || isFetchingHttps) {
      request->send(503, "application/json",
                    "{\"error\":\"Device busy, try again\"}");
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
    bool zoneUpdate[kZoneCapacity] = {false};

    if (keyValue == "zoneSettings" && !resolveZoneIndexFromRequest(request, n)) {
      request->send(400, "application/json",
                    "{\"error\":\"Missing or invalid zone\"}");
      return;
    }

    for (int i = 0; i < params; i++) {
      const AsyncWebParameter *p = request->getParam((size_t)i);
      if (!p || !p->isPost() || p->name() == "key")
        continue;

      Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

        if (keyValue == "systemSettings") {
          if (p->name() == "disableServiceMessages")
            disableServiceMessages = parseBoolParam(p->value());
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
            zoneNumbers = static_cast<uint8_t>(parsedZoneCount);
          }
          if (p->name() == "zone0Begin")
            zones[0].begin = p->value().toInt();
          if (p->name() == "zone0End")
            zones[0].end = p->value().toInt();
          if (p->name() == "zone1Begin")
            zones[1].begin = p->value().toInt();
          if (p->name() == "zone1End")
            zones[1].end = p->value().toInt();
          if (p->name() == "zone2Begin")
            zones[2].begin = p->value().toInt();
          if (p->name() == "zone2End")
            zones[2].end = p->value().toInt();
          if (p->name() == "zone3Begin")
            zones[3].begin = p->value().toInt();
          if (p->name() == "zone3End")
            zones[3].end = p->value().toInt();
          if (p->name() == "intensity")
            P.setIntensity(p->value().toInt());

          finishRequest = true;
        }

        if (keyValue == "zoneSettings") {
          if (n < kZoneCapacity) {
            if (p->name() == "workMode") {
              String oldWorkMode = zones[n].workMode;
              zones[n].workMode = p->value().c_str();
              if (zones[n].workMode == "owmWeather" &&
                  oldWorkMode != "owmWeather")
                owmLastTime = -1000000;
              if (zones[n].workMode == "haClient" && oldWorkMode != "haClient")
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

              if (!disableServiceMessages) {
                showModeMessage = true;
              }
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
            String val = p->value();
            Serial.print("Debug countdown: received target='" + val +
                         "' for zone ");
            Serial.println(n);
            if (val.length() > 0) {
              bool ok = parseCountdownTarget(val, n, timeClient.getEpochTime(),
                                             ntpTimeZone);
              Serial.print("Debug countdown: parseCountdownTarget returned ");
              Serial.print(ok ? "true" : "false");
              Serial.print(", target epoch=");
              Serial.println(countdownState[n].target);
            }
            zones[n].curTime = "";
            zones[n].forceUpdate = true;
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
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      if (keyValue == "zoneSettings") {
        configDirty = true;
        configDirtyTime = millis();
      } else {
        saveVarsToConfFile(keyValue, n);
      }

      // Immediate progress bar update after settings change
      if (keyValue == "pbSettings") {
        MD_MAX72XX *mx = P.getGraphicObject();
        for (uint8_t z = 0; z < zoneNumbers && z < 4; z++) {
          // Re-evaluate condition state
          if (progressBars[z].conditionEnabled) {
            // Condition enabled — reset to false until actual data confirms it
            progressBars[z].conditionMet = false;
          } else {
            // No condition — always active
            progressBars[z].conditionMet = true;
          }
          // Reset update timers to force immediate HA poll on next loop
          progressBars[z].lastDataUpdate = 0;
          progressBars[z].lastCondUpdate = 0;
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
            if (progressBars[z].enabled) {
              if (progressBars[z].dataSourceType == "mqtt" &&
                  progressBars[z].dataSourceId.length() > 0) {
                mqttClient.subscribe(progressBars[z].dataSourceId.c_str());
              }
              if (progressBars[z].conditionEnabled &&
                  progressBars[z].conditionSourceType == "mqtt" &&
                  progressBars[z].conditionSourceId.length() > 0) {
                mqttClient.subscribe(progressBars[z].conditionSourceId.c_str());
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
      if (keyValue == "displaySettings") {
        restartESP = true;
      }
    }
  });

  // ─── Factory Reset ────────────────────────────────────────────────────────
  server.on("/api/factory-reset", HTTP_POST,
            [](AsyncWebServerRequest *request) {
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

              if (wifiManagerPtr)
                wifiManagerPtr->resetSettings();
              WiFi.disconnect(true);

              request->send(200, "application/json",
                            "{\"status\":\"ok\",\"message\":\"Factory reset "
                            "complete. Restarting...\"}");

              // Schedule restart
              restartESP = true;
            });
}
