#pragma once

#include <Arduino.h>

#if defined(ESP8266)
#pragma message "Compiling for ESP8266 board"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h>
#else
#error "This ain't a ESP8266 or ESP32!"
#endif

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <NTPClient.h>
#include <OneWire.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#if defined(ESP32)
#include <Update.h>
#elif defined(ESP8266)
#include <Updater.h>
#ifndef UPDATE_SIZE_UNKNOWN
#define UPDATE_SIZE_UNKNOWN 0
#endif
#endif

#include "backupPage.h"
#include "base64_utils.h"
#include "countdown.h"
#include "indexPage.h"
#include "otaPage.h"
#include "owm.h"
#include "serviceSettingsPage.h"
#include "stockTicker.h"
#include "wledFont.h"
#include "zoneSettingsPage.h"

// ─── Pin Definitions
// ───────────────────────────────────────────────────────────
#if defined(ESP8266)
const int oneWireBus = D4;
#define DATA_PIN D7
#define CS_PIN D6
#define CLK_PIN D5
#elif defined(ESP32)
const int oneWireBus = 4;
#define DATA_PIN 23
#define CS_PIN 5
#define CLK_PIN 18
#else
#error "This ain't a ESP8266 or ESP32!"
#endif

// ─── Display Hardware
// ──────────────────────────────────────────────────────────
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// ─── Data Structures
// ───────────────────────────────────────────────────────────

typedef struct {
  uint8_t begin, end;
  uint8_t scrollSpeed, charspacing;
  uint8_t scrollPause;
  unsigned long previousMillis;
  String scrollAlign, scrollEffectIn, scrollEffectOut, font, workMode,
      clockDisplayFormat, haSensorId, haSensorPostfix, owmWhatToDisplay,
      mqttPostfix, ds18b20Postfix, curTime;
  // Countdown fields
  String countdownFormat, countdownFinish, countdownPrefix, countdownSuffix;
  bool countdownShowUnits;
  // Stock ticker fields
  String stockSymbols;
  String stockDisplayFormat; // namePrice, priceOnly, changePct
  String stockPrefix, stockPostfix;
  bool stockShowArrows;
  bool newMessageAvailable;
  bool scrollInfinite;
  bool restoreEffects;
  bool updateFont;
  bool forceUpdate;
  bool textFits;
} ZoneData;

typedef struct {
  String message, scrollEffectIn, scrollEffectOut, scrollSpeed, scrollPause,
      scrollAllign, charspacing, workMode, scrollInfinite, countdown,
      countdownFormat;
} MQTTZoneData;

typedef struct {
  bool active;
  unsigned long lastUpdate;
  uint16_t updateInterval;
  uint8_t intensity;
  uint8_t blinkPattern[8];
} WOPRData;

// ─── Extern Declarations
// ───────────────────────────────────────────────────────

// Core objects
extern Preferences preferences;
extern WiFiClient mqttEspClient;
extern OneWire oneWire;
extern DallasTemperature sensors;
extern uint8_t MAX_DEVICES;
extern MD_Parola P;
extern PubSubClient mqttClient;
extern AsyncWebServer server;
extern DNSServer dns;
extern AsyncWiFiManager *wifiManagerPtr;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

// Global flags
extern bool globalOtaInProgress;
extern bool otaPlatformMismatch;
extern bool otaPlatformOk;
extern uint8_t otaScanOverlap[32];
extern size_t otaScanOverlapLen;
extern bool shouldReboot;
extern bool shouldUpdateNtp;
extern size_t otaTotalSize;
extern String restoreJsonBuffer;
extern const char PLATFORM_MARKER[];
extern const char *firmwareVer;
extern int nLoop;
extern bool restartESP;
extern bool allTestsFinish;
extern bool initConfig;
extern char const *wifiAPpassword;
extern unsigned long currentMillis;
extern unsigned long previousMillis;
extern bool shouldSaveConfig;

// DS18B20
extern bool ds18b20Enable;
extern uint16_t ds18b20UpdateInterval;
extern unsigned long previousDsMillis;
extern String ds18b20UnitsFormat;
extern String dsTemp;
extern char dsTempBuff[5];
extern bool dsTempToDisplay;

// WOPR
extern WOPRData woprZones[4];

// MQTT
extern bool mqttEnable;
extern String mqttServerAddress;
extern uint16_t mqttServerPort;
extern String mqttUsername;
extern String mqttPassword;
extern long lastReconnectAttempt;
extern String shortMACaddr;
extern String MQTTGlobalPrefix;
extern MQTTZoneData MQTTZones[];
extern String MQTTIntensity;
extern String MQTTPower;
extern String MQTTStateTopic;
extern bool mqttPublished;
extern bool shouldMqttPublish;
extern bool shouldMqttDisconnect;
extern bool zoneServiceMessageScrolling[4];
extern unsigned long zoneScrollCompleteTime[4];

// Zones
extern ZoneData zones[];
extern uint8_t zoneNumbers;
extern uint8_t intensity;
extern bool power;
extern bool disableServiceMessages;
extern bool disableDotsBlink;
extern bool configDirty;
extern unsigned long configDirtyTime;
extern char zoneMessages[4][100];
extern char zoneMessagesPending[4][100];

// NTP
extern String ntpServer;
extern float ntpTimeZone;
extern unsigned long previousNTPsyncMillis;
extern uint16_t ntpUpdateInterval;

// OWM
extern unsigned long owmLastTime;
extern uint16_t owmUpdateInterval;
extern String owmUnitsFormat;
extern bool owmWeatherEnable;
extern String owmApiToken;
extern String owmCity;

// HA
extern String haAddr, haApiHttpType, haApiToken;
extern uint16_t haUpdateInterval;
extern uint16_t haApiPort;
extern uint32_t haLastTime;

// Stock Ticker
extern String stockApiToken;
extern uint16_t stockUpdateInterval;
extern unsigned long stockLastTime;
extern bool stockEnable;

// ─── Callback
// ──────────────────────────────────────────────────────────────────
void saveConfigCallback();
