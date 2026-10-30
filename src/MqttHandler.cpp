#include "MqttHandler.h"
#include "ConfigStore.h"
#include "DisplayUtils.h"
#include "sharedScratch.h"
#include <cctype>
#include <cstring>

namespace {
bool mqttDiscoveryPublished = false;
bool mqttClearRetainedPower = false;
bool mqttClearRetainedIntensity = false;

bool clearRetainedCommand(const String &topic, const __FlashStringHelper *name) {
  if (topic.length() == 0)
    return true;
  static const uint8_t emptyPayload = 0;
  bool cleared = mqttClient.publish(topic.c_str(), &emptyPayload, 0, true);
  Serial.print(F("\n[MQTT] Clear retained "));
  Serial.print(name);
  Serial.print(cleared ? F(" command: OK") : F(" command: FAILED"));
  return cleared;
}

bool clearPendingScheduledCommands() {
  if (!mqttClient.connected())
    return false;
  if (mqttClearRetainedPower) {
    if (!clearRetainedCommand(MQTTPower, F("power")))
      return false;
    mqttClearRetainedPower = false;
  }
  if (mqttClearRetainedIntensity) {
    if (!clearRetainedCommand(MQTTIntensity, F("intensity")))
      return false;
    mqttClearRetainedIntensity = false;
  }
  return true;
}

bool mqttHasHeadroom(uint32_t minFreeHeap, uint32_t minMaxBlock) {
#if defined(ESP8266)
  return ESP.getFreeHeap() >= minFreeHeap &&
         ESP.getMaxFreeBlockSize() >= minMaxBlock;
#else
  (void)minFreeHeap;
  (void)minMaxBlock;
  return true;
#endif
}

void logMqttLowMem(const char *where) {
#if defined(ESP8266)
  Serial.printf_P(PSTR("\n[MQTT] Skip %s due low mem. Heap=%u MaxBlock=%u Frag=%u%%"), where,
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                ESP.getHeapFragmentation());
#else
  Serial.printf_P(PSTR("\n[MQTT] Skip %s due low mem"), where);
#endif
}

bool mqttTopicEquals(const char *topic, const String &expected) {
  return topic && expected.length() > 0 &&
         strcmp(topic, expected.c_str()) == 0;
}

bool mqttPayloadIsUnsignedInt(const char *payload) {
  if (!payload || *payload == '\0')
    return false;
  for (const char *p = payload; *p; ++p) {
    if (!isdigit(static_cast<unsigned char>(*p)))
      return false;
  }
  return true;
}

bool mqttPayloadEqualsIgnoreCase(const char *payload, const char *literal) {
  if (!payload || !literal)
    return false;
  while (*payload && *literal) {
    if (tolower(static_cast<unsigned char>(*payload)) !=
        tolower(static_cast<unsigned char>(*literal))) {
      return false;
    }
    ++payload;
    ++literal;
  }
  return (*payload == '\0' && *literal == '\0');
}

bool mqttPayloadIsTrue(const char *payload) {
  return mqttPayloadEqualsIgnoreCase(payload, "on") ||
         mqttPayloadEqualsIgnoreCase(payload, "true") ||
         strcmp(payload, "1") == 0;
}

bool mqttPayloadIsFalse(const char *payload) {
  return mqttPayloadEqualsIgnoreCase(payload, "off") ||
         mqttPayloadEqualsIgnoreCase(payload, "false") ||
         strcmp(payload, "0") == 0;
}
} // namespace

void MQTTProtectScheduledState(bool powerChanged, bool intensityChanged) {
  mqttClearRetainedPower |= powerChanged;
  mqttClearRetainedIntensity |= intensityChanged;
  shouldMqttPublish = true;

  // If MQTT is currently connected, delete stale retained commands now.
  // Otherwise reconnect() performs this before subscribing to command topics.
  if (mqttClient.connected())
    clearPendingScheduledCommands();
}

// ─── HA Discovery
// ──────────────────────────────────────────────────────────────
void MQTTPublishHADiscovry(const String &zone, const String &device_type) {
  if (mqttClient.connected()) {
    if (!mqttHasHeadroom(6000, 2500)) {
      logMqttLowMem("MQTTPublishHADiscovry");
      return;
    }

    StaticJsonDocument<2048> &root = sharedJsonDocument();
    root.clear();
    char topic_config[120];
    char *buffer = sharedJsonBuffer();
    size_t bufferCapacity = sharedJsonBufferCapacity();
    if (!buffer) {
      Serial.println(F("[MQTT] Discovery deferred: no serialization buffer"));
      return;
    }

    // device block
    JsonObject device = root.createNestedObject("dev");
    JsonArray arrDevice = device.createNestedArray("ids");
    arrDevice.add(MQTTGlobalPrefix);
    device["mf"] = "widapro";
    device["mdl"] = "wledPixel-v2";
    device["name"] = "wledPixel" + shortMACaddr;
    device["sw"] = firmwareVer;

    // availability block
    JsonArray arrAvailability = root.createNestedArray("availability");
    JsonObject availability = arrAvailability.createNestedObject();
    availability["topic"] = MQTTGlobalPrefix + "/state";
    availability["value_template"] = "{{ value_json.status }}";

    // light block
    if (device_type == "light") {
      sprintf(topic_config, "homeassistant/light/%s/config",
              shortMACaddr.c_str());
      root["name"] = MQTTGlobalPrefix;
      root["uniq_id"] = "wledPixelLight-" + shortMACaddr;
      root["cmd_t"] = "wledPixel-" + shortMACaddr + "/power";
      root["pl_off"] = "OFF";
      root["pl_on"] = "ON";
      root["brightness"] = "true";
      root["bri_stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["bri_val_tpl"] = "{{ value_json['brightness'] }}";
      root["bri_cmd_t"] = "wledPixel-" + shortMACaddr + "/intensity";
      root["bri_scl"] = 16;
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["stat_val_tpl"] = "{{ 'ON' if value_json.power else 'OFF' }}";
      root["json_attr_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["qos"] = 1;
      root["ret"] = true;
      root["ic"] = "mdi:view-comfy";
    }

    if (device_type == "workMode") {
      sprintf(topic_config, "homeassistant/select/%s/zone%sWorkMode/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " work mode";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "WorkMode";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/workmode";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] = "{{ value_json['zone" + zone + "']['workMode'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions = root.createNestedArray("options");
      arrOptions.add("manualInput");
      arrOptions.add("wallClock");
      arrOptions.add("haClient");
      arrOptions.add("mqttClient");
      arrOptions.add("owmWeather");
      arrOptions.add("intTempSensor");
      arrOptions.add("wopr");
      arrOptions.add("countdown");
    }

    if (device_type == "scrollAlign") {
      sprintf(topic_config, "homeassistant/select/%s/zone%sScrollAlign/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " scroll align";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollAlign";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrollalign";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] = "{{ value_json['zone" + zone + "']['scrollAlign'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions = root.createNestedArray("options");
      arrOptions.add("PA_LEFT");
      arrOptions.add("PA_RIGHT");
      arrOptions.add("PA_CENTER");
    }

    if (device_type == "scrollEffectIn") {
      sprintf(topic_config,
              "homeassistant/select/%s/zone%sScrollEffectIn/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " scroll effect IN";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollEffectIn";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrolleffectIn";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] =
          "{{ value_json['zone" + zone + "']['scrollEffectIn'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions = root.createNestedArray("options");
      arrOptions.add("PA_NO_EFFECT");
      arrOptions.add("PA_PRINT");
      arrOptions.add("PA_SCROLL_UP");
      arrOptions.add("PA_SCROLL_DOWN");
      arrOptions.add("PA_SCROLL_LEFT");
      arrOptions.add("PA_SCROLL_RIGHT");
      arrOptions.add("PA_SLICE");
      arrOptions.add("PA_MESH");
      arrOptions.add("PA_FADE");
      arrOptions.add("PA_DISSOLVE");
      arrOptions.add("PA_BLINDS");
      arrOptions.add("PA_RANDOM");
      arrOptions.add("PA_WIPE");
      arrOptions.add("PA_WIPE_CURSOR");
      arrOptions.add("PA_SCAN_HORIZ");
      arrOptions.add("PA_SCAN_VERT");
      arrOptions.add("PA_OPENING");
      arrOptions.add("PA_OPENING_CURSOR");
      arrOptions.add("PA_CLOSING");
      arrOptions.add("PA_CLOSING_CURSOR");
      arrOptions.add("PA_SCROLL_UP_LEFT");
      arrOptions.add("PA_SCROLL_UP_RIGHT");
      arrOptions.add("PA_SCROLL_DOWN_LEFT");
      arrOptions.add("PA_SCROLL_DOWN_RIGHT");
      arrOptions.add("PA_GROW_UP");
      arrOptions.add("PA_GROW_DOWN");
    }

    if (device_type == "scrollEffectOut") {
      sprintf(topic_config,
              "homeassistant/select/%s/zone%sScrollEffectOut/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " scroll Effect Out";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollEffectOut";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrolleffectOut";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] =
          "{{ value_json['zone" + zone + "']['scrollEffectOut'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions = root.createNestedArray("options");
      arrOptions.add("PA_NO_EFFECT");
      arrOptions.add("PA_PRINT");
      arrOptions.add("PA_SCROLL_UP");
      arrOptions.add("PA_SCROLL_DOWN");
      arrOptions.add("PA_SCROLL_LEFT");
      arrOptions.add("PA_SCROLL_RIGHT");
      arrOptions.add("PA_SLICE");
      arrOptions.add("PA_MESH");
      arrOptions.add("PA_FADE");
      arrOptions.add("PA_DISSOLVE");
      arrOptions.add("PA_BLINDS");
      arrOptions.add("PA_RANDOM");
      arrOptions.add("PA_WIPE");
      arrOptions.add("PA_WIPE_CURSOR");
      arrOptions.add("PA_SCAN_HORIZ");
      arrOptions.add("PA_SCAN_VERT");
      arrOptions.add("PA_OPENING");
      arrOptions.add("PA_OPENING_CURSOR");
      arrOptions.add("PA_CLOSING");
      arrOptions.add("PA_CLOSING_CURSOR");
      arrOptions.add("PA_SCROLL_UP_LEFT");
      arrOptions.add("PA_SCROLL_UP_RIGHT");
      arrOptions.add("PA_SCROLL_DOWN_LEFT");
      arrOptions.add("PA_SCROLL_DOWN_RIGHT");
      arrOptions.add("PA_GROW_UP");
      arrOptions.add("PA_GROW_DOWN");
    }

    if (device_type == "charspacing") {
      sprintf(topic_config, "homeassistant/number/%s/zone%sCharspacing/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " charspacing";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "Charspacing";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/charspacing";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] = "{{ value_json['zone" + zone + "']['charspacing'] }}";
      root["entity_category"] = "config";
      root["min"] = 0;
      root["max"] = 3;
    }

    if (device_type == "scrollPause") {
      sprintf(topic_config, "homeassistant/number/%s/zone%sScrollPause/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " scroll pause";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollPause";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrollpause";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] = "{{ value_json['zone" + zone + "']['scrollPause'] }}";
      root["entity_category"] = "config";
      root["min"] = 0;
      root["max"] = 30000;
    }

    if (device_type == "scrollSpeed") {
      sprintf(topic_config, "homeassistant/number/%s/zone%sScrollSpeed/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " scroll speed";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollSpeed";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrollspeed";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] = "{{ value_json['zone" + zone + "']['scrollSpeed'] }}";
      root["entity_category"] = "config";
      root["min"] = 0;
      root["max"] = 100;
    }

    if (device_type == "scrollInfinite") {
      sprintf(topic_config,
              "homeassistant/switch/%s/zone%sScrollInfinite/config",
              shortMACaddr.c_str(), zone.c_str());
      root["name"] = "Zone" + zone + " infinite scroll";
      root["uniq_id"] =
          "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollInfinite";
      root["cmd_t"] =
          "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrollInfinite";
      root["stat_t"] = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"] =
          "{{ value_json['zone" + zone + "']['scrollInfinite'] }}";
      root["entity_category"] = "config";
      root["pl_on"] = "ON";
      root["pl_off"] = "OFF";
      root["icon"] = "mdi:repeat";
    }

    if (device_type == "ds18b20") {
      sprintf(topic_config, "homeassistant/sensor/%s/temperature/config",
              shortMACaddr.c_str());
      root["name"] = "wledPixel-" + shortMACaddr + "_Temperature";
      root["uniq_id"] = "wledPixelLight" + shortMACaddr + "Temperature";
      root["state_topic"] = "wledPixel-" + shortMACaddr + "/temperature";
      root["device_class"] = "temperature";
      root["unit_of_measurement"] = "°C";
    }

    if (root.overflowed()) {
      Serial.println(F("\n[MQTT] Discovery JSON overflow; skip publish"));
      return;
    }

    size_t n = serializeJson(root, buffer, bufferCapacity);
    if (n == 0 || n >= bufferCapacity) {
      Serial.println(F("\n[MQTT] Discovery payload too large; skip publish"));
      return;
    }

    Serial.printf_P(PSTR("\n[MQTT] HA discovery publish type=%s topic=%s bytes=%u"),
                  device_type.c_str(), topic_config, static_cast<unsigned>(n));
    Serial.print(F("\nMQTT publish HA device state: "));
    mqttPublished = mqttClient.publish(topic_config, buffer, n);
    if (mqttPublished)
      Serial.println(F("OK"));
    else
      Serial.println(F("fail"));

    yield();
  }
}

// ─── Publish State
// ─────────────────────────────────────────────────────────────
void MQTTPublishState() {
  Serial.print(F("MQTT pub state func"));
  if (mqttClient.connected()) {
    if (!mqttHasHeadroom(6000, 2500)) {
      logMqttLowMem("MQTTPublishState");
      return;
    }

    StaticJsonDocument<2048> &doc = sharedJsonDocument();
    doc.clear();
    char *buffer = sharedJsonBuffer();
    size_t bufferCapacity = sharedJsonBufferCapacity();
    if (!buffer) {
      Serial.println(F("[MQTT] State publish deferred: no serialization buffer"));
      return;
    }

    doc["status"] = "online";
    doc["power"] = power;
    doc["brightness"] = intensity + 1;
    doc["wifiSsid"] = WiFi.SSID();
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      char zoneKey[8];
      snprintf_P(zoneKey, sizeof(zoneKey), PSTR("zone%u"), n);
      JsonObject zoneObj = doc.createNestedObject(zoneKey);
      zoneObj["workMode"] = zones[n].workMode;
      zoneObj["charspacing"] = zones[n].charspacing;
      zoneObj["scrollAlign"] = zones[n].scrollAlign;
      zoneObj["scrollPause"] = zones[n].scrollPause;
      zoneObj["scrollSpeed"] = zones[n].scrollSpeed;
      zoneObj["scrollEffectIn"] = zones[n].scrollEffectIn;
      zoneObj["scrollEffectOut"] = zones[n].scrollEffectOut;
      zoneObj["scrollInfinite"] = zones[n].scrollInfinite ? "ON" : "OFF";
    }

    if (doc.overflowed()) {
      Serial.println(F("\n[MQTT] State JSON overflow; skip publish"));
      return;
    }

    serializeJson(doc, Serial);
    size_t n = serializeJson(doc, buffer, bufferCapacity);
    if (n == 0 || n >= bufferCapacity) {
      Serial.println(F("\n[MQTT] State payload too large; skip publish"));
      return;
    }

    Serial.print(F("MQTT publish device state: "));
    mqttPublished = mqttClient.publish(MQTTStateTopic.c_str(), buffer, n);
    if (mqttPublished)
      Serial.println(F("OK"));
    else
      Serial.println(F("fail"));

    yield();
  }
}

// ─── MQTT Callback
// ─────────────────────────────────────────────────────────────
void MQTTCallback(char *topic, byte *payload, int length) {
  if (!mqttHasHeadroom(4500, 1500)) {
    logMqttLowMem("MQTTCallback");
    return;
  }
  if (!topic || length <= 0 || length > 512) {
    Serial.println(F("\n[MQTT] Ignore payload: invalid length"));
    return;
  }
  char payloadBuf[513];
  memcpy(payloadBuf, payload, static_cast<size_t>(length));
  payloadBuf[length] = '\0';

  Serial.print(F("\nMQTT new message arrived"));
  Serial.print(F("\nTopic: "));
  Serial.println(topic);
  Serial.println(payloadBuf);
  bool shouldPublishState = false;

  if (mqttTopicEquals(topic, MQTTIntensity)) {
    if (mqttPayloadIsUnsignedInt(payloadBuf)) {
      int level = atoi(payloadBuf);
      if (level > 0) {
        intensity = level - 1;
        P.setIntensity(intensity);
        queueConfigSave("intensity", 99);
        if (!power) {
          Serial.print(F("\n[MQTT] Brightness updated while power is OFF; "
                         "display remains OFF"));
        }
        shouldPublishState = true;
      }
      if (level == 0) {
        P.displayShutdown(1);
        power = false;
        shouldPublishState = true;
      }
    } else {
      Serial.print(F("Supports are only numeric values"));
    }
  }
  if (mqttTopicEquals(topic, MQTTPower)) {
    if (mqttPayloadIsTrue(payloadBuf)) {
      P.displayShutdown(0);
      P.setIntensity(intensity);
      power = true;
      shouldPublishState = true;
    }
    if (mqttPayloadIsFalse(payloadBuf)) {
      P.displayShutdown(1);
      power = false;
      shouldPublishState = true;
    }
  }

  for (uint8_t n = 0; n < zoneNumbers; n++) {
    bool zoneSettingsChanged = false;
    if (zones[n].workMode == "mqttClient" &&
        mqttTopicEquals(topic, MQTTZones[n].message))
      zoneNewMessage(n, payloadBuf, zones[n].mqttPostfix, true, true);

    if (mqttTopicEquals(topic, MQTTZones[n].scrollEffectIn)) {
      zones[n].scrollEffectIn = payloadBuf;
      migrateScrollEffect(zones[n].scrollEffectIn);
      updateSpriteData(n);
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                      stringToTextEffectT(zones[n].scrollEffectOut));
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].scrollEffectOut)) {
      zones[n].scrollEffectOut = payloadBuf;
      migrateScrollEffect(zones[n].scrollEffectOut);
      updateSpriteData(n);
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                      stringToTextEffectT(zones[n].scrollEffectOut));
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].scrollSpeed)) {
      zones[n].scrollSpeed = atoi(payloadBuf);
      P.setSpeed(n, zones[n].scrollSpeed);
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].scrollPause)) {
      zones[n].scrollPause = atoi(payloadBuf);
      P.setPause(n, zones[n].scrollPause * 1000);
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].scrollAllign)) {
      zones[n].scrollAlign = payloadBuf;
      P.setTextAlignment(n, stringToTextPositionT(zones[n].scrollAlign));
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].charspacing)) {
      zones[n].charspacing = atoi(payloadBuf);
      P.setCharSpacing(n, zones[n].charspacing);
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].workMode)) {
      zones[n].workMode = payloadBuf;
      zones[n].owmWhatToDisplay = "owmTemperature";

      if (zones[n].workMode == "countdown") {
        zones[n].previousMillis = -1000000;
        zones[n].curTime = "";
        countdownState[n].reached = false;
        countdownState[n].blinkCount = 0;
        if (!disableServiceMessages)
          zoneShowModeMessage(n, "Timer");
      }
      if (zones[n].workMode == "wallClock") {
        timeClient.setTimeOffset(ntpTimeZone * 3600);
        ntpUpdateTime();
        zones[n].previousMillis = -1000000;
        zones[n].curTime = "";
      }
      if (zones[n].workMode == "wopr") {
        woprZones[n].active = true;
        P.displayClear(n);
        P.setTextEffect(n, PA_NO_EFFECT, PA_NO_EFFECT);
      } else {
        woprZones[n].active = false;
      }

      Serial.print(F("Debug: Processing mode update for zone "));
      Serial.print(n);
      Serial.print(F(". New mode: "));
      Serial.println(zones[n].workMode);

      if (!disableServiceMessages) {
        if (zones[n].workMode == "mqttClient")
          zoneShowModeMessage(n, "MQTT");
        if (zones[n].workMode == "manualInput")
          zoneShowModeMessage(n, "Manual");
        if (zones[n].workMode == "haClient")
          zoneShowModeMessage(n, "HA");
        if (zones[n].workMode == "owmWeather")
          zoneShowModeMessage(n, "OWM");
        if (zones[n].workMode == "intTempSensor")
          zoneShowModeMessage(n, "TempS");
      }

      if (zones[n].workMode == "mqttClient") {
        if (mqttClient.connected() && MQTTZones[n].message != "" &&
            MQTTZones[n].message != " ") {
          mqttClient.subscribe(MQTTZones[n].message.c_str());
        }
      }
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].scrollInfinite)) {
      if (mqttPayloadIsTrue(payloadBuf))
        zones[n].scrollInfinite = true;
      else
        zones[n].scrollInfinite = false;

      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].countdown)) {
      parseCountdownTarget(payloadBuf, n, timeClient.getEpochTime(),
                           ntpTimeZone);
      zones[n].workMode = "countdown";
      zones[n].previousMillis = -1000000;
      zones[n].curTime = "";
      zoneSettingsChanged = true;
    }
    if (mqttTopicEquals(topic, MQTTZones[n].countdownFormat)) {
      zones[n].countdownFormat = payloadBuf;
      zones[n].curTime = "";
      zoneSettingsChanged = true;
    }
    if (zoneSettingsChanged) {
      configDirty = true;
      configDirtyTime = millis();
      shouldPublishState = true;
    }
  }

  // ── Progress Bar MQTT handling ──
  for (uint8_t i = 0; i < zoneNumbers && i < 4; i++) {
    if (!progressBars[i].enabled)
      continue;

    // Data source (Regular mode only)
    if (!progressBars[i].dynamicMode &&
        progressBars[i].dataSourceType == "mqtt" &&
        progressBars[i].dataSourceId.length() > 0 &&
        mqttTopicEquals(topic, progressBars[i].dataSourceId)) {
      progressBars[i].currentValue = atof(payloadBuf);
      Serial.printf_P(PSTR("\n[PB] Zone %u data: %s -> %.2f"), i,
                    payloadBuf, progressBars[i].currentValue);
    }

    // Condition source (Regular mode only)
    if (!progressBars[i].dynamicMode &&
        progressBars[i].conditionEnabled &&
        progressBars[i].conditionSourceType == "mqtt" &&
        progressBars[i].conditionSourceId.length() > 0 &&
        mqttTopicEquals(topic, progressBars[i].conditionSourceId)) {
      progressBars[i].conditionMet = progressBars[i].conditionValue == payloadBuf;
      Serial.printf_P(PSTR("\n[PB] Zone %u condition: '%s' == '%s' -> %s"), i,
                    payloadBuf,
                    progressBars[i].conditionValue.c_str(),
                    progressBars[i].conditionMet ? "true" : "false");
    }

    // Dynamic animation source (Dynamic mode only)
    if (progressBars[i].dynamicMode &&
        progressBars[i].dynSourceType == "mqtt" &&
        progressBars[i].dynSourceId.length() > 0 &&
        mqttTopicEquals(topic, progressBars[i].dynSourceId)) {
      progressBars[i].dynCurrentSensorValue = payloadBuf;
      Serial.printf_P(PSTR("\n[PB] Zone %u dyn src: '%s'"), i, payloadBuf);
    }
  }

  if (shouldPublishState)
    MQTTPublishState();
}

// ─── Reconnect
// ─────────────────────────────────────────────────────────────────
boolean reconnect() {
  if (!mqttHasHeadroom(7000, 3000)) {
    logMqttLowMem("reconnect");
    return false;
  }
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);

  char lwtTopic[80];
  snprintf_P(lwtTopic, sizeof(lwtTopic), PSTR("%s/state"), MQTTGlobalPrefix.c_str());
  const char *lwtMessage = "{\"status\":\"offline\"}";

  if (mqttClient.connect(MQTTGlobalPrefix.c_str(), mqttUsername.c_str(),
                         mqttPassword.c_str(), lwtTopic, 1, true,
                         lwtMessage)) {
    // A scheduler profile is authoritative for explicitly changed power and
    // brightness. Remove older retained MQTT commands before subscribing;
    // otherwise every web-priority reconnect can replay stale ON/brightness.
    if (!clearPendingScheduledCommands()) {
      Serial.println(F("\n[MQTT] Retained command cleanup failed; reconnect "
                       "deferred to protect scheduled state"));
      mqttClient.disconnect();
      return false;
    }

    bool discoveryCompleted = true;
    if (!mqttDiscoveryPublished) {
      MQTTPublishHADiscovry("0", "light");
      yield();
      MQTTPublishHADiscovry("0", "ds18b20");
      yield();
    }

    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (!mqttDiscoveryPublished) {
        if (!mqttHasHeadroom(6000, 2500)) {
          logMqttLowMem("reconnect.discovery");
          discoveryCompleted = false;
          break;
        }
        char zoneBuf[4];
        snprintf_P(zoneBuf, sizeof(zoneBuf), PSTR("%u"), n);
        MQTTPublishHADiscovry(zoneBuf, "scrollAlign");
        yield();
        MQTTPublishHADiscovry(zoneBuf, "charspacing");
        yield();
#if !defined(ESP8266)
        MQTTPublishHADiscovry(zoneBuf, "workMode");
        yield();
#endif
        MQTTPublishHADiscovry(zoneBuf, "scrollPause");
        yield();
        MQTTPublishHADiscovry(zoneBuf, "scrollSpeed");
        yield();
#if !defined(ESP8266)
        MQTTPublishHADiscovry(zoneBuf, "scrollEffectIn");
        yield();
        MQTTPublishHADiscovry(zoneBuf, "scrollEffectOut");
        yield();
#endif
      }

      if (MQTTZones[n].message != "" && MQTTZones[n].message != " ")
        mqttClient.subscribe(MQTTZones[n].message.c_str());
      mqttClient.subscribe(MQTTZones[n].scrollEffectIn.c_str());
      mqttClient.subscribe(MQTTZones[n].scrollEffectOut.c_str());
      mqttClient.subscribe(MQTTZones[n].scrollSpeed.c_str());
      mqttClient.subscribe(MQTTZones[n].scrollPause.c_str());
      mqttClient.subscribe(MQTTZones[n].scrollAllign.c_str());
      mqttClient.subscribe(MQTTZones[n].charspacing.c_str());
      mqttClient.subscribe(MQTTZones[n].workMode.c_str());
      mqttClient.subscribe(MQTTZones[n].scrollInfinite.c_str());
      mqttClient.subscribe(MQTTZones[n].countdown.c_str());
      mqttClient.subscribe(MQTTZones[n].countdownFormat.c_str());
      yield();
    }

    // Subscribe to progress bar MQTT topics
    for (uint8_t i = 0; i < zoneNumbers && i < 4; i++) {
      if (!mqttHasHeadroom(5000, 1800)) {
        logMqttLowMem("reconnect.subscribe");
        break;
      }
      if (progressBars[i].enabled) {
        if (!progressBars[i].dynamicMode) {
          // Regular mode: subscribe to data + condition sources
          if (progressBars[i].dataSourceType == "mqtt" &&
              progressBars[i].dataSourceId.length() > 0) {
            mqttClient.subscribe(progressBars[i].dataSourceId.c_str());
          }
          if (progressBars[i].conditionEnabled &&
              progressBars[i].conditionSourceType == "mqtt" &&
              progressBars[i].conditionSourceId.length() > 0) {
            mqttClient.subscribe(progressBars[i].conditionSourceId.c_str());
          }
        } else {
          // Dynamic mode: subscribe to rules source
          if (progressBars[i].dynSourceType == "mqtt" &&
              progressBars[i].dynSourceId.length() > 0) {
            mqttClient.subscribe(progressBars[i].dynSourceId.c_str());
          }
        }
      }
      yield();
    }

    if (!mqttDiscoveryPublished && discoveryCompleted) {
      mqttDiscoveryPublished = true;
    }

    MQTTPublishState();
    Serial.println(F("MQTT subscribe objects"));
    mqttClient.subscribe(MQTTIntensity.c_str());
    mqttClient.subscribe(MQTTPower.c_str());

    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "mqttClient") {
        zoneShowModeMessage(n, "mqtt ok");
      }
    }
  } else {
    Serial.print(F("MQTT failed, rc= "));
    Serial.println(mqttClient.state());
    Serial.print(F(" try again in 5 seconds"));
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "mqttClient") {
        zoneShowModeMessage(n, "mqtt err");
      }
    }
  }
  return mqttClient.connected();
}
