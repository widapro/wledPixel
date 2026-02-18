// ─── wledPixel  –  main.cpp ────────────────────────────────────────────────
// After refactoring, this file only contains setup() and loop().
// All helpers, data, and route definitions live in their own modules.
// ────────────────────────────────────────────────────────────────────────────

#include "ConfigStore.h"
#include "DisplayUtils.h"
#include "MqttHandler.h"
#include "WebRoutes.h"

// ─── WiFi helpers (tiny, stays here) ────────────────────────────────────────
void saveConfigCallback() { shouldSaveConfig = true; }

// ════════════════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════════════════
void setup() {
  readAllConfig();
  Serial.begin(115200);
  Serial.print(F("Start serial...."));

  readAllConfig();

  initDisplay();

  // Show boot message
  P.begin(1); // Safe mode for boot messages
  P.setZone(0, zones[0].begin, zones[0].end);
  P.displayClear();
  P.setTextAlignment(0, PA_CENTER);
  P.setIntensity(7);
  P.setSpeed(0, 100);
  P.print("Boot");
  P.displayAnimate();

#if defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif

  P.print("WiFi");
  P.displayAnimate();

  wifiManagerPtr = new AsyncWiFiManager(&server, &dns);

  wifiManagerPtr->setSaveConfigCallback(saveConfigCallback);
  wifiManagerPtr->setAPCallback(wifiApWelcomeMessage);
  wifiManagerPtr->setConnectTimeout(15);

  bool wifiRes =
      wifiManagerPtr->autoConnect(MQTTGlobalPrefix.c_str(), wifiAPpassword);

  if (!wifiRes) {
    Serial.println("Wifi failed to connect");
  } else {
    Serial.print(F("Wifi connected to SSID: "));
    Serial.println(WiFi.SSID().c_str());
    Serial.print(F("Local ip: "));
    Serial.println(WiFi.localIP().toString().c_str());
    Serial.print(F("Gateway: "));
    Serial.println(WiFi.gatewayIP().toString().c_str());
    Serial.print(F("Subnet: "));
    Serial.println(WiFi.subnetMask().toString().c_str());
    Serial.print(F("DNS: "));
    Serial.println(WiFi.dnsIP().toString().c_str());
    Serial.print(F("HostName: "));
    Serial.println(MQTTGlobalPrefix.c_str());
    Serial.print(F("MQTT Device Prefix: "));
    Serial.println(MQTTGlobalPrefix.c_str());

    // Initialize mDNS for easy device discovery
    if (MDNS.begin(MQTTGlobalPrefix.c_str())) {
      Serial.print(F("mDNS responder started: "));
      Serial.print(MQTTGlobalPrefix);
      Serial.println(F(".local"));
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println(F("Error setting up mDNS responder"));
    }
  }

  // Start MQTT client
  if (mqttEnable) {
    mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
    mqttClient.setCallback(MQTTCallback);
    mqttClient.setBufferSize(1536);
  }

  // Start NTP client
  timeClient.begin();
  timeClient.setTimeOffset(ntpTimeZone * 3600);
  timeClient.setUpdateInterval(ntpUpdateInterval * 3600);
  timeClient.setPoolServerName(ntpServer.c_str());
  ntpUpdateTime();

  // Initialize countdown module
  countdownInit();

  // Register all web-server routes
  setupWebRoutes();

  // Start webserver
  server.begin();

  // Start ArduinoOTA
  ArduinoOTA.setHostname(MQTTGlobalPrefix.c_str());
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("OTA Start updating " + type);
    P.displayClear();
    P.write("OTA...");
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA End"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Start ds18b20
  sensors.begin();

  // Re-initialize all zones properly with loaded config
  P.begin(zoneNumbers);
  for (uint8_t n = 0; n < zoneNumbers; n++) {
    P.setZone(n, zones[n].begin, zones[n].end);
    P.setIntensity(n, intensity);
  }
  Serial.println("Setup complete, entering loop");
}

// ════════════════════════════════════════════════════════════════════════════
//  LOOP
// ════════════════════════════════════════════════════════════════════════════
void loop() {
  // ── Font hot-swap ─────────────────────────────────────────────────────────
  for (uint8_t n = 0; n < zoneNumbers; n++) {
    if (zones[n].updateFont) {
      P.displayClear(n);
      // Override: Force symbol font for OWM Icon mode
      if (zones[n].workMode == "owmWeather" &&
          zones[n].owmWhatToDisplay == "owmWeatherIcon") {
        P.setFont(n, wledSymbolFont);
      } else {
        applyZoneFont(n, zones[n].font);
      }
      P.displayReset(n);
      zones[n].updateFont = false;

      // Force refresh for modes
      if (zones[n].workMode == "wallClock")
        zones[n].curTime = "";
      if (zones[n].workMode == "owmWeather")
        zones[n].previousMillis = -1000000;
      if (zones[n].workMode == "haClient")
        zones[n].previousMillis = -1000000;
      if (zones[n].workMode == "intTempSensor")
        previousDsMillis = -1000000;
    }
  }

  if (shouldReboot) {
    Serial.println("Rebooting...");
    delay(2000);
    ESP.restart();
  }

  if (shouldUpdateNtp) {
    Serial.println("Deferred NTP update...");
    ntpUpdateTime();
    shouldUpdateNtp = false;
  }

  // If OTA update is in progress, block main loop execution
  if (globalOtaInProgress) {
    yield();
    return;
  }

  // Handle OTA updates
  ArduinoOTA.handle();

  currentMillis = millis();

  // Handle a restart ESP request
  if (restartESP) {
    Serial.print(F("Rebooting ESP..."));
    delay(1000);
    ESP.restart();
  }

  // ── Init display animation (boot sequence) ───────────────────────────────
  if (!allTestsFinish) {
    if (nLoop > zoneNumbers && P.getZoneStatus(0)) {
      delay(5000);
      allTestsFinish = true;
      initConfig = true;
    }
    if (nLoop == 0) {
      testZones(nLoop);
      nLoop++;
    } else if (nLoop <= zoneNumbers && P.getZoneStatus(nLoop - 1)) {
      testZones(nLoop);
      nLoop++;
    }
  }

  // ── Apply saved config for display ────────────────────────────────────────
  if (initConfig) {
    Serial.print(F("Init configuration"));
    initConfig = false;
    readAllConfig();

    if (mqttEnable)
      mqttClient.disconnect();
    P.setIntensity(intensity);
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      P.setSpeed(n, zones[n].scrollSpeed);
      P.setPause(n, zones[n].scrollPause * 1000);
      P.setTextAlignment(n, stringToTextPositionT(zones[n].scrollAlign));
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                      stringToTextEffectT(zones[n].scrollEffectOut));
      P.setCharSpacing(n, zones[n].charspacing);
      applyZoneFont(n, zones[n].font);

      if (zones[n].workMode == "wallClock") {
        zones[n].curTime = "";
        zones[n].previousMillis = -1000000;
      }
      if (zones[n].workMode == "wopr") {
        woprZones[n].active = true;
      }

      if (zones[n].workMode == "mqttClient" && !disableServiceMessages)
        zoneShowModeMessage(n, "MQTT");
      if (zones[n].workMode == "manualInput" && !disableServiceMessages)
        zoneShowModeMessage(n, "Manual");
      if (zones[n].workMode == "owmWeather") {
        if (!disableServiceMessages)
          zoneShowModeMessage(n, "OWM");
        zoneNewMessage(n, "Loading...", "");
        zones[n].previousMillis = -1000000;
        if (zones[n].owmWhatToDisplay == "owmWeatherIcon") {
          P.setFont(n, wledSymbolFont);
          P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                          PA_NO_EFFECT);
        }
      }
      if (zones[n].workMode == "haClient") {
        if (!disableServiceMessages)
          zoneShowModeMessage(n, "HA");
        zoneNewMessage(n, "Loading...", "");
        zones[n].previousMillis = -1000000;
      }

      if (zones[n].workMode == "intTempSensor") {
        if (!disableServiceMessages)
          zoneShowModeMessage(n, "TempS");
        zoneNewMessage(n, "Loading...", "");
        previousDsMillis = -1000000;
      }

      // Clear boot messages to prevent them from looping in MQTT mode
      if (zones[n].workMode == "mqttClient" && disableServiceMessages) {
        strcpy(zoneMessages[n], "");
        strcpy(zoneMessagesPending[n], "");
        P.displayClear(n);
      }
      // Only call displayReset if we didn't just set a mode message
      if (zones[n].workMode != "mqttClient" &&
          zones[n].workMode != "manualInput") {
        P.displayReset(n);
      }
    }
  }

  // ── Handle deferred config save ───────────────────────────────────────────
  if (configDirty && (millis() - configDirtyTime > 2000)) {
    Serial.println(F("Saving deferred configuration..."));
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      saveVarsToConfFile("zoneSettings", n);
    }
    configDirty = false;
  }

  // ── Main logic (after boot tests finish) ──────────────────────────────────
  if (allTestsFinish) {
    // MQTT reconnect / loop
    if (mqttEnable && mqttServerAddress.length() > 0) {
      if (!mqttClient.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
          lastReconnectAttempt = now;
          if (reconnect()) {
            lastReconnectAttempt = 0;
          }
        }
      } else {
        mqttClient.loop();
        if (!mqttPublished)
          MQTTPublishState();
        if (shouldMqttPublish) {
          MQTTPublishState();
          shouldMqttPublish = false;
        }
        if (shouldMqttDisconnect) {
          mqttClient.disconnect();
          mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
          lastReconnectAttempt = 0;
          shouldMqttDisconnect = false;
        }
      }
    }

    // DS18B20 measurement
    if (ds18b20Enable) {
      if (currentMillis - previousDsMillis >=
          (unsigned)ds18b20UpdateInterval * 1000) {
        previousDsMillis = currentMillis;
        sensors.requestTemperatures();
        float dsTempC = sensors.getTempCByIndex(0);
        if (dsTempC == DEVICE_DISCONNECTED_C) {
          Serial.print(F("\nERROR: ds18b20 getting temperature failed"));
        } else {
          dsTempToDisplay = true;
          if (ds18b20UnitsFormat == "Fahrenheit") {
            float dsTempF = DallasTemperature::toFahrenheit(dsTempC);
            dtostrf(dsTempF, 4, 1, dsTempBuff);
            dsTemp = String(dsTempBuff);
            Serial.printf("\nds18b20 measurement temperature: %s F",
                          dsTemp.c_str());
          } else {
            dtostrf(dsTempC, 4, 1, dsTempBuff);
            dsTemp = (char *)dsTempBuff;
            Serial.printf("\nds18b20 measurement temperature: %s C",
                          dsTemp.c_str());
          }
          if (mqttEnable)
            mqttClient.publish(
                String("wledPixel-" + shortMACaddr + "/temperature").c_str(),
                dsTemp.c_str());
        }
      }
    }

    // OWM update
    owmWeatherEnable = false;
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "owmWeather" && owmApiToken.length() > 0 &&
          owmCity.length() > 0)
        owmWeatherEnable = true;
    }
    if (owmWeatherEnable && currentMillis - owmLastTime >=
                                (unsigned long)owmUpdateInterval * 1000) {
      owmWeatherUpdate(owmCity, owmUnitsFormat, owmApiToken);
      owmLastTime = currentMillis;
    }

    // ── Zone routines ─────────────────────────────────────────────────────
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      // WOPR Mode
      if (zones[n].workMode == "wopr") {
        updateWOPREffect(n);
      }

      // Wall Clock
      if (zones[n].workMode == "wallClock") {
        String curTimeNew =
            getCurTime(zones[n].font, zones[n].clockDisplayFormat);
        if (zones[n].clockDisplayFormat == "HHMM" ||
            zones[n].clockDisplayFormat == "HHMMSS" ||
            zones[n].clockDisplayFormat == "ddmmaahhmm") {
          if (!disableDotsBlink &&
              (zones[n].textFits || !zones[n].scrollInfinite)) {
            if (currentMillis - zones[n].previousMillis >= 1000 ||
                zones[n].forceUpdate) {
              bool isSameMinuteForBlink = false;
              if (zones[n].clockDisplayFormat == "HHMMSS") {
                String curNoSec = zones[n].curTime.substring(0, 5);
                String newNoSec = curTimeNew.substring(0, 5);
                String blinkChar = (zones[n].font == "default") ? " " : "\xA0";
                curNoSec.replace(blinkChar, ":");
                newNoSec.replace(blinkChar, ":");
                isSameMinuteForBlink = (curNoSec == newNoSec);
              } else {
                isSameMinuteForBlink =
                    (zones[n].curTime == curTimeNew ||
                     zones[n].curTime ==
                         flashClockDots(curTimeNew, zones[n].font));
              }

              if (isSameMinuteForBlink && !zones[n].forceUpdate) {
                if (!P.getZoneStatus(n))
                  continue;

                zones[n].previousMillis = currentMillis;
                if (zones[n].curTime.indexOf(":") > 0) {
                  String blinkChar =
                      (zones[n].font == "default") ? " " : "\xA0";
                  curTimeNew.replace(":", blinkChar);
                }
                P.setPause(n, 100);
                P.setTextEffect(n, PA_PRINT, PA_NO_EFFECT);
                zones[n].curTime = curTimeNew;
                zoneNewMessage(n, zones[n].curTime, "", false);
              } else {
                zones[n].previousMillis = currentMillis;
                zones[n].curTime = curTimeNew;
                P.setPause(n, 100);
                textEffect_t inEffect;
                if (!zones[n].textFits && !zones[n].scrollInfinite) {
                  inEffect = PA_PRINT;
                } else if (zones[n].forceUpdate) {
                  inEffect = PA_PRINT;
                } else {
                  inEffect = stringToTextEffectT(zones[n].scrollEffectIn);
                }
                P.setTextEffect(n, inEffect, PA_NO_EFFECT);
                zones[n].forceUpdate = false;
                zoneNewMessage(n, zones[n].curTime, "", true, true);
              }
            }
          } else {
            if (zones[n].curTime != curTimeNew) {
              zones[n].curTime = curTimeNew;
              zoneNewMessage(n, zones[n].curTime, "");
            }
          }
        } else {
          if (zones[n].curTime != curTimeNew) {
            zones[n].curTime = curTimeNew;
            zoneNewMessage(n, zones[n].curTime, "");
          }
        }
      }

      // Countdown timer
      if (zones[n].workMode == "countdown") {
        if (countdownState[n].target > 0) {

          long currentEpoch = timeClient.getEpochTime();
          long utcEpoch = currentEpoch - (long)(ntpTimeZone * 3600);

          String countdownText = getCountdownText(
              n, zones[n].countdownFormat, zones[n].countdownShowUnits,
              zones[n].countdownPrefix, zones[n].countdownSuffix, utcEpoch,
              ntpTimeZone);

          bool showText =
              countdownHandleBlink(n, zones[n].countdownFinish, currentMillis);

          // ── Countdown reached zero ──
          if (countdownState[n].reached) {
            if (currentMillis - zones[n].previousMillis >= 500 ||
                zones[n].forceUpdate) {
              zones[n].previousMillis = currentMillis;
              if (showText) {
                if (zones[n].curTime != countdownText || zones[n].forceUpdate) {
                  zones[n].curTime = countdownText;
                  zones[n].forceUpdate = false;
                  zoneNewMessage(n, zones[n].curTime, "", true, true);
                }
              } else {
                P.setTextEffect(n, PA_PRINT, PA_NO_EFFECT);
                zones[n].curTime = "";
                zoneNewMessage(n, "", "", false, true);
              }
            }
          }
          // ── Path A: dot blink active + (text fits OR not-infinite scroll) ──
          // Mirrors clock's blink path: use PA_PRINT for every tick,
          // handle dot blink, never call restoreEffects (no displayReset).
          else if (!zones[n].countdownShowUnits && !disableDotsBlink &&
                   (zones[n].textFits || !zones[n].scrollInfinite)) {
            if (currentMillis - zones[n].previousMillis >= 1000 ||
                zones[n].forceUpdate) {

              // Apply dot blink toggle
              if (!countdownState[n].reached) {
                if (utcEpoch % 2 != 0) {
                  countdownText = flashClockDots(countdownText, zones[n].font);
                }
              }

              if (zones[n].forceUpdate) {
                // First-time setup: use configured effect (like clock's
                // "new minute" path)
                zones[n].previousMillis = currentMillis;
                zones[n].curTime = countdownText;
                P.setPause(n, 100);
                textEffect_t inEffect;
                if (!zones[n].textFits && !zones[n].scrollInfinite) {
                  inEffect = PA_PRINT;
                } else {
                  inEffect = PA_PRINT;
                }
                P.setTextEffect(n, inEffect, PA_NO_EFFECT);
                zones[n].forceUpdate = false;
                zoneNewMessage(n, zones[n].curTime, "", true, true);
              } else if (zones[n].curTime != countdownText) {
                // Regular tick: lightweight PA_PRINT (like clock's blink
                // path)
                if (!P.getZoneStatus(n))
                  continue; // wait for current display frame to finish

                zones[n].previousMillis = currentMillis;
                zones[n].curTime = countdownText;
                P.setPause(n, 100);
                P.setTextEffect(n, PA_PRINT, PA_NO_EFFECT);
                zoneNewMessage(n, zones[n].curTime, "", false);
              } else {
                zones[n].previousMillis = currentMillis;
              }
            }
          }
          // ── Path B: showUnits=true OR (text doesn't fit AND infinite
          // scroll) ──
          // Mirrors clock's else branch: bare zoneNewMessage with defaults
          // (restoreEffects=false, force=false). No timer — just compare.
          // The mid-scroll ticker swap in displayAnimation() handles it.
          // No dot blink here — dots always visible during infinite scroll
          // (same as clock mode).
          else {

            if (zones[n].forceUpdate) {
              zones[n].curTime = countdownText;
              zones[n].forceUpdate = false;
              zoneNewMessage(n, zones[n].curTime, "", true, true);
            } else if (zones[n].curTime != countdownText) {
              zones[n].curTime = countdownText;
              zoneNewMessage(n, zones[n].curTime, "");
            }
          }
        }
      }

      // OWM
      if (zones[n].workMode == "owmWeather") {
        if (owmWeatherEnable) {
          if (currentMillis - zones[n].previousMillis >=
              (unsigned long)owmUpdateInterval * 1000) {
            zoneNewMessage(n,
                           openWetherMapGetWeather(zones[n].owmWhatToDisplay,
                                                   owmUnitsFormat),
                           "");
            zones[n].previousMillis = currentMillis;
          }
        } else {
          zoneNewMessage(n, "err", "");
          Serial.print(F("\nERROR: Open Weather Map config error"));
        }
      }

      // HA client
      if (zones[n].workMode == "haClient") {
        if (currentMillis - zones[n].previousMillis >=
            (unsigned)haUpdateInterval * 1000) {
          zoneNewMessage(
              n, haApiGet(zones[n].haSensorId, zones[n].haSensorPostfix), "");
          zones[n].previousMillis = currentMillis;
        }
      }

      // DS18B20
      if (zones[n].workMode == "intTempSensor" && dsTempToDisplay == true) {
        zoneNewMessage(n, String(dsTemp), zones[n].ds18b20Postfix);
        dsTempToDisplay = false;
      }
    }
  }

  displayAnimation();
}