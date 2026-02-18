#include "MqttHandler.h"
#include "ConfigStore.h"
#include "DisplayUtils.h"

// ─── HA Discovery
// ──────────────────────────────────────────────────────────────
void MQTTPublishHADiscovry(String zone, String device_type) {
  if (mqttClient.connected()) {
    DynamicJsonDocument root(1072);
    char topic_config[120];
    char buffer[1072];
    boolean result;

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

    size_t n = serializeJson(root, buffer);

    Serial.print(F("\nMQTT publish HA device state: "));
    mqttPublished = mqttClient.publish(topic_config, buffer, n);
    if (mqttPublished)
      Serial.println(F("OK"));
    else
      Serial.println(F("fail"));

    result = mqttClient.endPublish();
    if (result == false)
      Serial.print(F("\nERROR: MQTT HA config error end!"));

    root.shrinkToFit();
    root.garbageCollect();
  }
}

// ─── Publish State
// ─────────────────────────────────────────────────────────────
void MQTTPublishState() {
  Serial.print(F("MQTT pub state func"));
  if (mqttClient.connected()) {
    DynamicJsonDocument doc(1024);
    char buffer[1024];

    doc["status"] = "online";
    doc["power"] = power;
    doc["brightness"] = intensity + 1;
    doc["wifiSsid"] = WiFi.SSID();
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      doc["zone" + String(n)]["workMode"] = zones[n].workMode;
      doc["zone" + String(n)]["charspacing"] = zones[n].charspacing;
      doc["zone" + String(n)]["scrollAlign"] = zones[n].scrollAlign;
      doc["zone" + String(n)]["scrollPause"] = zones[n].scrollPause;
      doc["zone" + String(n)]["scrollSpeed"] = zones[n].scrollSpeed;
      doc["zone" + String(n)]["scrollEffectIn"] = zones[n].scrollEffectIn;
      doc["zone" + String(n)]["scrollEffectOut"] = zones[n].scrollEffectOut;
      doc["zone" + String(n)]["scrollInfinite"] =
          zones[n].scrollInfinite ? "ON" : "OFF";
    }

    serializeJson(doc, Serial);
    doc.shrinkToFit();
    doc.garbageCollect();
    size_t n = serializeJson(doc, buffer);

    Serial.print(F("MQTT publish device state: "));
    mqttPublished = mqttClient.publish(MQTTStateTopic.c_str(), buffer, n);
    if (mqttPublished)
      Serial.println(F("OK"));
    else
      Serial.println(F("fail"));
  }
}

// ─── MQTT Callback
// ─────────────────────────────────────────────────────────────
void MQTTCallback(char *topic, byte *payload, int length) {
  String topicStr(topic);
  String PayloadString;
  PayloadString.reserve(length + 1);
  for (int i = 0; i < length; i++) {
    PayloadString += (char)payload[i];
  }

  Serial.print(F("\nMQTT new message arrived"));
  Serial.print(F("\nTopic: "));
  Serial.println(topic);
  Serial.println(PayloadString);

  if (topicStr == MQTTIntensity) {
    if (isNumeric(PayloadString)) {
      if (PayloadString.toInt() > 0) {
        intensity = PayloadString.toInt() - 1;
        if (!power) {
          power = true;
          P.displayShutdown(0);
        }
        P.setIntensity(intensity);
        saveVarsToConfFile("intensity", 99);
        MQTTPublishState();
      }
      if (PayloadString.toInt() == 0) {
        P.displayShutdown(1);
        power = false;
        MQTTPublishState();
      }
    } else {
      Serial.print(F("Supports are only numeric values"));
    }
  }
  if (topicStr == MQTTPower) {
    if (PayloadString == "on" || PayloadString == "ON" ||
        PayloadString == "TRUE") {
      P.displayShutdown(0);
      P.setIntensity(intensity);
      power = true;
      MQTTPublishState();
    }
    if (PayloadString == "off" || PayloadString == "OFF" ||
        PayloadString == "FALSE") {
      P.displayShutdown(1);
      power = false;
      MQTTPublishState();
    }
  }

  for (uint8_t n = 0; n < zoneNumbers; n++) {
    bool zoneSettingsChanged = false;
    if (zones[n].workMode == "mqttClient" && topicStr == MQTTZones[n].message)
      zoneNewMessage(n, PayloadString.c_str(), zones[n].mqttPostfix, true,
                     true);

    if (topicStr == MQTTZones[n].scrollEffectIn) {
      zones[n].scrollEffectIn = PayloadString.c_str();
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                      stringToTextEffectT(zones[n].scrollEffectOut));
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].scrollEffectOut) {
      zones[n].scrollEffectOut = PayloadString.c_str();
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                      stringToTextEffectT(zones[n].scrollEffectOut));
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].scrollSpeed) {
      zones[n].scrollSpeed = PayloadString.toInt();
      P.setSpeed(n, zones[n].scrollSpeed);
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].scrollPause) {
      zones[n].scrollPause = PayloadString.toInt();
      P.setPause(n, zones[n].scrollPause * 1000);
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].scrollAllign) {
      zones[n].scrollAlign = PayloadString.c_str();
      P.setTextAlignment(n, stringToTextPositionT(zones[n].scrollAlign));
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].charspacing) {
      zones[n].charspacing = PayloadString.toInt();
      P.setCharSpacing(n, zones[n].charspacing);
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].workMode) {
      zones[n].workMode = PayloadString.c_str();
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

      Serial.print("Debug: Processing mode update for zone ");
      Serial.print(n);
      Serial.print(". New mode: ");
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
    if (topicStr == MQTTZones[n].scrollInfinite) {
      if (PayloadString == "on" || PayloadString == "ON" ||
          PayloadString == "TRUE" || PayloadString == "true" ||
          PayloadString == "1")
        zones[n].scrollInfinite = true;
      else
        zones[n].scrollInfinite = false;

      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].countdown) {
      parseCountdownTarget(PayloadString, n, timeClient.getEpochTime(),
                           ntpTimeZone);
      zones[n].workMode = "countdown";
      zones[n].previousMillis = -1000000;
      zones[n].curTime = "";
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].countdownFormat) {
      zones[n].countdownFormat = PayloadString;
      zones[n].curTime = "";
      zoneSettingsChanged = true;
    }
    if (zoneSettingsChanged) {
      configDirty = true;
      configDirtyTime = millis();
    }
  }
  MQTTPublishState();
}

// ─── Reconnect
// ─────────────────────────────────────────────────────────────────
boolean reconnect() {
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);

  String lwtTopic = MQTTGlobalPrefix + "/state";
  const char *lwtMessage = "{\"status\":\"offline\"}";

  if (mqttClient.connect(MQTTGlobalPrefix.c_str(), mqttUsername.c_str(),
                         mqttPassword.c_str(), lwtTopic.c_str(), 1, true,
                         lwtMessage)) {
    MQTTPublishHADiscovry("0", "light");
    MQTTPublishHADiscovry("0", "ds18b20");

    for (uint8_t n = 0; n < zoneNumbers; n++) {
      MQTTPublishHADiscovry(String(n), "scrollAlign");
      MQTTPublishHADiscovry(String(n), "charspacing");
      MQTTPublishHADiscovry(String(n), "workMode");
      MQTTPublishHADiscovry(String(n), "scrollPause");
      MQTTPublishHADiscovry(String(n), "scrollSpeed");
      MQTTPublishHADiscovry(String(n), "scrollEffectIn");
      MQTTPublishHADiscovry(String(n), "scrollEffectOut");

      if (MQTTZones[n].message != "" && MQTTZones[n].message != " ")
        mqttClient.subscribe((char *)MQTTZones[n].message.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].scrollEffectIn.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].scrollEffectOut.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].scrollSpeed.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].scrollPause.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].scrollAllign.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].charspacing.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].workMode.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].scrollInfinite.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].countdown.c_str());
      mqttClient.subscribe((char *)MQTTZones[n].countdownFormat.c_str());
    }
    MQTTPublishState();
    Serial.println(F("MQTT subscribe objects"));
    mqttClient.subscribe((char *)MQTTIntensity.c_str());
    mqttClient.subscribe((char *)MQTTPower.c_str());

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
