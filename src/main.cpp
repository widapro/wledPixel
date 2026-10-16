// ─── wledPixel  –  main.cpp ────────────────────────────────────────────────
// After refactoring, this file only contains setup() and loop().
// All helpers, data, and route definitions live in their own modules.
// ────────────────────────────────────────────────────────────────────────────

#include "ConfigStore.h"
#include "DisplayUtils.h"
#include "HilTest.h"
#include "MqttHandler.h"
#include "Scheduler.h"
#include "WebPriority.h"
#include "WebRoutes.h"

// ─── WiFi helpers (tiny, stays here) ────────────────────────────────────────
static bool dsConversionPending = false;
static unsigned long dsConversionStarted = 0;

void saveConfigCallback() { shouldSaveConfig = true; }

// ════════════════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════════════════
void setup() {
  readAllConfig();
  Serial.begin(115200);
  Serial.print(F("Start serial...."));
  Serial.printf("\nCPU frequency: %u MHz\n", ESP.getCpuFreqMHz());
#if defined(ESP8266)
  Serial.printf("Reset reason: %s\n", ESP.getResetReason().c_str());
#endif

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

#if defined(ESP8266)
  // AsyncWiFiManager is only needed during autoConnect/config portal.
  // Keeping it after normal boot costs heap that the web UI needs later.
  if (wifiManagerPtr) {
    delete wifiManagerPtr;
    wifiManagerPtr = nullptr;
  }
#endif

  // Start MQTT client
  if (mqttEnable) {
    mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
    mqttClient.setCallback(MQTTCallback);
#if defined(ESP8266)
    mqttClient.setBufferSize(1024);
#else
    mqttClient.setBufferSize(1536);
#endif
  }

  // Pre-fetch OWM weather data during boot so it's ready immediately
  if (power && owmApiToken.length() > 0 && owmCity.length() > 0) {
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "owmWeather") {
        Serial.println(F("[OWM] Pre-fetching weather data during boot..."));
        owmWeatherUpdate(owmCity, owmUnitsFormat, owmApiToken);
        owmLastTime = millis(); // prevent immediate re-fetch in loop()
        break; // one zone triggers the fetch; all share the data
      }
    }
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
  sensors.setWaitForConversion(false);

  // Re-initialize all zones properly with loaded config
  P.begin(zoneNumbers);
  for (uint8_t n = 0; n < zoneNumbers; n++) {
    P.setZone(n, zones[n].begin, zones[n].end);
    P.setIntensity(n, intensity);
  }
  Serial.printf("[OTA] Platform marker: %s\n", PLATFORM_MARKER);
  Serial.println("Setup complete, entering loop");
}

// ════════════════════════════════════════════════════════════════════════════
//  LOOP
// ════════════════════════════════════════════════════════════════════════════
void loop() {
  currentMillis = millis();

  if (shouldReboot && !webPriorityHasActiveRequests() && !webPriorityIsActive(currentMillis) &&
      !schedulerHasPendingProgressBarSave() &&
      !hasDeferredConfigSaves() && !schedulerHasPendingProfileSave() &&
      !schedulerHasPendingProfileApply()) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

  // If OTA update is in progress, block main loop execution
  if (globalOtaInProgress) {
    yield();
    return;
  }

  serviceDisplayOrientation();

  // During page load the ESP8266 should give the async web server the heap and
  // CPU budget first. Reboot requests still win; routine background work waits.
  if (restartESP && !webPriorityHasActiveRequests() && !webPriorityIsActive(currentMillis) &&
      !hasDeferredConfigSaves() && !schedulerHasPendingProfileSave() &&
      !schedulerHasPendingProgressBarSave() &&
      !schedulerHasPendingProfileApply()) {
    Serial.print(F("Rebooting ESP..."));
    delay(100);
    ESP.restart();
  }

#if defined(ESP8266)
  if (webPriorityIsActive(currentMillis)) {
    serviceWebPriorityMode(currentMillis);
    yield();
    return;
  }
#endif

  // A queued write must not reuse request workspaces before a response drains.
  if (webPriorityHasActiveRequests()) {
    yield();
    return;
  }

  // Persist while MQTT still has its small web-priority buffer. Restoring the
  // 1KB MQTT buffer before a LittleFS write can leave ESP8266 without a large
  // enough contiguous heap block.
  if (schedulerServicePendingProfileSave() ||
      schedulerServicePendingProgressBarSave() ||
      schedulerServicePendingProfileApply() || serviceDeferredConfigSave() ||
      webServicePendingRestore()) {
    yield();
    return;
  }

#if defined(ESP8266)
  // No web work or pending flash writes remain; background networking may now
  // reclaim its normal buffers.
  serviceWebPriorityMode(currentMillis);
#endif

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
      if (zones[n].workMode == "stockTicker")
        stockLastTime = 0; // Force immediate stock update
    }
  }

#if defined(ESP8266)
  // A web request can arrive from the async TCP callback after the top-of-loop
  // priority check. Stop before boot/config work if that happened mid-loop.
  if (webPriorityIsActive(millis())) {
    yield();
    return;
  }
#endif

  if (shouldUpdateNtp) {
    Serial.println("Deferred NTP update...");
    ntpUpdateTime();
    shouldUpdateNtp = false;
  }

  // Handle OTA updates
  ArduinoOTA.handle();

  // HIL display commands are queued by async HTTP callbacks and applied here,
  // on the same execution path as normal Parola rendering.
  hilTestService(currentMillis);
  if (hilTestDisplayHoldActive(currentMillis)) {
    yield();
    return;
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

#if defined(ESP8266)
  if (webPriorityIsActive(millis())) {
    yield();
    return;
  }
#endif

  // ── Apply saved config for display ────────────────────────────────────────
  if (initConfig) {
    SchedulerWorkGuard work;
    Serial.print(F("Init configuration"));
    initConfig = false;
    const bool preserveVisibleFrame = displayWakePending && power;
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

      if (!preserveVisibleFrame && zones[n].workMode == "mqttClient" &&
          !disableServiceMessages)
        zoneShowModeMessage(n, "MQTT");
      if (!preserveVisibleFrame && zones[n].workMode == "manualInput" &&
          !disableServiceMessages)
        zoneShowModeMessage(n, "Manual");
      if (zones[n].workMode == "owmWeather") {
        if (!preserveVisibleFrame && !disableServiceMessages)
          zoneShowModeMessage(n, "OWM");
        if (!preserveVisibleFrame)
          zoneNewMessage(n, "Loading...", "");
        zones[n].previousMillis = -1000000;
        if (zones[n].owmWhatToDisplay == "owmWeatherIcon") {
          P.setFont(n, wledSymbolFont);
          P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                          PA_NO_EFFECT);
        }
      }
      if (zones[n].workMode == "haClient") {
        if (!preserveVisibleFrame && !disableServiceMessages)
          zoneShowModeMessage(n, "HA");
        if (!preserveVisibleFrame)
          zoneNewMessage(n, "Loading...", "");
        zones[n].previousMillis = -1000000;
      }

      if (zones[n].workMode == "intTempSensor") {
        if (!preserveVisibleFrame && !disableServiceMessages)
          zoneShowModeMessage(n, "TempS");
        if (!preserveVisibleFrame)
          zoneNewMessage(n, "Loading...", "");
        previousDsMillis = -1000000;
      }

      if (zones[n].workMode == "stockTicker") {
        if (!preserveVisibleFrame && !disableServiceMessages)
          zoneShowModeMessage(n, "Stock");
        if (!preserveVisibleFrame)
          zoneNewMessage(n, "Loading...", "");
        stockLastTime = 0;
        stockParseSymbols(n, zones[n].stockSymbols);
      }

      // Clear boot messages to prevent them from looping in MQTT mode
      if (!preserveVisibleFrame && zones[n].workMode == "mqttClient" &&
          disableServiceMessages) {
        strcpy(zoneMessages[n], "");
        strcpy(zoneMessagesPending[n], "");
        P.displayClear(n);
      }
      // Only call displayReset if we didn't just set a mode message
      if (!preserveVisibleFrame && zones[n].workMode != "mqttClient" &&
          zones[n].workMode != "manualInput") {
        P.displayReset(n);
      }
    }

    // During a scheduled wake the already visible cached frame was preserved
    // while the new settings were installed. Normal sensor updates can now
    // replace it without an intermediate clear/reset.
    if (displayWakePending) {
      if (power)
        Serial.print(F("\n[SLEEP] Display wake configuration committed"));
      displayWakePending = false;
    }
  }

#if defined(ESP8266)
  if (webPriorityIsActive(millis())) {
    yield();
    return;
  }
#endif

  // ── Scheduler: check profiles every ~60s ──────────────────────────────────
  schedulerCheck();

  // ── Stock ticker statics (declared early so config-save can reset them) ──
  static uint8_t stockCurrentZone = 0;
  static bool stockFetchingRound = false;
  static unsigned long stockLastStaggerTime = 0;

  // ── Handle deferred config save ───────────────────────────────────────────
  // These cooperative variables protect the heap from fragmentation by spacing
  // out heavy file operations and blocking network requests by at least 2000ms.
  bool heavyTaskExecutedThisLoop = false;
  static unsigned long lastHeavyTaskTime = 0;

  if (configDirty && (millis() - configDirtyTime > 2000)) {
    if (!heavyTaskExecutedThisLoop &&
        currentMillis - lastHeavyTaskTime > 2000) {
      heavyTaskExecutedThisLoop = true;
      SchedulerWorkGuard work;
      Serial.println(F("Saving deferred configuration..."));
      for (uint8_t n = 0; n < zoneNumbers; n++) {
        saveVarsToConfFile("zoneSettings", n);
      }
      configDirty = false;
      lastHeavyTaskTime = millis();
#if defined(ESP8266)
      // Config save fragments heap via Preferences String temporaries.
      // Abort any in-progress stock fetch round and reset timer so the
      // next TLS allocation doesn't crash into fragmented memory.
      stockFetchingRound = false;
      stockLastTime = millis();
      Serial.printf("[Config] Heap after save: %u, MaxBlock: %u\n",
                    ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
#endif
    }
  }

  // ── Main logic (after boot tests finish) ──────────────────────────────────
  if (allTestsFinish) {

    // MQTT reconnect / loop
    if (mqttEnable && mqttServerAddress.length() > 0) {
      if (!mqttClient.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
          if (!heavyTaskExecutedThisLoop &&
              currentMillis - lastHeavyTaskTime > 2000) {
            heavyTaskExecutedThisLoop = true;
            lastReconnectAttempt = now;
            if (reconnect()) {
              lastReconnectAttempt = 0;
            }
            lastHeavyTaskTime = millis();
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

    // ── Display-off sleep mode ─────────────────────────────────────────────
    // Keep only the control plane alive while the display is off: web/OTA,
    // MQTT (so Home Assistant can wake the device), NTP and the scheduler.
    // Sensor reads, external data requests and display animation resume only
    // after power is turned back on.
    static bool previousDisplayPower = power;

    // A scheduled wake is still being configured. Keep the control plane
    // responsive, but do not animate the old frame or start sensor requests.
    if (displayWakePending) {
      yield();
      return;
    }

    if (power != previousDisplayPower) {
      if (power) {
        Serial.print(F("\n[SLEEP] Display ON; refreshing data sources"));

        // Make every display data source eligible on the first safe loop.
        previousDsMillis =
            currentMillis -
            (static_cast<unsigned long>(ds18b20UpdateInterval) * 1000UL) - 1UL;
        owmLastTime =
            currentMillis -
            (static_cast<unsigned long>(owmUpdateInterval) * 1000UL) - 1UL;
        stockLastTime = 0;
        stockFetchingRound = false;

        for (uint8_t n = 0; n < zoneNumbers; n++) {
          zones[n].forceUpdate = true;
          if (zones[n].workMode == "haClient") {
            zones[n].previousMillis =
                currentMillis -
                (static_cast<unsigned long>(haUpdateInterval) * 1000UL) - 1UL;
          } else if (zones[n].workMode == "owmWeather") {
            zones[n].previousMillis =
                currentMillis -
                (static_cast<unsigned long>(owmUpdateInterval) * 1000UL) - 1UL;
          }

          if (n < 4) {
            progressBars[n].lastDataUpdate =
                currentMillis -
                (static_cast<unsigned long>(haUpdateInterval) * 1000UL) - 1UL;
            progressBars[n].lastCondUpdate =
                progressBars[n].lastDataUpdate;
            progressBars[n].lastDynSrcUpdate =
                progressBars[n].lastDataUpdate;
            progressBars[n].dynLastStep = currentMillis;
          }
        }
      } else {
        Serial.print(F("\n[SLEEP] Display OFF; sensor polling suspended"));
        stockFetchingRound = false;
        dsTempToDisplay = false;
      }
      previousDisplayPower = power;
    }

    if (!power) {
      yield();
      return;
    }

    // DS18B20 measurement
    if (ds18b20Enable) {
      if (!dsConversionPending && !heavyTaskExecutedThisLoop &&
          currentMillis - lastHeavyTaskTime > 2000 &&
          currentMillis - previousDsMillis >=
              (unsigned)ds18b20UpdateInterval * 1000) {
        heavyTaskExecutedThisLoop = true;
        previousDsMillis = currentMillis;
        sensors.requestTemperatures();
        dsConversionStarted = millis();
        dsConversionPending = true;
        lastHeavyTaskTime = millis();
      }
      // Maximum 12-bit conversion time; keep animation and networking running.
      if (dsConversionPending && millis() - dsConversionStarted >= 750UL) {
        dsConversionPending = false;
        heavyTaskExecutedThisLoop = true;
        float dsTempC = sensors.getTempCByIndex(0);
        if (dsTempC == DEVICE_DISCONNECTED_C) {
          Serial.print(F("\nERROR: ds18b20 getting temperature failed"));
        } else {
          dsTempToDisplay = true;
          if (ds18b20UnitsFormat == "Fahrenheit") {
            float dsTempF = DallasTemperature::toFahrenheit(dsTempC);
            dtostrf(dsTempF, 4, 1, dsTempBuff);
            dsTemp = dsTempBuff;
            Serial.printf("\nds18b20 measurement temperature: %s F",
                          dsTemp.c_str());
          } else {
            dtostrf(dsTempC, 4, 1, dsTempBuff);
            dsTemp = dsTempBuff;
            Serial.printf("\nds18b20 measurement temperature: %s C",
                          dsTemp.c_str());
          }
          if (mqttEnable) {
            static char dsTopic[80] = {0};
            if (dsTopic[0] == '\0') {
              snprintf(dsTopic, sizeof(dsTopic), "%s/temperature",
                       MQTTGlobalPrefix.c_str());
            }
            mqttClient.publish(dsTopic, dsTemp.c_str());
          }
        }
        lastHeavyTaskTime = millis();
      }
    }

    // OWM update
    owmWeatherEnable = false;
    bool hasFreshDsReading = dsTempToDisplay;
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "owmWeather" && owmApiToken.length() > 0 &&
          owmCity.length() > 0)
        owmWeatherEnable = true;
    }
    if (owmWeatherEnable && !heavyTaskExecutedThisLoop &&
        currentMillis - lastHeavyTaskTime > 2000 &&
        currentMillis - owmLastTime >=
            (unsigned long)owmUpdateInterval * 1000) {
      heavyTaskExecutedThisLoop = true;
      owmWeatherUpdate(owmCity, owmUnitsFormat, owmApiToken);
      owmLastTime = currentMillis;
      lastHeavyTaskTime = millis();
    }

    // Stock Ticker update — fetch ONE symbol per tick (non-blocking)
    // Each tick fetches a single symbol from one zone, cycling through zones
    // and symbols. The full update cycle is stockUpdateInterval (all symbols
    // refreshed), but individual fetches are staggered every 2s.

    bool stockTickerActive = false;
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "stockTicker" && stockApiToken.length() > 0 &&
          zones[n].stockSymbols.length() > 0) {
        stockTickerActive = true;
        break;
      }
    }

    if (stockLastTime == 0) {
      stockFetchingRound = false;
      stockLastTime =
          currentMillis - ((unsigned long)stockUpdateInterval * 1000) - 1;
    }

    if (stockTickerActive) {
      if (!stockFetchingRound &&
          currentMillis - stockLastTime >=
              (unsigned long)stockUpdateInterval * 1000) {
        stockFetchingRound = true;
        stockCurrentZone = 0;
      }

      if (stockFetchingRound && !heavyTaskExecutedThisLoop &&
          currentMillis - lastHeavyTaskTime > 2000 &&
          currentMillis - stockLastStaggerTime >= 2000UL) {
        heavyTaskExecutedThisLoop = true;
        bool fetched = false;
        while (stockCurrentZone < zoneNumbers) {
          if (zones[stockCurrentZone].workMode == "stockTicker" &&
              zones[stockCurrentZone].stockSymbols.length() > 0) {

            // Prevent firing again if task is already running
            if (!stockTaskInProgress) {
              stockUpdateOneSymbolAsync(stockCurrentZone, stockApiToken);
              fetched = true;

              // Move to next symbol/zone logic only when done
              StockZoneData &zd = stockZones[stockCurrentZone];
              // nextFetchIndex advances internally, if we wrapped around
              // we move to the next zone
              if (zd.nextFetchIndex == 0 && zd.symbolCount > 0 &&
                  zd.quotes[0].valid) {
                // In Async mode nextFetchIndex will be updated inside the async
                // thread. So we don't increment stockCurrentZone here directly
                // until zd.nextFetchIndex == 0 after at least one fetch. We can
                // safely just trust zd.nextFetchIndex wrapper
                stockCurrentZone++;
              }
            } else {
              fetched = true; // task is busy, so don't advance the staggered
                              // time, just wait
            }
            break; // Only ONE symbol per tick
          } else {
            stockCurrentZone++;
          }
        }

        if (!fetched || stockCurrentZone >= zoneNumbers) {
          stockFetchingRound = false;
          stockLastTime = currentMillis;
        }
        stockLastStaggerTime = currentMillis;
        lastHeavyTaskTime = millis();
      }
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
            zones[n].clockDisplayFormat == "HHMM12" ||
            zones[n].clockDisplayFormat == "HHMM12AP" ||
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
        if (!heavyTaskExecutedThisLoop &&
            currentMillis - lastHeavyTaskTime > 2000 &&
            currentMillis - zones[n].previousMillis >=
                (unsigned)haUpdateInterval * 1000) {
          heavyTaskExecutedThisLoop = true;
          zoneNewMessage(
              n, haApiGet(zones[n].haSensorId, zones[n].haSensorPostfix), "");
          zones[n].previousMillis = currentMillis;
          lastHeavyTaskTime = millis();
        }
      }

      // DS18B20
      if (zones[n].workMode == "intTempSensor" && hasFreshDsReading) {
        zoneNewMessage(n, dsTemp, zones[n].ds18b20Postfix);
      }

      // Stock Ticker
      if (zones[n].workMode == "stockTicker") {
        String stockDisplay = stockGetDisplayString(
            n, zones[n].stockShowArrows, zones[n].stockDisplayFormat,
            zones[n].stockPrefix, zones[n].stockPostfix);
        if (stockDisplay != zones[n].curTime) {
          zones[n].curTime = stockDisplay;
          zoneNewMessage(n, stockDisplay, "", true, true);
        }
      }
    }
    if (hasFreshDsReading) {
      dsTempToDisplay = false;
    }
  }

  displayAnimation();

  // ── Progress Bar HA polling + rendering ───────────────────────────────────
  if (allTestsFinish) {
    for (uint8_t i = 0; i < zoneNumbers && i < 4; i++) {
      if (!progressBars[i].enabled || woprZones[i].active)
        continue;

      // ── HA data source polling (Regular mode only) ──
      if (!progressBars[i].dynamicMode &&
          progressBars[i].dataSourceType == "ha" &&
          progressBars[i].dataSourceId.length() > 0 && haAddr.length() > 0 &&
          haApiToken.length() > 0) {
        if (!heavyTaskExecutedThisLoop &&
            currentMillis - lastHeavyTaskTime > 2000 &&
            currentMillis - progressBars[i].lastDataUpdate >=
                (unsigned long)haUpdateInterval * 1000) {
          heavyTaskExecutedThisLoop = true;
          String state = haApiGetState(progressBars[i].dataSourceId);
          if (state.length() > 0) {
            progressBars[i].currentValue = state.toFloat();
            Serial.printf("\n[PB] Zone %u HA data: '%s' -> %.2f", i,
                          state.c_str(), progressBars[i].currentValue);
          }
          progressBars[i].lastDataUpdate = currentMillis;
          lastHeavyTaskTime = millis();
        }
      }

      // ── HA condition source polling (Regular mode only) ──
      if (!progressBars[i].dynamicMode &&
          progressBars[i].conditionEnabled &&
          progressBars[i].conditionSourceType == "ha" &&
          progressBars[i].conditionSourceId.length() > 0 &&
          haAddr.length() > 0 && haApiToken.length() > 0) {
        if (!heavyTaskExecutedThisLoop &&
            currentMillis - lastHeavyTaskTime > 2000 &&
            currentMillis - progressBars[i].lastCondUpdate >=
                (unsigned long)haUpdateInterval * 1000) {
          heavyTaskExecutedThisLoop = true;
          String state = haApiGetState(progressBars[i].conditionSourceId);
          if (state.length() > 0) {
            progressBars[i].conditionMet =
                (state == progressBars[i].conditionValue);
            progressBars[i].lastCondReceivedValue = state;
            Serial.printf("\n[PB] Zone %u HA cond: '%s' == '%s' -> %s", i,
                          state.c_str(),
                          progressBars[i].conditionValue.c_str(),
                          progressBars[i].conditionMet ? "true" : "false");
          }
          progressBars[i].lastCondUpdate = currentMillis;
          lastHeavyTaskTime = millis();
        }
      }

      // ── HA dynamic source polling (Dynamic mode only) ──
      if (progressBars[i].dynamicMode &&
          progressBars[i].dynSourceType == "ha" &&
          progressBars[i].dynSourceId.length() > 0 &&
          haAddr.length() > 0 && haApiToken.length() > 0) {
        if (!heavyTaskExecutedThisLoop &&
            currentMillis - lastHeavyTaskTime > 2000 &&
            currentMillis - progressBars[i].lastDynSrcUpdate >=
                (unsigned long)haUpdateInterval * 1000) {
          heavyTaskExecutedThisLoop = true;
          String state = haApiGetState(progressBars[i].dynSourceId);
          if (state.length() > 0) {
            progressBars[i].dynCurrentSensorValue = state;
            Serial.printf("\n[PB] Zone %u HA dyn src: '%s'", i,
                          state.c_str());
          }
          progressBars[i].lastDynSrcUpdate = currentMillis;
          lastHeavyTaskTime = millis();
        }
      }
    }
  }
}
