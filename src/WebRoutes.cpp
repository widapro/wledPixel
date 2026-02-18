#include "WebRoutes.h"
#include "ConfigStore.h"
#include "DisplayUtils.h"
#include "MqttHandler.h"

void setupWebRoutes() {
  // ─── Static Pages ──────────────────────────────────────────────────────────
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", indexPage);
  });

  server.on("/settings", HTTP_ANY, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginChunkedResponse(
        "text/html",
        [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
          size_t len1 = strlen_P(settingsPagePart1);
          size_t len2 = strlen_P(settingsPagePart2);

          if (index < len1) {
            size_t available = len1 - index;
            size_t toCopy = (available > maxLen) ? maxLen : available;
            memcpy_P(buffer, settingsPagePart1 + index, toCopy);
            return toCopy;
          } else if (index < len1 + len2) {
            size_t offset = index - len1;
            size_t available = len2 - offset;
            size_t toCopy = (available > maxLen) ? maxLen : available;
            memcpy_P(buffer, settingsPagePart2 + offset, toCopy);
            return toCopy;
          }
          return 0; // End of response
        });
    request->send(response);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", otaPage);
  });

  server.on("/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", backupPage);
  });

  // ─── Backup API ────────────────────────────────────────────────────────────
  server.on("/api/backup", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    String filename = "attachment; filename=wledPixel-";
    filename += shortMACaddr;
    filename += "_backup.json";
    response->addHeader("Content-Disposition", filename);
    DynamicJsonDocument doc(4096);

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

    // Zones
    for (int i = 0; i < 4; i++) {
      String z = "zone" + String(i);
      doc[z + "Begin"] = zones[i].begin;
      doc[z + "End"] = zones[i].end;
      doc["workModeZone" + String(i)] = zones[i].workMode;
      doc["scrollSpeedZone" + String(i)] = zones[i].scrollSpeed;
      doc["scrollPauseZone" + String(i)] = zones[i].scrollPause;
      doc["scrollAlignZone" + String(i)] = zones[i].scrollAlign;
      doc["scrollEffectZone" + String(i) + "In"] = zones[i].scrollEffectIn;
      doc["scrollEffectZone" + String(i) + "Out"] = zones[i].scrollEffectOut;
      doc["charspacingZone" + String(i)] = zones[i].charspacing;
      doc["fontZone" + String(i)] = zones[i].font;
      doc["mqttTextTopicZone" + String(i)] = MQTTZones[i].message;
      doc["mqttPostfixZone" + String(i)] = zones[i].mqttPostfix;
      doc["clockDisplayFormatZone" + String(i)] = zones[i].clockDisplayFormat;
      doc["owmWhatToDisplayZone" + String(i)] = zones[i].owmWhatToDisplay;
      doc["haSensorIdZone" + String(i)] = zones[i].haSensorId;
      doc["haSensorPostfixZone" + String(i)] = zones[i].haSensorPostfix;
      doc["ds18b20PostfixZone" + String(i)] = zones[i].ds18b20Postfix;
      doc["woprUpdateIntervalZone" + String(i)] = woprZones[i].updateInterval;
      doc["scrollInfiniteZone" + String(i)] = zones[i].scrollInfinite;
      // Countdown
      doc["countdownFormatZone" + String(i)] = zones[i].countdownFormat;
      doc["countdownFinishZone" + String(i)] = zones[i].countdownFinish;
      doc["countdownShowUnitsZone" + String(i)] = zones[i].countdownShowUnits;
      doc["countdownPrefixZone" + String(i)] = zones[i].countdownPrefix;
      doc["countdownSuffixZone" + String(i)] = zones[i].countdownSuffix;
      doc["countdownTargetZone" + String(i)] = countdownState[i].targetStr;
    }

    serializeJson(doc, *response);
    request->send(response);
  });

  // ─── Restore API ───────────────────────────────────────────────────────────
  server.on(
      "/api/restore", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        if (restoreJsonBuffer.length() == 0) {
          request->send(400, "text/plain", "Empty backup");
          return;
        }
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, restoreJsonBuffer);
        restoreJsonBuffer = ""; // Free memory

        if (error) {
          request->send(400, "text/plain", "Invalid JSON");
          return;
        }

        // Restore System
        if (doc.containsKey("disableServiceMessages"))
          disableServiceMessages = doc["disableServiceMessages"];
        if (doc.containsKey("zoneNumbers"))
          zoneNumbers = doc["zoneNumbers"];
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

        // Restore Zones
        for (int i = 0; i < 4; i++) {
          String z = "zone" + String(i);
          if (doc.containsKey(z + "Begin"))
            zones[i].begin = doc[z + "Begin"];
          if (doc.containsKey(z + "End"))
            zones[i].end = doc[z + "End"];
          if (doc.containsKey("workModeZone" + String(i)))
            zones[i].workMode = doc["workModeZone" + String(i)].as<String>();
          if (doc.containsKey("scrollSpeedZone" + String(i)))
            zones[i].scrollSpeed = doc["scrollSpeedZone" + String(i)];
          if (doc.containsKey("scrollPauseZone" + String(i)))
            zones[i].scrollPause = doc["scrollPauseZone" + String(i)];
          if (doc.containsKey("scrollAlignZone" + String(i)))
            zones[i].scrollAlign =
                doc["scrollAlignZone" + String(i)].as<String>();
          if (doc.containsKey("scrollEffectZone" + String(i) + "In"))
            zones[i].scrollEffectIn =
                doc["scrollEffectZone" + String(i) + "In"].as<String>();
          if (doc.containsKey("scrollEffectZone" + String(i) + "Out"))
            zones[i].scrollEffectOut =
                doc["scrollEffectZone" + String(i) + "Out"].as<String>();
          if (doc.containsKey("charspacingZone" + String(i)))
            zones[i].charspacing = doc["charspacingZone" + String(i)];
          if (doc.containsKey("fontZone" + String(i)))
            zones[i].font = doc["fontZone" + String(i)].as<String>();
          if (doc.containsKey("mqttTextTopicZone" + String(i)))
            MQTTZones[i].message =
                doc["mqttTextTopicZone" + String(i)].as<String>();
          if (doc.containsKey("mqttPostfixZone" + String(i)))
            zones[i].mqttPostfix =
                doc["mqttPostfixZone" + String(i)].as<String>();
          if (doc.containsKey("clockDisplayFormatZone" + String(i)))
            zones[i].clockDisplayFormat =
                doc["clockDisplayFormatZone" + String(i)].as<String>();
          if (doc.containsKey("owmWhatToDisplayZone" + String(i)))
            zones[i].owmWhatToDisplay =
                doc["owmWhatToDisplayZone" + String(i)].as<String>();
          if (doc.containsKey("haSensorIdZone" + String(i)))
            zones[i].haSensorId =
                doc["haSensorIdZone" + String(i)].as<String>();
          if (doc.containsKey("haSensorPostfixZone" + String(i)))
            zones[i].haSensorPostfix =
                doc["haSensorPostfixZone" + String(i)].as<String>();
          if (doc.containsKey("ds18b20PostfixZone" + String(i)))
            zones[i].ds18b20Postfix =
                doc["ds18b20PostfixZone" + String(i)].as<String>();
          if (doc.containsKey("woprUpdateIntervalZone" + String(i)))
            woprZones[i].updateInterval =
                doc["woprUpdateIntervalZone" + String(i)];
          if (doc.containsKey("scrollInfiniteZone" + String(i)))
            zones[i].scrollInfinite = doc["scrollInfiniteZone" + String(i)];
          // Countdown
          if (doc.containsKey("countdownFormatZone" + String(i)))
            zones[i].countdownFormat =
                doc["countdownFormatZone" + String(i)].as<String>();
          if (doc.containsKey("countdownFinishZone" + String(i)))
            zones[i].countdownFinish =
                doc["countdownFinishZone" + String(i)].as<String>();
          if (doc.containsKey("countdownShowUnitsZone" + String(i)))
            zones[i].countdownShowUnits =
                doc["countdownShowUnitsZone" + String(i)];
          if (doc.containsKey("countdownPrefixZone" + String(i)))
            zones[i].countdownPrefix =
                doc["countdownPrefixZone" + String(i)].as<String>();
          if (doc.containsKey("countdownSuffixZone" + String(i)))
            zones[i].countdownSuffix =
                doc["countdownSuffixZone" + String(i)].as<String>();
          if (doc.containsKey("countdownTargetZone" + String(i))) {
            String target = doc["countdownTargetZone" + String(i)].as<String>();
            if (target.length() > 0)
              parseCountdownTarget(target, i, timeClient.getEpochTime(),
                                   ntpTimeZone);
          }

          saveVarsToConfFile("zoneSettings", i);
        }

        saveVarsToConfFile("displaySettings", 0);

        request->send(200, "text/plain", "OK");
        shouldReboot = true;
      },
      [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final) {
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

  // ─── Settings JSON API ────────────────────────────────────────────────────
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument doc(4096);

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
    doc["zoneNumbers"] = zoneNumbers;
    doc["intensity"] = intensity + 1;

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
    Serial.print("API Settings: mqttPassword sentinel sent: ");
    Serial.println((mqttPassword == "") ? "EMPTY" : "MASKED");
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

    // Zones
    for (int i = 0; i < 4; i++) {
      String z = "zone" + String(i);
      doc[z + "Begin"] = zones[i].begin;
      doc[z + "End"] = zones[i].end;
      doc["workModeZone" + String(i)] = zones[i].workMode;
      doc["scrollSpeedZone" + String(i)] = zones[i].scrollSpeed;
      doc["scrollPauseZone" + String(i)] = zones[i].scrollPause;
      doc["scrollAlignZone" + String(i)] = zones[i].scrollAlign;
      doc["scrollEffectZone" + String(i) + "In"] = zones[i].scrollEffectIn;
      doc["scrollEffectZone" + String(i) + "Out"] = zones[i].scrollEffectOut;
      doc["charspacingZone" + String(i)] = zones[i].charspacing;
      doc["fontZone" + String(i)] = zones[i].font;

      doc["mqttTextTopicZone" + String(i)] = MQTTZones[i].message;
      doc["mqttPostfixZone" + String(i)] = zones[i].mqttPostfix;

      doc["clockDisplayFormatZone" + String(i)] = zones[i].clockDisplayFormat;
      doc["owmWhatToDisplayZone" + String(i)] = zones[i].owmWhatToDisplay;

      doc["haSensorIdZone" + String(i)] = zones[i].haSensorId;
      doc["haSensorPostfixZone" + String(i)] = zones[i].haSensorPostfix;

      doc["ds18b20PostfixZone" + String(i)] = zones[i].ds18b20Postfix;
      doc["scrollInfiniteZone" + String(i)] = zones[i].scrollInfinite;
      // Countdown
      doc["countdownFormatZone" + String(i)] = zones[i].countdownFormat;
      doc["countdownFinishZone" + String(i)] = zones[i].countdownFinish;
      doc["countdownShowUnitsZone" + String(i)] = zones[i].countdownShowUnits;
      doc["countdownPrefixZone" + String(i)] = zones[i].countdownPrefix;
      doc["countdownSuffixZone" + String(i)] = zones[i].countdownSuffix;
      doc["countdownTargetZone" + String(i)] = countdownState[i].targetStr;
    }

    serializeJson(doc, *response);
    request->send(response);
  });

  // ─── OTA Firmware Upload ──────────────────────────────────────────────────
  server.on(
      "/api/ota", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // Get chunk info from query parameters
        int chunkNum = 0;
        int totalChunks = 1;

        if (request->hasParam("chunk")) {
          chunkNum = request->getParam("chunk")->value().toInt();
        }
        if (request->hasParam("total")) {
          totalChunks = request->getParam("total")->value().toInt();
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
        // This is called for each data fragment within a single POST request

        // Get chunk info from query parameters
        int chunkNum = 0;
        size_t totalSize = 0;

        if (request->hasParam("chunk")) {
          chunkNum = request->getParam("chunk")->value().toInt();
        }
        if (request->hasParam("size")) {
          totalSize = request->getParam("size")->value().toInt();
        }

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

        // Write data
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
        request->send(200, "text/plain", String(dsTemp).c_str());
      }
    } else {
      request->send(404, "text/plain", "Sensor disabled");
    }
  });

  // ─── Message API ──────────────────────────────────────────────────────────
  server.on("/api/message", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.print(F("API request received "));
    int params = request->params();
    Serial.print(params);
    Serial.println(F(" params sent in"));

    for (int i = 0; i < params; i++) {
      const AsyncWebParameter *p = request->getParam((size_t)i);
      if (p->isPost()) {
        Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

        if (strcmp(p->name().c_str(), "messageZone0") == 0)
          zoneNewMessage(0, p->value().c_str(), "", true, true);
        if (strcmp(p->name().c_str(), "messageZone1") == 0)
          zoneNewMessage(1, p->value().c_str(), "", true, true);
        if (strcmp(p->name().c_str(), "messageZone2") == 0)
          zoneNewMessage(2, p->value().c_str(), "", true, true);
        if (strcmp(p->name().c_str(), "messageZone3") == 0)
          zoneNewMessage(3, p->value().c_str(), "", true, true);
      }
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ─── Config API ───────────────────────────────────────────────────────────
  server.on("/api/config", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.print(F("\nAPI request received "));
    int params = request->params();
    Serial.print(params);
    Serial.println(F(" params sent in"));
    const AsyncWebParameter *key = request->getParam((size_t)0);
    uint8_t n = 99;
    bool finishRequest = false;
    bool showModeMessage = false;
    bool zoneUpdate[4] = {false, false, false, false};

    for (int i = 1; i < params; i++) {
      const AsyncWebParameter *p = request->getParam((size_t)i);
      if (p->isPost()) {
        Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

        if (key->value() == "systemSettings") {
          if (p->name() == "disableServiceMessages") {
            if (strcmp(p->value().c_str(), "true") == 0)
              disableServiceMessages = true;
            if (strcmp(p->value().c_str(), "false") == 0)
              disableServiceMessages = false;
          }
          if (p->name() == "deviceName")
            shortMACaddr = p->value().c_str();

          finishRequest = true;
        }

        if (key->value() == "displaySettings") {
          if (p->name() == "zoneNumbers")
            zoneNumbers = p->value().toInt();
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

        if (key->value() == "zoneSettings") {
          if (p->name() == "zone")
            n = p->value().toInt();
          if (p->name().indexOf("zone0") >= 0)
            n = 0;
          if (p->name().indexOf("zone1") >= 0)
            n = 1;
          if (p->name().indexOf("zone2") >= 0)
            n = 2;
          if (p->name().indexOf("zone3") >= 0)
            n = 3;

          if (n < 4) {
            if (p->name() == "workMode") {
              zones[n].workMode = p->value().c_str();
              if (zones[n].workMode == "owmWeather")
                owmLastTime = -1000000;
              if (zones[n].workMode == "haClient")
                haLastTime = -1000000;
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
                  zones[n].workMode != "wopr") {
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
            String pv = p->value();
            if (pv.equalsIgnoreCase("true") || pv.equalsIgnoreCase("on") ||
                pv == "1")
              zones[n].scrollInfinite = true;
            else
              zones[n].scrollInfinite = false;
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
            String pv = p->value();
            zones[n].countdownShowUnits =
                (pv.equalsIgnoreCase("true") || pv.equalsIgnoreCase("on") ||
                 pv == "1");
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

          finishRequest = true;
        }
      }
      if (key->value() == "mqttSettings") {
        if (p->name() == "mqttEnable") {
          if (strcmp(p->value().c_str(), "true") == 0)
            mqttEnable = true;
          if (strcmp(p->value().c_str(), "false") == 0)
            mqttEnable = false;
        }
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

      if (key->value() == "wallClockSett") {
        if (p->name() == "ntpTimeZone")
          ntpTimeZone = p->value().toFloat();
        if (p->name() == "disableDotsBlink") {
          if (strcmp(p->value().c_str(), "true") == 0)
            disableDotsBlink = true;
          if (strcmp(p->value().c_str(), "false") == 0)
            disableDotsBlink = false;
        }
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

      if (key->value() == "owmSettings") {
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
      if (key->value() == "haSettings") {
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

      if (key->value() == "ds18b20Settings") {
        if (p->name() == "ds18b20Enable") {
          if (strcmp(p->value().c_str(), "true") == 0)
            ds18b20Enable = true;
          if (strcmp(p->value().c_str(), "false") == 0)
            ds18b20Enable = false;
        }
        if (p->name() == "ds18b20UpdateInterval")
          ds18b20UpdateInterval = p->value().toInt();
        if (p->name() == "ds18b20UnitsFormat") {
          ds18b20UnitsFormat = p->value().c_str();
          previousDsMillis = -1000;
        }

        finishRequest = true;
      }

      if (key->value() == "intensity") {
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

    // Apply batched updates
    for (int z = 0; z < 4; z++) {
      if (zoneUpdate[z])
        zones[z].forceUpdate = true;
    }

    if (finishRequest) {
      if (showModeMessage && !disableServiceMessages) {
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
      }
      if (key->value() == "zoneSettings" && zones[n].workMode == "mqttClient") {
        if (mqttClient.connected() && MQTTZones[n].message != "" &&
            MQTTZones[n].message != " ") {
          mqttClient.subscribe(MQTTZones[n].message.c_str());
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      if (key->value() == "zoneSettings") {
        configDirty = true;
        configDirtyTime = millis();
      } else {
        saveVarsToConfFile(key->value(), n);
      }
      shouldMqttPublish = true;
      // Reinforce power state after zone settings to prevent display
      // turning off
      if (key->value() == "zoneSettings" && power) {
        P.displayShutdown(0);
      }
      if (key->value() == "displaySettings") {
        restartESP = true;
      }
    }
  });

  // ─── Factory Reset ────────────────────────────────────────────────────────
  server.on("/api/factory-reset", HTTP_POST,
            [](AsyncWebServerRequest *request) {
              Serial.println(F("Factory reset requested!"));

              // Clear all preferences namespaces
              preferences.begin("systemSettings", false);
              preferences.clear();
              preferences.end();

              preferences.begin("displaySettings", false);
              preferences.clear();
              preferences.end();

              preferences.begin("mqttSettings", false);
              preferences.clear();
              preferences.end();

              preferences.begin("wallClockSett", false);
              preferences.clear();
              preferences.end();

              preferences.begin("owmSettings", false);
              preferences.clear();
              preferences.end();

              preferences.begin("haSettings", false);
              preferences.clear();
              preferences.end();

              preferences.begin("ds18b20Settings", false);
              preferences.clear();
              preferences.end();

              preferences.begin("intensity", false);
              preferences.clear();
              preferences.end();

              for (uint8_t n = 0; n < 4; n++) {
                preferences.begin("zoneSettings", false);
                preferences.clear();
                preferences.end();
              }

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
