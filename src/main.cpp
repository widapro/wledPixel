#include <Arduino.h>

#if defined(ESP8266)
#pragma message "Compiling for ESP8266 board"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
// #pragma message "Compiling for ESP32 board"
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
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>

#include <WiFiClientSecure.h>

#include <ArduinoOTA.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Preferences.h>

#include "backupPage.h"
#include "base64_utils.h"
#include "indexPage.h"
#include "otaPage.h"
#include "owm.h"

// Global OTA flags
bool globalOtaInProgress = false;
bool shouldReboot = false;
bool shouldUpdateNtp = false; // Flag to trigger NTP update in null loop
size_t otaTotalSize = 0;
String restoreJsonBuffer = "";

#include "settingsPage.h"
#include "wledFont.h"

#if defined(ESP32)
#include <Update.h>
#elif defined(ESP8266)
#include <Updater.h>
// ESP8266 doesn't define UPDATE_SIZE_UNKNOWN, define it as 0 (unknown size)
#ifndef UPDATE_SIZE_UNKNOWN
#define UPDATE_SIZE_UNKNOWN 0
#endif
#endif

Preferences preferences;
WiFiClient mqttEspClient;

/// PINOUT ///
#if defined(ESP8266)
const int oneWireBus = D4; // WeMos D1 mini GPIO02
#define DATA_PIN D7        // WeMos D1 mini GPIO13
#define CS_PIN D6          // WeMos D1 mini GPIO12
#define CLK_PIN D5         // WeMos D1 mini GPIO14
#elif defined(ESP32)
const int oneWireBus = 4; // ESP32 GPIO04
#define DATA_PIN 23 // ESP32 GPIO23
#define CS_PIN 5    // ESP32 GPIO5
#define CLK_PIN 18  // ESP32 GPIO18
#else
#error "This ain't a ESP8266 or ESP32!"
#endif

/// GLOBAL ///
const char *firmwareVer = "v3.3-rc";
int nLoop = 0;
bool restartESP = false;
bool allTestsFinish = false;
bool initConfig = false;
char const *wifiAPpassword = "12345678";
unsigned long currentMillis;
unsigned long previousMillis = 0;

//// DS18B20 ////

OneWire oneWire(oneWireBus); // WeMos D1 mini GPIO02
DallasTemperature sensors(&oneWire);
bool ds18b20Enable = false;
uint16_t ds18b20UpdateInterval = 30; // in seconds
unsigned long previousDsMillis = -100000;
String ds18b20UnitsFormat = "Celsius";
String dsTemp = "-127";
char dsTempBuff[5];
bool dsTempToDisplay = false;

// WOPR Mode variables
typedef struct {
  bool active;
  unsigned long lastUpdate;
  uint16_t updateInterval;
  uint8_t intensity;
  uint8_t blinkPattern[8];
} WOPRData;

WOPRData woprZones[4] = {{false, 0, 100, 7, {0}},
                         {false, 0, 100, 7, {0}},
                         {false, 0, 100, 7, {0}},
                         {false, 0, 100, 7, {0}}};

//// MQTT settings ////
bool mqttEnable = false;
String mqttServerAddress = "";
uint16_t mqttServerPort = 1883;
String mqttUsername = "";
String mqttPassword = "";
long lastReconnectAttempt = 0;
PubSubClient mqttClient(mqttEspClient);

String shortMACaddr =
    WiFi.macAddress().substring(12, 14) +
    WiFi.macAddress().substring(15); // last five chars of the MAC, ie: C0A4;
// String deviceName = shortMACaddr;
String MQTTGlobalPrefix = "wledPixel-" + shortMACaddr;

typedef struct {
  String message, scrollEffectIn, scrollEffectOut, scrollSpeed, scrollPause,
      scrollAllign, charspacing, workMode, scrollInfinite;
} MQTTZoneData;

MQTTZoneData MQTTZones[] = {
    {MQTTGlobalPrefix + "/zone0/text",
     MQTTGlobalPrefix + "/zone0/scrolleffectIn",
     MQTTGlobalPrefix + "/zone0/scrolleffectOut",
     MQTTGlobalPrefix + "/zone0/scrollspeed",
     MQTTGlobalPrefix + "/zone0/scrollpause",
     MQTTGlobalPrefix + "/zone0/scrollalign",
     MQTTGlobalPrefix + "/zone0/charspacing",
     MQTTGlobalPrefix + "/zone0/workmode",
     MQTTGlobalPrefix + "/zone0/scrollInfinite"},
    {MQTTGlobalPrefix + "/zone1/text",
     MQTTGlobalPrefix + "/zone1/scrolleffectIn",
     MQTTGlobalPrefix + "/zone1/scrolleffectOut",
     MQTTGlobalPrefix + "/zone1/scrollspeed",
     MQTTGlobalPrefix + "/zone1/scrollpause",
     MQTTGlobalPrefix + "/zone1/scrollalign",
     MQTTGlobalPrefix + "/zone1/charspacing",
     MQTTGlobalPrefix + "/zone1/workmode",
     MQTTGlobalPrefix + "/zone1/scrollInfinite"},
    {MQTTGlobalPrefix + "/zone2/text",
     MQTTGlobalPrefix + "/zone2/scrolleffectIn",
     MQTTGlobalPrefix + "/zone2/scrolleffectOut",
     MQTTGlobalPrefix + "/zone2/scrollspeed",
     MQTTGlobalPrefix + "/zone2/scrollpause",
     MQTTGlobalPrefix + "/zone2/scrollalign",
     MQTTGlobalPrefix + "/zone2/charspacing",
     MQTTGlobalPrefix + "/zone2/workmode",
     MQTTGlobalPrefix + "/zone2/scrollInfinite"},
    {MQTTGlobalPrefix + "/zone3/text",
     MQTTGlobalPrefix + "/zone3/scrolleffectIn",
     MQTTGlobalPrefix + "/zone3/scrolleffectOut",
     MQTTGlobalPrefix + "/zone3/scrollspeed",
     MQTTGlobalPrefix + "/zone3/scrollpause",
     MQTTGlobalPrefix + "/zone3/scrollalign",
     MQTTGlobalPrefix + "/zone3/charspacing",
     MQTTGlobalPrefix + "/zone3/workmode",
     MQTTGlobalPrefix + "/zone3/scrollInfinite"},
};
String MQTTIntensity = MQTTGlobalPrefix + "/intensity";
String MQTTPower = MQTTGlobalPrefix + "/power";
String MQTTStateTopic = MQTTGlobalPrefix + "/state";
bool mqttPublished = false;
bool shouldMqttPublish = false;
bool shouldMqttDisconnect = false;
bool zoneServiceMessageScrolling[4] = {false, false, false, false};
unsigned long zoneScrollCompleteTime[4] = {
    0, 0, 0, 0}; // Time when scroll animation completed

// Display config
#define HARDWARE_TYPE                                                          \
  MD_MAX72XX::                                                                 \
      FC16_HW // type of device hardware
              // https://majicdesigns.github.io/MD_MAX72XX/page_hardware.html
//// Display pinout

uint8_t MAX_DEVICES = 16; // number of device segments
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Sprites
const uint8_t F_PMAN1 = 6;
const uint8_t W_PMAN1 = 8;
const uint8_t PROGMEM pacman1[F_PMAN1 * W_PMAN1] = // gobbling pacman animation
    {
        0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c, 0x00, 0x42, 0xe7, 0xe7,
        0xff, 0xff, 0x7e, 0x3c, 0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
        0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x24, 0x66, 0xe7, 0xff,
        0xff, 0xff, 0x7e, 0x3c, 0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
};

const uint8_t F_PMAN2 = 6;
const uint8_t W_PMAN2 = 18;
const uint8_t PROGMEM pacman2[F_PMAN2 * W_PMAN2] = // ghost pursued by a pacman
    {
        0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe,
        0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe, 0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff,
        0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
        0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe,
        0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe, 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff,
        0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
        0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe,
        0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe, 0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff,
        0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
};

const uint8_t F_WAVE = 14;
const uint8_t W_WAVE = 14;
const uint8_t PROGMEM wave[F_WAVE * W_WAVE] = // triangular wave / worm
    {
        0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40,
        0x20, 0x10, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
        0x40, 0x80, 0x40, 0x20, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04,
        0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02,
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0x40, 0x20, 0x10,
        0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40, 0x80,
        0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
        0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04,
        0x08, 0x10, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02,
        0x01, 0x02, 0x04, 0x08, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10,
        0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x02, 0x04, 0x08, 0x10,
        0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x01, 0x02,
        0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02,
        0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10,
        0x08, 0x04, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x40, 0x20, 0x10, 0x08,
};

const uint8_t F_MARIO = 1;
const uint8_t W_MARIO = 8;
const uint8_t PROGMEM mario[F_MARIO * W_MARIO] = {0x00, 0xde, 0x7f, 0x5b,
                                                  0x57, 0x7b, 0xd6, 0x00};

const uint8_t F_GHOST = 1;
const uint8_t W_GHOST = 8;
const uint8_t PROGMEM ghost[F_GHOST * W_GHOST] = {0x7c, 0xbe, 0x77, 0xbf,
                                                  0x77, 0xbf, 0x7e, 0x34};

const uint8_t F_DINO = 1;
const uint8_t W_DINO = 8;
const uint8_t PROGMEM dino[F_DINO * W_DINO] = {0x38, 0x30, 0xe0, 0x30,
                                               0x38, 0xfe, 0x3b, 0x23};

const uint8_t F_ROLL1 = 4;
const uint8_t W_ROLL1 = 8;
const uint8_t PROGMEM roll1[F_ROLL1 * W_ROLL1] = // rolling square
    {
        0xff, 0x8f, 0x8f, 0x8f, 0x81, 0x81, 0x81, 0xff, 0xff, 0xf1, 0xf1,
        0xf1, 0x81, 0x81, 0x81, 0xff, 0xff, 0x81, 0x81, 0x81, 0xf1, 0xf1,
        0xf1, 0xff, 0xff, 0x81, 0x81, 0x81, 0x8f, 0x8f, 0x8f, 0xff,
};

const uint8_t F_ROLL2 = 4;
const uint8_t W_ROLL2 = 8;
const uint8_t PROGMEM roll2[F_ROLL2 * W_ROLL2] = // rolling octagon
    {
        0x3c, 0x4e, 0x8f, 0x8f, 0x81, 0x81, 0x42, 0x3c, 0x3c, 0x72, 0xf1,
        0xf1, 0x81, 0x81, 0x42, 0x3c, 0x3c, 0x42, 0x81, 0x81, 0xf1, 0xf1,
        0x72, 0x3c, 0x3c, 0x42, 0x81, 0x81, 0x8f, 0x8f, 0x4e, 0x3c,
};

const uint8_t F_LINES = 3;
const uint8_t W_LINES = 8;
const uint8_t PROGMEM lines[F_LINES * W_LINES] = // spaced lines
    {
        0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff,
        0x00, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff,
};

const uint8_t F_ARROW1 = 3;
const uint8_t W_ARROW1 = 10;
const uint8_t PROGMEM arrow1[F_ARROW1 * W_ARROW1] = // arrow fading to center
    {
        0x18, 0x3c, 0x7e, 0xff, 0x7e, 0x00, 0x00, 0x3c, 0x00, 0x00,
        0x18, 0x3c, 0x7e, 0xff, 0x00, 0x7e, 0x00, 0x00, 0x18, 0x00,
        0x18, 0x3c, 0x7e, 0xff, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x18,
};

const uint8_t F_ARROW2 = 3;
const uint8_t W_ARROW2 = 9;
const uint8_t PROGMEM arrow2[F_ARROW2 * W_ARROW2] = // arrow fading to outside
    {
        0x18, 0x3c, 0x7e, 0xe7, 0x00, 0x00, 0xc3, 0x00, 0x00,
        0x18, 0x3c, 0x7e, 0xe7, 0xe7, 0x00, 0x00, 0x81, 0x00,
        0x18, 0x3c, 0x7e, 0xe7, 0x00, 0xc3, 0x00, 0x00, 0x81,
};

const uint8_t F_SAILBOAT = 1;
const uint8_t W_SAILBOAT = 11;
const uint8_t PROGMEM sailboat[F_SAILBOAT * W_SAILBOAT] = // sail boat
    {
        0x10, 0x30, 0x58, 0x94, 0x92, 0x9f, 0x92, 0x94, 0x98, 0x50, 0x30,
};

const uint8_t F_STEAMBOAT = 2;
const uint8_t W_STEAMBOAT = 11;
const uint8_t PROGMEM steamboat[F_STEAMBOAT * W_STEAMBOAT] = // steam boat
    {
        0x10, 0x30, 0x50, 0x9c, 0x9e, 0x90, 0x91, 0x9c, 0x9d, 0x90, 0x71,
        0x10, 0x30, 0x50, 0x9c, 0x9c, 0x91, 0x90, 0x9d, 0x9e, 0x91, 0x70,
};

const uint8_t F_HEART = 5;
const uint8_t W_HEART = 9;
const uint8_t PROGMEM heart[F_HEART * W_HEART] = // beating heart
    {
        0x0e, 0x11, 0x21, 0x42, 0x84, 0x42, 0x21, 0x11, 0x0e, 0x0e, 0x1f, 0x33,
        0x66, 0xcc, 0x66, 0x33, 0x1f, 0x0e, 0x0e, 0x1f, 0x3f, 0x7e, 0xfc, 0x7e,
        0x3f, 0x1f, 0x0e, 0x0e, 0x1f, 0x33, 0x66, 0xcc, 0x66, 0x33, 0x1f, 0x0e,
        0x0e, 0x11, 0x21, 0x42, 0x84, 0x42, 0x21, 0x11, 0x0e,
};

const uint8_t F_INVADER = 2;
const uint8_t W_INVADER = 10;
const uint8_t PROGMEM invader[F_INVADER * W_INVADER] = // space invader
    {
        0x0e, 0x98, 0x7d, 0x36, 0x3c, 0x3c, 0x36, 0x7d, 0x98, 0x0e,
        0x70, 0x18, 0x7d, 0xb6, 0x3c, 0x3c, 0xb6, 0x7d, 0x18, 0x70,
};

const uint8_t F_ROCKET = 2;
const uint8_t W_ROCKET = 11;
const uint8_t PROGMEM rocket[F_ROCKET * W_ROCKET] = // rocket
    {
        0x18, 0x24, 0x42, 0x81, 0x99, 0x18, 0x99, 0x18, 0xa5, 0x5a, 0x81,
        0x18, 0x24, 0x42, 0x81, 0x18, 0x99, 0x18, 0x99, 0x24, 0x42, 0x99,
};

const uint8_t F_FBALL = 2;
const uint8_t W_FBALL = 11;
const uint8_t PROGMEM fireball[F_FBALL * W_FBALL] = // fireball
    {
        0x7e, 0xab, 0x54, 0x28, 0x52, 0x24, 0x40, 0x18, 0x04, 0x10, 0x08,
        0x7e, 0xd5, 0x2a, 0x14, 0x24, 0x0a, 0x30, 0x04, 0x28, 0x08, 0x10,
};

const uint8_t F_CHEVRON = 1;
const uint8_t W_CHEVRON = 9;
const uint8_t PROGMEM chevron[F_CHEVRON * W_CHEVRON] = // chevron
    {
        0x18, 0x3c, 0x66, 0xc3, 0x99, 0x3c, 0x66, 0xc3, 0x81,
};

const uint8_t F_WALKER = 5;
const uint8_t W_WALKER = 7;
const uint8_t PROGMEM walker[F_WALKER * W_WALKER] = // walking man
    {
        0x00, 0x48, 0x77, 0x1f, 0x1c, 0x94, 0x68, 0x00, 0x90, 0xee, 0x3e, 0x38,
        0x28, 0xd0, 0x00, 0x00, 0xae, 0xfe, 0x38, 0x28, 0x40, 0x00, 0x00, 0x2e,
        0xbe, 0xf8, 0x00, 0x00, 0x00, 0x10, 0x6e, 0x3e, 0xb8, 0xe8, 0x00,
};

typedef struct {
  uint8_t begin, end;
  uint8_t scrollSpeed, charspacing;
  uint8_t scrollPause;
  unsigned long previousMillis;
  String scrollAlign, scrollEffectIn, scrollEffectOut, font, workMode,
      clockDisplayFormat, haSensorId, haSensorPostfix, owmWhatToDisplay,
      mqttPostfix, ds18b20Postfix, curTime;
  bool newMessageAvailable;
  bool scrollInfinite;
  bool restoreEffects;
  bool updateFont;
  bool forceUpdate; // New flag for immediate updates
  bool textFits;
} ZoneData;

// structure:
ZoneData zones[] = {
    {0,
     3,
     35,
     1,
     3,
     0,
     "PA_CENTER",
     "PA_SCROLL_DOWN",
     "PA_NO_EFFECT",
     "wledFont_cyrillic",
     "manualInput",
     "HHMM",
     "",
     "",
     "owmTemperature",
     "",
     "",
     "",
     false,
     true,
     false,
     false,
     false,
     true},
    {4,
     5,
     35,
     1,
     3,
     0,
     "PA_CENTER",
     "PA_SCROLL_DOWN",
     "PA_NO_EFFECT",
     "wledFont_cyrillic",
     "manualInput",
     "HHMM",
     "",
     "",
     "owmTemperature",
     "",
     "",
     "",
     false,
     true,
     false,
     false,
     false,
     true},
    {6,
     7,
     35,
     1,
     3,
     0,
     "PA_CENTER",
     "PA_SCROLL_DOWN",
     "PA_NO_EFFECT",
     "wledFont_cyrillic",
     "manualInput",
     "HHMM",
     "",
     "",
     "owmTemperature",
     "",
     "",
     "",
     false,
     true,
     false,
     false,
     false,
     true},
    {8,
     11,
     35,
     1,
     3,
     0,
     "PA_CENTER",
     "PA_SCROLL_DOWN",
     "PA_NO_EFFECT",
     "wledFont_cyrillic",
     "manualInput",
     "HHMM",
     "",
     "",
     "owmTemperature",
     "",
     "",
     "",
     false,
     true,
     false,
     false,
     false,
     true},
};

uint8_t zoneNumbers = 1;
uint8_t intensity = 7;
bool power = true;
bool disableServiceMessages = false;
bool disableDotsBlink = false;
bool configDirty = false;
unsigned long configDirtyTime = 0;

// Zone message buffers - refactored to array-based approach
char zoneMessages[4][100] = {"zone0", "zone1", "zone2", "zone3"};
char zoneMessagesPending[4][100] = {"", "", "", ""};

// Initialize NTP
String ntpServer = "us.pool.ntp.org";
float ntpTimeZone = 3.0;
WiFiUDP ntpUDP;
unsigned long previousNTPsyncMillis = millis();
uint16_t ntpUpdateInterval = 6; // in hours
NTPClient timeClient(ntpUDP, ntpServer.c_str(), ntpTimeZone * 3600,
                     ntpUpdateInterval * 3600);

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager *wifiManagerPtr = nullptr;

// flag for saving wifi data
bool shouldSaveConfig = false;
// callback notifying us of the need to save config
void saveConfigCallback() { shouldSaveConfig = true; }

// Open Weather Map client
int owmLastTime = 0;
uint16_t owmUpdateInterval = 60;
String owmUnitsFormat = "metric";
bool owmWeatherEnable = false;
bool owmPrinted = true;
String owmApiToken;
String owmCity;

// Home Assistant client
String haAddr, haApiHttpType, haApiToken;
uint16_t haUpdateInterval = 60;
uint16_t haApiPort = 8123;
static uint32_t haLastTime = 0;

// Convert scrollAlig in String to textPosition_t type
textPosition_t stringToTextPositionT(String val) {
  if (val == F("PA_CENTER"))
    return PA_CENTER;
  if (val == F("PA_LEFT"))
    return PA_LEFT;
  if (val == F("PA_RIGHT"))
    return PA_RIGHT;
  return PA_LEFT;
}

void applyZoneFont(int zone, String font) {
  if (font == F("default"))
    P.setFont(zone, nullptr);
  if (font == F("wledFont"))
    P.setFont(zone, wledFont);
  if (font == F("wledFont_cyrillic"))
    P.setFont(zone, wledFont_cyrillic);
  if (font == F("wledSymbolFont"))
    P.setFont(zone, wledSymbolFont);
}

textEffect_t stringToTextEffectT(String val) {
  if (val == F("PA_NO_EFFECT"))
    return PA_NO_EFFECT;
  if (val == F("PA_PRINT"))
    return PA_PRINT;
  if (val == F("PA_SCROLL_UP"))
    return PA_SCROLL_UP;
  if (val == F("PA_SCROLL_DOWN"))
    return PA_SCROLL_DOWN;
  if (val == F("PA_SCROLL_LEFT"))
    return PA_SCROLL_LEFT;
  if (val == F("PA_SCROLL_RIGHT"))
    return PA_SCROLL_RIGHT;
  if (val == F("PACMAN"))
    return PA_SPRITE;
  if (val == F("WAVE"))
    return PA_SPRITE;
  if (val == F("ROLL"))
    return PA_SPRITE;
  if (val == F("LINES"))
    return PA_SPRITE;
  if (val == F("ARROW"))
    return PA_SPRITE;
  if (val == F("SAILBOAT"))
    return PA_SPRITE;
  if (val == F("STEAMBOAT"))
    return PA_SPRITE;
  if (val == F("HEART"))
    return PA_SPRITE;
  if (val == F("INVADER"))
    return PA_SPRITE;
  if (val == F("ROCKET"))
    return PA_SPRITE;
  if (val == F("FBALL"))
    return PA_SPRITE;
  if (val == F("CHEVRON"))
    return PA_SPRITE;
  if (val == F("WALKER"))
    return PA_SPRITE;
  if (val == F("MARIO"))
    return PA_SPRITE;
  if (val == F("GHOST"))
    return PA_SPRITE;
  if (val == F("DINO"))
    return PA_SPRITE;
  if (val == F("PA_SPRITE"))
    return PA_SPRITE;
  if (val == F("PA_SLICE"))
    return PA_SLICE;
  if (val == F("PA_MESH"))
    return PA_MESH;
  if (val == F("PA_FADE"))
    return PA_FADE;
  if (val == F("PA_DISSOLVE"))
    return PA_DISSOLVE;
  if (val == F("PA_BLINDS"))
    return PA_BLINDS;
  if (val == F("PA_RANDOM"))
    return PA_RANDOM;
  if (val == F("PA_WIPE"))
    return PA_WIPE;
  if (val == F("PA_WIPE_CURSOR"))
    return PA_WIPE_CURSOR;
  if (val == F("PA_SCAN_HORIZ"))
    return PA_SCAN_HORIZ;
  if (val == F("PA_SCAN_HORIZX"))
    return PA_SCAN_HORIZX;
  if (val == F("PA_SCAN_VERT"))
    return PA_SCAN_VERT;
  if (val == F("PA_SCAN_VERTX"))
    return PA_SCAN_VERTX;
  if (val == F("PA_OPENING"))
    return PA_OPENING;
  if (val == F("PA_OPENING_CURSOR"))
    return PA_OPENING_CURSOR;
  if (val == F("PA_CLOSING"))
    return PA_CLOSING;
  if (val == F("PA_CLOSING_CURSOR"))
    return PA_CLOSING_CURSOR;
  if (val == F("PA_SCROLL_UP_LEFT"))
    return PA_SCROLL_UP_LEFT;
  if (val == F("PA_SCROLL_UP_RIGHT"))
    return PA_SCROLL_UP_RIGHT;
  if (val == F("PA_SCROLL_DOWN_LEFT"))
    return PA_SCROLL_DOWN_LEFT;
  if (val == F("PA_SCROLL_DOWN_RIGHT"))
    return PA_SCROLL_DOWN_RIGHT;
  if (val == F("PA_GROW_UP"))
    return PA_GROW_UP;
  if (val == F("PA_GROW_DOWN"))
    return PA_GROW_DOWN;
  return PA_SCROLL_LEFT;
}

// Return values for variables in html page
String processor(const String &var) {
  char buffer[10];
  if (var == F("workModeZone0"))
    return zones[0].workMode;
  if (var == F("workModeZone1"))
    return zones[1].workMode;
  if (var == F("workModeZone2"))
    return zones[2].workMode;
  if (var == F("workModeZone3"))
    return zones[3].workMode;
  if (var == F("zoneNumbers"))
    return itoa(zoneNumbers, buffer, 10);
  if (var == F("zone0Begin"))
    return itoa(zones[0].begin, buffer, 10);
  if (var == F("zone0End"))
    return itoa(zones[0].end, buffer, 10);
  if (var == F("zone1Begin"))
    return itoa(zones[1].begin, buffer, 10);
  if (var == F("zone1End"))
    return itoa(zones[1].end, buffer, 10);
  if (var == F("zone2Begin"))
    return itoa(zones[2].begin, buffer, 10);
  if (var == F("zone2End"))
    return itoa(zones[2].end, buffer, 10);
  if (var == F("zone3Begin"))
    return itoa(zones[3].begin, buffer, 10);
  if (var == F("zone3End"))
    return itoa(zones[3].end, buffer, 10);
  if (var == F("wifiSsid"))
    return WiFi.SSID();
  if (var == F("wifiIp"))
    return WiFi.localIP().toString();
  if (var == F("wifiGateway"))
    return WiFi.gatewayIP().toString();
  if (var == F("intensity"))
    return itoa(intensity + 1, buffer, 10);
  if (var == F("scrollSpeedZone0"))
    return itoa(zones[0].scrollSpeed, buffer, 10);
  if (var == F("scrollSpeedZone1"))
    return itoa(zones[1].scrollSpeed, buffer, 10);
  if (var == F("scrollSpeedZone2"))
    return itoa(zones[2].scrollSpeed, buffer, 10);
  if (var == F("scrollSpeedZone3"))
    return itoa(zones[3].scrollSpeed, buffer, 10);
  if (var == F("scrollPauseZone0"))
    return itoa(zones[0].scrollPause, buffer, 10);
  if (var == F("scrollPauseZone1"))
    return itoa(zones[1].scrollPause, buffer, 10);
  if (var == F("scrollPauseZone2"))
    return itoa(zones[2].scrollPause, buffer, 10);
  if (var == F("scrollPauseZone3"))
    return itoa(zones[3].scrollPause, buffer, 10);
  if (var == F("scrollAlignZone0"))
    return zones[0].scrollAlign;
  if (var == F("scrollAlignZone1"))
    return zones[1].scrollAlign;
  if (var == F("scrollAlignZone2"))
    return zones[2].scrollAlign;
  if (var == F("scrollAlignZone3"))
    return zones[3].scrollAlign;
  if (var == F("scrollEffectZone0In"))
    return zones[0].scrollEffectIn;
  if (var == F("scrollEffectZone1In"))
    return zones[1].scrollEffectIn;
  if (var == F("scrollEffectZone2In"))
    return zones[2].scrollEffectIn;
  if (var == F("scrollEffectZone3In"))
    return zones[3].scrollEffectIn;
  if (var == F("scrollEffectZone0Out"))
    return zones[0].scrollEffectOut;
  if (var == F("scrollEffectZone1Out"))
    return zones[1].scrollEffectOut;
  if (var == F("scrollEffectZone2Out"))
    return zones[2].scrollEffectOut;
  if (var == F("scrollEffectZone3Out"))
    return zones[3].scrollEffectOut;
  if (var == F("mqttEnable")) {
    if (mqttEnable)
      return "true";
    if (!mqttEnable)
      return "false";
  }
  if (var == F("mqttServerAddress"))
    return mqttServerAddress;
  if (var == F("mqttServerPort"))
    return itoa(mqttServerPort, buffer, 10);
  if (var == F("mqttUsername"))
    return mqttUsername;
  if (var == F("mqttPassword"))
    return mqttPassword;
  if (var == F("mqttDevicePrefix"))
    return MQTTGlobalPrefix;
  if (var == F("mqttTextTopicZone0"))
    return MQTTZones[0].message;
  if (var == F("mqttTextTopicZone1"))
    return MQTTZones[1].message;
  if (var == F("mqttTextTopicZone2"))
    return MQTTZones[2].message;
  if (var == F("mqttTextTopicZone3"))
    return MQTTZones[3].message;
  if (var == F("mqttPostfixZone0"))
    return zones[0].mqttPostfix;
  if (var == F("mqttPostfixZone1"))
    return zones[1].mqttPostfix;
  if (var == F("mqttPostfixZone2"))
    return zones[2].mqttPostfix;
  if (var == F("mqttPostfixZone3"))
    return zones[3].mqttPostfix;
  if (var == F("ntpTimeZone"))
    return String(ntpTimeZone);
  if (var == F("ntpUpdateInterval"))
    return itoa(ntpUpdateInterval, buffer, 10);
  if (var == F("ntpServer"))
    return ntpServer;
  if (var == F("clockDisplayFormatZone0"))
    return zones[0].clockDisplayFormat;
  if (var == F("clockDisplayFormatZone1"))
    return zones[1].clockDisplayFormat;
  if (var == F("clockDisplayFormatZone2"))
    return zones[2].clockDisplayFormat;
  if (var == F("clockDisplayFormatZone3"))
    return zones[3].clockDisplayFormat;
  if (var == F("owmApiToken"))
    return owmApiToken;
  if (var == F("owmUnitsFormat"))
    return owmUnitsFormat;
  if (var == F("owmUpdateInterval"))
    return itoa(owmUpdateInterval, buffer, 10);
  if (var == F("owmWhatToDisplayZone0"))
    return zones[0].owmWhatToDisplay;
  if (var == F("owmWhatToDisplayZone1"))
    return zones[1].owmWhatToDisplay;
  if (var == F("owmWhatToDisplayZone2"))
    return zones[2].owmWhatToDisplay;
  if (var == F("owmWhatToDisplayZone3"))
    return zones[3].owmWhatToDisplay;
  if (var == F("owmCity"))
    return owmCity;
  if (var == F("fontZone0"))
    return zones[0].font;
  if (var == F("fontZone1"))
    return zones[1].font;
  if (var == F("fontZone2"))
    return zones[2].font;
  if (var == F("fontZone3"))
    return zones[3].font;
  if (var == F("haAddr"))
    return haAddr;
  if (var == F("haApiToken"))
    return haApiToken;
  if (var == F("haApiHttpType"))
    return haApiHttpType;
  if (var == F("haApiPort"))
    return itoa(haApiPort, buffer, 10);
  if (var == F("haSensorIdZone0"))
    return zones[0].haSensorId;
  if (var == F("haSensorIdZone1"))
    return zones[1].haSensorId;
  if (var == F("haSensorIdZone2"))
    return zones[2].haSensorId;
  if (var == F("haSensorIdZone3"))
    return zones[3].haSensorId;
  if (var == F("haSensorPostfixZone0"))
    return zones[0].haSensorPostfix;
  if (var == F("haSensorPostfixZone1"))
    return zones[1].haSensorPostfix;
  if (var == F("haSensorPostfixZone2"))
    return zones[2].haSensorPostfix;
  if (var == F("haSensorPostfixZone3"))
    return zones[3].haSensorPostfix;
  if (var == F("haUpdateInterval"))
    return itoa(haUpdateInterval, buffer, 10);
  if (var == F("charspacingZone0"))
    return itoa(zones[0].charspacing, buffer, 10);
  if (var == F("charspacingZone1"))
    return itoa(zones[1].charspacing, buffer, 10);
  if (var == F("charspacingZone2"))
    return itoa(zones[2].charspacing, buffer, 10);
  if (var == F("charspacingZone3"))
    return itoa(zones[3].charspacing, buffer, 10);
  if (var == F("firmwareVer"))
    return firmwareVer;
  if (var == F("deviceName"))
    return shortMACaddr;
  if (var == F("disableServiceMessages"))
    return String(disableServiceMessages);
  if (var == F("disableDotsBlink"))
    return String(disableDotsBlink);
  if (var == F("ds18b20Enable"))
    return String(ds18b20Enable);
  if (var == F("ds18b20Temp"))
    return (dsTemp == "-127") ? "Not connected" : dsTemp;
  if (var == F("ds18b20UpdateInterval"))
    return itoa(ds18b20UpdateInterval, buffer, 10);
  if (var == F("ds18b20UnitsFormat"))
    return ds18b20UnitsFormat;
  if (var == F("ds18b20PostfixZone0"))
    return zones[0].ds18b20Postfix;
  if (var == F("ds18b20PostfixZone1"))
    return zones[1].ds18b20Postfix;
  if (var == F("ds18b20PostfixZone2"))
    return zones[2].ds18b20Postfix;
  if (var == F("ds18b20PostfixZone3"))
    return zones[3].ds18b20Postfix;
  return String();
}

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
      // root["opt"] = true;
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

    // result = mqttClient.beginPublish(topic_config, measureJson(root), true);
    // if( result == false ) Serial.println(F( "MQTT HA config error begin!" ));
    // serializeJson(root, mqttClient);

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

void saveVarsToConfFile(String groupName, uint8_t n) {
  preferences.begin(groupName.c_str(), false);

  if (groupName == "systemSettings") {
    preferences.putString("deviceName", shortMACaddr);
    preferences.putBool("disablServMess", disableServiceMessages);
  }

  if (groupName == "displaySettings") {
    preferences.putUChar("zoneNumbers", zoneNumbers);
    preferences.putUChar("zones0Begin", zones[0].begin);
    preferences.putUChar("zones0End", zones[0].end);
    preferences.putUChar("zones1Begin", zones[1].begin);
    preferences.putUChar("zones1End", zones[1].end);
    preferences.putUChar("zones2Begin", zones[2].begin);
    preferences.putUChar("zones2End", zones[2].end);
    preferences.putUChar("zones3Begin", zones[3].begin);
    preferences.putUChar("zones3End", zones[3].end);
  }

  if (groupName == "zoneSettings") {
    preferences.putString((String("zone") + n + "WorkMode").c_str(),
                          zones[n].workMode);
    preferences.putUChar((String("zone") + n + "ScrolSpeed").c_str(),
                         zones[n].scrollSpeed);
    preferences.putUChar((String("zone") + n + "ScrolPause").c_str(),
                         zones[n].scrollPause);
    preferences.putString((String("zone") + n + "ScrolAlign").c_str(),
                          zones[n].scrollAlign);
    preferences.putString((String("zone") + n + "ScrolEfIn").c_str(),
                          zones[n].scrollEffectIn);
    preferences.putString((String("zone") + n + "ScrolEfOut").c_str(),
                          zones[n].scrollEffectOut);
    preferences.putString((String("zone") + n + "Font").c_str(), zones[n].font);
    preferences.putUChar((String("zone") + n + "Charspac").c_str(),
                         zones[n].charspacing);
    preferences.putString((String("zone") + n + "mqttTexTop").c_str(),
                          MQTTZones[n].message);
    preferences.putString((String("zone") + n + "mqttPosfix").c_str(),
                          zones[n].mqttPostfix);
    preferences.putString((String("zone") + n + "ClDispForm").c_str(),
                          zones[n].clockDisplayFormat);
    preferences.putString((String("zone") + n + "OwmWhDisp").c_str(),
                          zones[n].owmWhatToDisplay);
    preferences.putString((String("zone") + n + "HaSensorId").c_str(),
                          zones[n].haSensorId);
    preferences.putString((String("zone") + n + "HaSensorPf").c_str(),
                          zones[n].haSensorPostfix);
    preferences.putString((String("zone") + n + "Ds18b20Pf").c_str(),
                          zones[n].ds18b20Postfix);
    preferences.putUShort((String("zone") + n + "WoprUpdInt").c_str(),
                          woprZones[n].updateInterval);
    preferences.putBool((String("zone") + n + "ScrolInf").c_str(),
                        zones[n].scrollInfinite);
  }

  if (groupName == "mqttSettings") {
    preferences.putBool("mqttEnable", mqttEnable);
    preferences.putString("mqttServerAddr", mqttServerAddress);
    preferences.putUShort("mqttServerPort", mqttServerPort);
    preferences.putString("mqttUsername", mqttUsername);
    preferences.putString("mqttPassword", mqttPassword);
  }

  if (groupName == "wallClockSett") {
    preferences.putFloat("ntpTz", ntpTimeZone);
    preferences.putBool("disableDotBlink", disableDotsBlink);
    preferences.putUShort("ntpUpdateInt", ntpUpdateInterval);
    preferences.putString("ntpServer", ntpServer);
  }

  if (groupName == "owmSettings") {
    preferences.putString("owmApiToken", owmApiToken);
    preferences.putString("owmUnitsFormat", owmUnitsFormat);
    preferences.putUShort("owmUpdateInt", owmUpdateInterval);
    preferences.putString("owmCity", owmCity);
  }

  if (groupName == "haSettings") {
    preferences.putString("haAddr", haAddr);
    preferences.putString("haApiToken", haApiToken);
    preferences.putString("haApiHttpType", haApiHttpType);
    preferences.putUShort("haApiPort", haApiPort);
    preferences.putUShort("haUpdateInt", haUpdateInterval);
  }

  if (groupName == "ds18b20Settings") {
    preferences.putBool("ds18b20Enable", ds18b20Enable);
    preferences.putUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    preferences.putString("ds18b20UnitFr", ds18b20UnitsFormat);
  }

  if (groupName == "intensity") {
    preferences.putUChar("intensity", intensity);
  }

  preferences.end();
}

void readConfig(String groupName, uint8_t n) {
  preferences.begin(groupName.c_str(), true);

  if (groupName == "systemSettings") {
    shortMACaddr = preferences.getString("deviceName", shortMACaddr);
    disableServiceMessages =
        preferences.getBool("disablServMess", disableServiceMessages);
  }

  if (groupName == "displaySettings") {
    zoneNumbers = preferences.getUChar("zoneNumbers", zoneNumbers);
    zones[0].begin = preferences.getUChar("zones0Begin", zones[0].begin);
    zones[0].end = preferences.getUChar("zones0End", zones[0].end);
    zones[1].begin = preferences.getUChar("zones1Begin", zones[1].begin);
    zones[1].end = preferences.getUChar("zones1End", zones[1].end);
    zones[2].begin = preferences.getUChar("zones2Begin", zones[2].begin);
    zones[2].end = preferences.getUChar("zones2End", zones[2].end);
    zones[3].begin = preferences.getUChar("zones3Begin", zones[3].begin);
    zones[3].end = preferences.getUChar("zones3End", zones[3].end);
  }

  if (groupName == "zoneSettings") {
    zones[n].workMode = preferences.getString(
        (String("zone") + n + "WorkMode").c_str(), zones[n].workMode);
    zones[n].scrollSpeed = preferences.getUChar(
        (String("zone") + n + "ScrolSpeed").c_str(), zones[n].scrollSpeed);
    zones[n].scrollPause = preferences.getUChar(
        (String("zone") + n + "ScrolPause").c_str(), zones[n].scrollPause);
    zones[n].scrollAlign = preferences.getString(
        (String("zone") + n + "ScrolAlign").c_str(), zones[n].scrollAlign);
    zones[n].scrollEffectIn = preferences.getString(
        (String("zone") + n + "ScrolEfIn").c_str(), zones[n].scrollEffectIn);
    zones[n].scrollEffectOut = preferences.getString(
        (String("zone") + n + "ScrolEfOut").c_str(), zones[n].scrollEffectOut);
    zones[n].font = preferences.getString((String("zone") + n + "Font").c_str(),
                                          zones[n].font);
    zones[n].charspacing = preferences.getUChar(
        (String("zone") + n + "Charspac").c_str(), zones[n].charspacing);
    MQTTZones[n].message = preferences.getString(
        (String("zone") + n + "mqttTexTop").c_str(), MQTTZones[n].message);
    zones[n].mqttPostfix = preferences.getString(
        (String("zone") + n + "mqttPosfix").c_str(), zones[n].mqttPostfix);
    zones[n].clockDisplayFormat =
        preferences.getString((String("zone") + n + "ClDispForm").c_str(),
                              zones[n].clockDisplayFormat);
    zones[n].owmWhatToDisplay = preferences.getString(
        (String("zone") + n + "OwmWhDisp").c_str(), zones[n].owmWhatToDisplay);
    zones[n].scrollInfinite = preferences.getBool(
        (String("zone") + n + "ScrolInf").c_str(), zones[n].scrollInfinite);
    zones[n].haSensorId = preferences.getString(
        (String("zone") + n + "HaSensorId").c_str(), zones[n].haSensorId);
    zones[n].haSensorPostfix = preferences.getString(
        (String("zone") + n + "HaSensorPf").c_str(), zones[n].haSensorPostfix);
    zones[n].ds18b20Postfix = preferences.getString(
        (String("zone") + n + "Ds18b20Pf").c_str(), zones[n].ds18b20Postfix);
    woprZones[n].updateInterval =
        preferences.getUShort((String("zone") + n + "WoprUpdInt").c_str(),
                              woprZones[n].updateInterval);
  }

  if (groupName == "mqttSettings") {
    mqttEnable = preferences.getBool("mqttEnable", mqttEnable);
    mqttServerAddress =
        preferences.getString("mqttServerAddr", mqttServerAddress);
    mqttServerPort = preferences.getUShort("mqttServerPort", mqttServerPort);
    mqttUsername = preferences.getString("mqttUsername", mqttUsername);
    mqttPassword = preferences.getString("mqttPassword", mqttPassword);
    Serial.print("readConfig: Loaded mqttPassword: ");
    Serial.println(mqttPassword.length() > 0 ? "EXISTS" : "EMPTY");
  }

  if (groupName == "wallClockSett") {
    ntpTimeZone = preferences.getFloat("ntpTz", ntpTimeZone);
    disableDotsBlink = preferences.getBool("disableDotBlink", disableDotsBlink);
    ntpUpdateInterval =
        preferences.getUShort("ntpUpdateInt", ntpUpdateInterval);
    ntpServer = preferences.getString("ntpServer", ntpServer);
  }

  if (groupName == "owmSettings") {
    owmApiToken = preferences.getString("owmApiToken", owmApiToken);
    owmUnitsFormat = preferences.getString("owmUnitsFormat", owmUnitsFormat);
    owmUpdateInterval =
        preferences.getUShort("owmUpdateInt", owmUpdateInterval);
    owmCity = preferences.getString("owmCity", owmCity);
  }

  if (groupName == "haSettings") {
    haAddr = preferences.getString("haAddr", haAddr);
    haApiToken = preferences.getString("haApiToken", haApiToken);
    haApiHttpType = preferences.getString("haApiHttpType", haApiHttpType);
    haApiPort = preferences.getUShort("haApiPort", haApiPort);
    haUpdateInterval = preferences.getUShort("haUpdateInt", haUpdateInterval);
  }

  if (groupName == "ds18b20Settings") {
    ds18b20Enable = preferences.getBool("ds18b20Enable", ds18b20Enable);
    ds18b20UpdateInterval =
        preferences.getUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    ds18b20UnitsFormat =
        preferences.getString("ds18b20UnitFr", ds18b20UnitsFormat);
  }

  if (groupName == "intensity") {
    intensity = preferences.getUChar("intensity", intensity);
  }

  preferences.end();
}

void readAllConfig() {
  readConfig("systemSettings", 99);
  readConfig("displaySettings", 99);
  for (uint8_t n = 0; n < zoneNumbers; n++) {
    readConfig("zoneSettings", n);
  }
  readConfig("mqttSettings", 99);
  readConfig("wallClockSett", 99);
  readConfig("owmSettings", 99);
  readConfig("haSettings", 99);
  readConfig("ds18b20Settings", 99);
  readConfig("intensity", 99);
  readConfig("intensity", 99);
}

// check a string to see if it is numeric (all characters must be digits)
bool isNumeric(String str);

void zoneNewMessage(int zone, String newMessage, String postfix,
                    bool restoreEffects = true, bool force = false) {
  if (zone >= 0 && zone < 4) {
    String fullMessage = newMessage + postfix;

    // Calculate if text fits for display optimization
    int16_t textWidth = P.getTextColumns(zone, fullMessage.c_str());
    // Fallback heuristic if getTextColumns returns 0 (e.g. font not loaded or
    // helper issue)
    if (textWidth == 0 && fullMessage.length() > 0)
      textWidth = fullMessage.length() * 6;

    uint16_t startCol, endCol;
    P.getDisplayExtent(zone, startCol, endCol);
    int16_t zoneWidth = abs(startCol - endCol) + 1;

    zones[zone].textFits = (textWidth <= zoneWidth);

    if (!force) {
      // Check against pending if available, otherwise active
      if (zones[zone].newMessageAvailable) {
        if (String(zoneMessagesPending[zone]) == fullMessage)
          return;
      } else {
        if (String(zoneMessages[zone]) == fullMessage)
          return;
      }
    }

    strcpy(zoneMessagesPending[zone], fullMessage.c_str());
    zones[zone].newMessageAvailable = true;
    zones[zone].restoreEffects = restoreEffects;
  }
}

// Display mode name message that stays on screen briefly (uses PA_PRINT for IN
// and PA_NO_EFFECT for OUT)
void zoneShowModeMessage(int zone, String modeName) {
  if (zone >= 0 && zone < 4) {
    Serial.print("Debug: zoneShowModeMessage called for zone ");
    Serial.print(zone);
    Serial.print(" with: ");
    Serial.println(modeName);

    if (disableServiceMessages)
      return;

    // If the message is already displayed, don't restart the animation/display
    if (String(zoneMessages[zone]) == modeName) {
      Serial.println("Debug: Message already displayed, skipping update.");
      return;
    }

    P.displayClear(zone); // Stop current animation

    // Calculate text width vs zone width
    // We need to temporarily set the buffer to measure it properly or use
    // helper
    int16_t textWidth = P.getTextColumns(zone, modeName.c_str());
    // Fallback heuristic if getTextColumns returns 0 (sometimes happens if not
    // yet displayed)
    if (textWidth == 0 && modeName.length() > 0)
      textWidth = modeName.length() * 6; // Approx 6px per char

    uint16_t startCol, endCol;
    P.getDisplayExtent(zone, startCol, endCol);
    int16_t zoneWidth = abs(startCol - endCol) + 1;

    if (textWidth <= zoneWidth) {
      // Fits: Static display
      P.setTextEffect(zone, PA_PRINT, PA_NO_EFFECT);
      P.setPause(zone, 500); // Brief 0.5 second pause
      P.setTextAlignment(zone, PA_CENTER);
      zoneServiceMessageScrolling[zone] = false;
    } else {
      // Doesn't fit: Scroll it
      P.setTextEffect(zone, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      P.setPause(zone, 0);
      P.setTextAlignment(zone, PA_LEFT);
      zoneServiceMessageScrolling[zone] = true;
    }

    // Copy message directly to active buffer (bypass pending)
    strcpy(zoneMessages[zone], modeName.c_str());
    strcpy(zoneMessagesPending[zone], ""); // Clear pending
    zones[zone].newMessageAvailable = false;
    zones[zone].restoreEffects = true; // Restore effects on next real message

    // Immediately update display
    P.setTextBuffer(zone, zoneMessages[zone]);
    P.displayReset(zone);
  }
}

// check a string to see if it is numeric (all characters must be digits)
bool isNumeric(String str) {
  if (str.length() == 0)
    return false;
  for (byte i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i)))
      return false;
  }
  return true;
}

void ntpUpdateTime() {
  if (timeClient.update()) {
    previousNTPsyncMillis = currentMillis;
    Serial.print(F("\nNTP time synchronized successfully"));
  } else {
    previousNTPsyncMillis = currentMillis - (ntpUpdateInterval * 3600 * 1000) +
                            30000; // Retry in 30 seconds
    Serial.print(
        F("\nERROR: NTP time synchronization failed, will retry in 30s"));
  }
}

void MQTTCallback(char *topic, byte *payload, int length) {
  String topicStr(topic);
  // Pre-allocate string to prevent heap fragmentation
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
    // scrollInfinite check - must be outside workMode block
    if (topicStr == MQTTZones[n].scrollInfinite) {
      if (PayloadString == "on" || PayloadString == "ON" ||
          PayloadString == "TRUE" || PayloadString == "true" ||
          PayloadString == "1")
        zones[n].scrollInfinite = true;
      else
        zones[n].scrollInfinite = false;

      zoneSettingsChanged = true;
    }
    if (zoneSettingsChanged) {
      // Defer save to prevent UI freeze
      configDirty = true;
      configDirtyTime = millis();
    }
  }
  MQTTPublishState();
}

boolean reconnect() {
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);

  // Prepare Last Will & Testament for offline detection
  String lwtTopic = MQTTGlobalPrefix + "/state";
  const char *lwtMessage = "{\"status\":\"offline\"}";

  // Attempt to connect with LWT (QoS 1, retain=true)
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
    }
    MQTTPublishState();
    Serial.println(F("MQTT subscribe objects"));
    mqttClient.subscribe((char *)MQTTIntensity.c_str());
    mqttClient.subscribe((char *)MQTTPower.c_str());

    // Display feedback on MQTT zones
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

void wifiApWelcomeMessage(AsyncWiFiManager *wifiManager) {
  Serial.println(F("AP Callback Triggered"));
  P.displayClear();
  P.setTextAlignment(0, PA_CENTER);
  P.setIntensity(7);
  P.setSpeed(0, 100);
  P.print("AP Mode");
  P.displayAnimate();
}

String getCurTime(String curZoneFont, String displayFormat) {
  if (currentMillis - previousNTPsyncMillis >=
      (unsigned)ntpUpdateInterval * 3600 * 1000)
    ntpUpdateTime();

  String t = (String)timeClient.getFormattedTime(); // returns HH:MM:SS

  if (displayFormat == "HHMM") {
    t.remove(5, 4);
    return t;
  }
  if (displayFormat == "HH") {
    t.remove(2, 7);
    return t;
  }
  if (displayFormat == "MM") {
    t.remove(5, 4);
    t.remove(0, 3);
    return t;
  }

  time_t epochTime = timeClient.getEpochTime(); // returns the Unix epoch
  // Get a time structure
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;

  String currentMonthStr = "";
  if (currentMonth < 10)
    currentMonthStr += '0';
  currentMonthStr += currentMonth;

  String monthDayStr = "";
  if (monthDay < 10)
    monthDayStr += '0';
  monthDayStr += monthDay;

  // String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday",
  // "Thursday", "Friday", "Saturday"};
  String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  String weekDaysCyrillic[7] = {"½", "§", "©", "®", "µ", "¶", "¼"};
  String weekDay;
  if (curZoneFont == "wledFont_cyrillic") {
    weekDay = weekDaysCyrillic[timeClient.getDay()];
  } else {
    weekDay = weekDays[timeClient.getDay()];
  }
  int currentYear = ptm->tm_year + 1900;

  if (displayFormat == "ddmmyyyy")
    t = monthDayStr + "." + currentMonthStr + "." + String(currentYear);
  if (displayFormat == "ddmm")
    t = monthDayStr + "." + currentMonthStr;
  if (displayFormat == "ddmmaa")
    t = monthDayStr + "." + currentMonthStr + String(weekDay);
  if (displayFormat == "aa")
    t = String(weekDay);
  if (displayFormat == "ddmmaahhmm") {
    t.remove(5, 4);
    t = monthDayStr + "." + currentMonthStr + " " + String(weekDay) + " " + t;
  }
  return t;
}

// Helper to update sprite data based on zone settings
void updateSpriteData(uint8_t z) {
  if (zones[z].scrollEffectIn == "PACMAN" ||
      zones[z].scrollEffectOut == "PACMAN") {
    P.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2, F_PMAN2);
  } else if (zones[z].scrollEffectIn == "WAVE" ||
             zones[z].scrollEffectOut == "WAVE") {
    P.setSpriteData(wave, W_WAVE, F_WAVE, wave, W_WAVE, F_WAVE);
  } else if (zones[z].scrollEffectIn == "ROLL" ||
             zones[z].scrollEffectOut == "ROLL") {
    P.setSpriteData(roll1, W_ROLL1, F_ROLL1, roll2, W_ROLL2, F_ROLL2);
  } else if (zones[z].scrollEffectIn == "LINES" ||
             zones[z].scrollEffectOut == "LINES") {
    P.setSpriteData(lines, W_LINES, F_LINES, lines, W_LINES, F_LINES);
  } else if (zones[z].scrollEffectIn == "ARROW" ||
             zones[z].scrollEffectOut == "ARROW") {
    P.setSpriteData(arrow1, W_ARROW1, F_ARROW1, arrow2, W_ARROW2, F_ARROW2);
  } else if (zones[z].scrollEffectIn == "SAILBOAT" ||
             zones[z].scrollEffectOut == "SAILBOAT") {
    P.setSpriteData(sailboat, W_SAILBOAT, F_SAILBOAT, sailboat, W_SAILBOAT,
                    F_SAILBOAT);
  } else if (zones[z].scrollEffectIn == "STEAMBOAT" ||
             zones[z].scrollEffectOut == "STEAMBOAT") {
    P.setSpriteData(steamboat, W_STEAMBOAT, F_STEAMBOAT, steamboat, W_STEAMBOAT,
                    F_STEAMBOAT);
  } else if (zones[z].scrollEffectIn == "HEART" ||
             zones[z].scrollEffectOut == "HEART") {
    P.setSpriteData(heart, W_HEART, F_HEART, heart, W_HEART, F_HEART);
  } else if (zones[z].scrollEffectIn == "INVADER" ||
             zones[z].scrollEffectOut == "INVADER") {
    P.setSpriteData(invader, W_INVADER, F_INVADER, invader, W_INVADER,
                    F_INVADER);
  } else if (zones[z].scrollEffectIn == "ROCKET" ||
             zones[z].scrollEffectOut == "ROCKET") {
    P.setSpriteData(rocket, W_ROCKET, F_ROCKET, rocket, W_ROCKET, F_ROCKET);
  } else if (zones[z].scrollEffectIn == "FBALL" ||
             zones[z].scrollEffectOut == "FBALL") {
    P.setSpriteData(fireball, W_FBALL, F_FBALL, fireball, W_FBALL, F_FBALL);
  } else if (zones[z].scrollEffectIn == "CHEVRON" ||
             zones[z].scrollEffectOut == "CHEVRON") {
    P.setSpriteData(chevron, W_CHEVRON, F_CHEVRON, chevron, W_CHEVRON,
                    F_CHEVRON);
  } else if (zones[z].scrollEffectIn == "WALKER" ||
             zones[z].scrollEffectOut == "WALKER") {
    P.setSpriteData(walker, W_WALKER, F_WALKER, walker, W_WALKER, F_WALKER);
  } else if (zones[z].scrollEffectIn == "MARIO" ||
             zones[z].scrollEffectOut == "MARIO") {
    P.setSpriteData(mario, W_MARIO, F_MARIO, mario, W_MARIO, F_MARIO);
  } else if (zones[z].scrollEffectIn == "GHOST" ||
             zones[z].scrollEffectOut == "GHOST") {
    P.setSpriteData(ghost, W_GHOST, F_GHOST, ghost, W_GHOST, F_GHOST);
  } else if (zones[z].scrollEffectIn == "DINO" ||
             zones[z].scrollEffectOut == "DINO") {
    P.setSpriteData(dino, W_DINO, F_DINO, dino, W_DINO, F_DINO);
  }
}

String flashClockDots(String t) {
  if (t.indexOf(":") > 0)
    t.replace(":", "¦");
  else
    t.replace("¦", ":");
  return t;
}

void displayAnimation() {
  P.displayAnimate();
  for (uint8_t z = 0; z < zoneNumbers && z < 4; z++) {
    if (woprZones[z].active)
      continue; // Skip Parola logic if WOPR is active
    if (P.getZoneStatus(z)) {
      if (zones[z].newMessageAvailable) {
        // --- Swap Buffer & Configure ---
        Serial.print("Debug: Swapping buffer for zone ");
        Serial.println(z);
        strcpy(zoneMessages[z], zoneMessagesPending[z]);
        zones[z].newMessageAvailable = false;

        if (zones[z].restoreEffects) {
          textEffect_t effIn = stringToTextEffectT(zones[z].scrollEffectIn);
          textEffect_t effOut = stringToTextEffectT(zones[z].scrollEffectOut);

          if (zones[z].workMode == "wallClock" ||
              zones[z].workMode == "owmWeather") {
            effOut = PA_NO_EFFECT;
          }

          int16_t textWidth = P.getTextColumns(z, zoneMessages[z]);
          // Fallback heuristic
          if (textWidth == 0 && strlen(zoneMessages[z]) > 0)
            textWidth = strlen(zoneMessages[z]) * 7;

          uint16_t startCol, endCol;
          P.getDisplayExtent(z, startCol, endCol);
          int16_t zoneWidth = abs(startCol - endCol) + 1;

          zones[z].textFits = (textWidth <= zoneWidth);

          if (!zones[z].textFits) {
            // Force Ticker Mode for wide text
            P.setTextAlignment(z, PA_LEFT);
            effIn = PA_SCROLL_LEFT;
            effOut = PA_SCROLL_LEFT;
            // No mid-scroll pause - for clock, pause will be blank screen
            // between loops
            P.setPause(z, 0);
            P.displayClear(z);
          } else {
            // Restore standard config
            P.setTextAlignment(z, stringToTextPositionT(zones[z].scrollAlign));
            P.setPause(z, zones[z].scrollPause * 1000);
          }

          if (zones[z].scrollEffectIn == "PACMAN" ||
              zones[z].scrollEffectOut == "PACMAN") {
            P.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2,
                            F_PMAN2);
          } else if (zones[z].scrollEffectIn == "WAVE" ||
                     zones[z].scrollEffectOut == "WAVE") {
            P.setSpriteData(wave, W_WAVE, F_WAVE, wave, W_WAVE, F_WAVE);
          } else if (zones[z].scrollEffectIn == "ROLL" ||
                     zones[z].scrollEffectOut == "ROLL") {
            P.setSpriteData(roll1, W_ROLL1, F_ROLL1, roll2, W_ROLL2, F_ROLL2);
          } else if (zones[z].scrollEffectIn == "LINES" ||
                     zones[z].scrollEffectOut == "LINES") {
            P.setSpriteData(lines, W_LINES, F_LINES, lines, W_LINES, F_LINES);
          } else if (zones[z].scrollEffectIn == "ARROW" ||
                     zones[z].scrollEffectOut == "ARROW") {
            P.setSpriteData(arrow1, W_ARROW1, F_ARROW1, arrow2, W_ARROW2,
                            F_ARROW2);
          } else if (zones[z].scrollEffectIn == "SAILBOAT" ||
                     zones[z].scrollEffectOut == "SAILBOAT") {
            P.setSpriteData(sailboat, W_SAILBOAT, F_SAILBOAT, sailboat,
                            W_SAILBOAT, F_SAILBOAT);
          } else if (zones[z].scrollEffectIn == "STEAMBOAT" ||
                     zones[z].scrollEffectOut == "STEAMBOAT") {
            P.setSpriteData(steamboat, W_STEAMBOAT, F_STEAMBOAT, steamboat,
                            W_STEAMBOAT, F_STEAMBOAT);
          } else if (zones[z].scrollEffectIn == "HEART" ||
                     zones[z].scrollEffectOut == "HEART") {
            P.setSpriteData(heart, W_HEART, F_HEART, heart, W_HEART, F_HEART);
          } else if (zones[z].scrollEffectIn == "INVADER" ||
                     zones[z].scrollEffectOut == "INVADER") {
            P.setSpriteData(invader, W_INVADER, F_INVADER, invader, W_INVADER,
                            F_INVADER);
          } else if (zones[z].scrollEffectIn == "ROCKET" ||
                     zones[z].scrollEffectOut == "ROCKET") {
            P.setSpriteData(rocket, W_ROCKET, F_ROCKET, rocket, W_ROCKET,
                            F_ROCKET);
          } else if (zones[z].scrollEffectIn == "FBALL" ||
                     zones[z].scrollEffectOut == "FBALL") {
            P.setSpriteData(fireball, W_FBALL, F_FBALL, fireball, W_FBALL,
                            F_FBALL);
          } else if (zones[z].scrollEffectIn == "CHEVRON" ||
                     zones[z].scrollEffectOut == "CHEVRON") {
            P.setSpriteData(chevron, W_CHEVRON, F_CHEVRON, chevron, W_CHEVRON,
                            F_CHEVRON);
          } else if (zones[z].scrollEffectIn == "WALKER" ||
                     zones[z].scrollEffectOut == "WALKER") {
            P.setSpriteData(walker, W_WALKER, F_WALKER, walker, W_WALKER,
                            F_WALKER);
          } else if (zones[z].scrollEffectIn == "MARIO" ||
                     zones[z].scrollEffectOut == "MARIO") {
            P.setSpriteData(mario, W_MARIO, F_MARIO, mario, W_MARIO, F_MARIO);
          } else if (zones[z].scrollEffectIn == "GHOST" ||
                     zones[z].scrollEffectOut == "GHOST") {
            P.setSpriteData(ghost, W_GHOST, F_GHOST, ghost, W_GHOST, F_GHOST);
          } else if (zones[z].scrollEffectIn == "DINO" ||
                     zones[z].scrollEffectOut == "DINO") {
            P.setSpriteData(dino, W_DINO, F_DINO, dino, W_DINO, F_DINO);
          }

          P.setTextEffect(z, effIn, effOut);
        } else {
          if ((zones[z].workMode == "wallClock" ||
               zones[z].workMode == "owmWeather") &&
              allTestsFinish) {
            // Check if text fits - if not, use scroll effects instead of
            // NO_EFFECT
            int16_t textWidth = P.getTextColumns(z, zoneMessages[z]);
            if (textWidth == 0 && strlen(zoneMessages[z]) > 0)
              textWidth = strlen(zoneMessages[z]) * 7;
            uint16_t startCol, endCol;
            P.getDisplayExtent(z, startCol, endCol);
            int16_t zoneWidth = abs(startCol - endCol) + 1;
            zones[z].textFits = (textWidth <= zoneWidth);

            if (!zones[z].textFits) {
              P.setTextAlignment(z, PA_LEFT);
              P.setTextEffect(z, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
              P.setPause(z, 0);
            } else {
              P.setTextEffect(z, PA_NO_EFFECT, PA_NO_EFFECT);
            }
          }
        }
        P.setTextBuffer(z, zoneMessages[z]);
        P.displayReset(z);
      } else if (zoneServiceMessageScrolling[z]) {
        // Scroll finished, switch to static display
        zoneServiceMessageScrolling[z] = false;
        P.setTextEffect(z, PA_PRINT, PA_NO_EFFECT);
        P.setTextAlignment(z, PA_LEFT);
        P.displayReset(z);
      } else if ((allTestsFinish && zones[z].scrollInfinite &&
                  (zones[z].workMode == "mqttClient" ||
                   zones[z].workMode == "manualInput")) ||
                 (allTestsFinish && zones[z].workMode == "wallClock" &&
                  !zones[z].textFits)) {

        // For wallClock with text that doesn't fit: blank screen pause between
        // loops
        if (zones[z].workMode == "wallClock" && !zones[z].textFits) {
          if (zoneScrollCompleteTime[z] == 0) {
            // First time detecting scroll complete - record time
            zoneScrollCompleteTime[z] = millis();
          }
          // Wait for scrollPause seconds (blank screen)
          if (millis() - zoneScrollCompleteTime[z] >=
              (unsigned long)(zones[z].scrollPause * 1000)) {
            zoneScrollCompleteTime[z] = 0; // Reset timer
            P.displayReset(z);
          }
          // Else: do nothing, keep screen blank
        }
        // Optimization: If static content fits and NO_EFFECT OUT, don't reset
        // (blink)
        else if ((zones[z].workMode == "mqttClient" ||
                  zones[z].workMode == "manualInput") &&
                 zones[z].textFits &&
                 zones[z].scrollEffectOut == "PA_NO_EFFECT") {
          // Do nothing, keep static
        } else {
          P.displayReset(z);
        }
      }
    }
  }
}

String haApiGet(String sensorId, String sensorPostfix) {
  Serial.printf("\nHA updating...");
  bool https;
  if (haApiHttpType == "https")
    https = true;
  if (haApiHttpType == "http")
    https = false;
  JsonObject haApiRespondPostObj =
      httpsRequest(haAddr, haApiPort, "/api/states/" + sensorId,
                   "Bearer " + haApiToken, https);
  return haApiRespondPostObj[F("state")].as<String>() + sensorPostfix;
}

void setup() {
  readAllConfig();
  Serial.begin(115200);
  Serial.print(F("Start serial...."));

  readAllConfig();

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

  // Serial.println("autoConnect finished");

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
  } // <--- Close else block for wifi success

  // Watchdog timer disabled - causes crashes without proper reset calls in
  // loop To re-enable, add esp_task_wdt_reset() in main loop #if
  // defined(ESP32)
  //   esp_task_wdt_init(30, true);
  //   esp_task_wdt_add(NULL);
  //   Serial.println(F("Watchdog timer initialized (30s)"));
  // #endif

  // Start MQTT client (buffer reduced from 4096 to 1536 for memory
  // efficiency)
  if (mqttEnable) {
    mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
    mqttClient.setCallback(MQTTCallback);
    mqttClient.setBufferSize(1536);
  }

  // start NTP client
  timeClient.begin();
  timeClient.setTimeOffset(ntpTimeZone * 3600);
  timeClient.setUpdateInterval(ntpUpdateInterval * 3600);
  timeClient.setPoolServerName(ntpServer.c_str());
  ntpUpdateTime();

  // Start Dallas ds18b20
  // if (ds18b20Enable) {
  //  oneWire.reset();
  //  oneWire.search(addrS1); //getting the address of Sesnor-1
  //}

  // Web server routings
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
    }

    serializeJson(doc, *response);
    request->send(response);
  });

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

  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    DynamicJsonDocument doc(
        3072); // Allocate sufficient memory for all settings

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
    }

    serializeJson(doc, *response);
    request->send(response);
  });

  // OTA firmware upload handler - chunked upload support
  // Uses globalOtaInProgress and otaTotalSize defined at top of file

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
          // Serial.printf("OTA Update Start: %s, total size: %u\n",
          // filename.c_str(), totalSize);
          globalOtaInProgress = true;
          otaTotalSize = 0;

      // Note: Display/MQTT removed to prevent conflicts

      // Begin update with total file size if known, otherwise unknown
      // Note: Display updates removed to prevent SPI conflicts/hangs
#ifdef ESP8266
          Update.runAsync(true);
#endif
          size_t updateSize = totalSize > 0 ? totalSize : UPDATE_SIZE_UNKNOWN;
          if (!Update.begin(updateSize, U_FLASH)) {
            // Update.printError(Serial);
            globalOtaInProgress = false;
            return;
          }
        }

        // Write data
        if (len > 0 && globalOtaInProgress) {
          if (Update.write(data, len) != len) {
            // Update.printError(Serial);
            globalOtaInProgress = false;
            return;
          }
          otaTotalSize += len;
        }
      });

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
                P.displayReset(n); // Reset to ensure no animation state lingers
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
                zones[n].workMode == "wopr") {
              if (zones[n].workMode == "wallClock") {
                strcpy(zoneMessages[n], ""); // Clear active text
                P.displayClear(n);
                P.displayReset(n);
              }
              zones[n].newMessageAvailable = false;
              zoneUpdate[n] = true;
            } else {
              textEffect_t effOut =
                  stringToTextEffectT(zones[n].scrollEffectOut);
              P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn),
                              effOut);
              P.displayReset(n);
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
                zones[n].font = p->value().c_str();
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
        if (p->name() == "ntpServer")
          ntpServer = p->value().c_str();

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
        // if (zones[n].workMode == "wallClock")
        //   zoneShowModeMessage(n, "Clock");
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
      if (key->value() == "wallClockSett")
        ntpUpdateTime();
      // Reinforce power state after zone settings to prevent display
      // turning off
      if (key->value() == "zoneSettings" && power) {
        P.displayShutdown(0);
      }
      if (key->value() == "displaySettings") {
        restartESP = true;
      }
      // readConfig(key->value(), n);
    }
  });

  // Factory reset endpoint - clears all saved preferences
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
void updateWOPREffect(uint8_t zone) {
  if (!woprZones[zone].active)
    return;

  if (currentMillis - woprZones[zone].lastUpdate >=
      woprZones[zone].updateInterval) {
    woprZones[zone].lastUpdate = currentMillis;

    MD_MAX72XX *mx = P.getGraphicObject();
    if (!mx)
      return;

    // Calculate the column range for this zone across ALL modules
    // Each module is 8 columns wide (COL_SIZE = 8)
    int startCol = zones[zone].begin * 8;
    int endCol = (zones[zone].end * 8) + 7;

    // Generate random blinking pattern using their working method
    for (int col = startCol; col <= endCol; col++) {
      if (random(0, 100) < 30) { // 30% chance to update this column
        for (int row = 0; row < 8; row++) {
          // Create random patterns
          int pattern = random(0, 4);
          bool state = false;

          switch (pattern) {
          case 0:                     // Full random - their method
            state = random(100) < 65; // 65% chance ON like theirs
            break;
          case 1: // Horizontal sweep
            state = (currentMillis / 100 + col) % 2;
            break;
          case 2: // Vertical sweep
            state = (currentMillis / 100 + row) % 2;
            break;
          case 3: // Checkerboard flicker
            state = ((row + col + (currentMillis / 200)) % 2);
            break;
          case 4: // Random intensity blocks
            if (random(0, 100) < 20) {
              state = true;
              mx->setPoint(row, col, state);
              for (int i = 1; i < 3 && row + i < 8; i++) {
                mx->setPoint(row + i, col, state);
              }
              continue;
            }
            break;
          }
          mx->setPoint(row, col, state);
        }
      }
    }

    // Occasionally flash entire zone
    if (random(0, 100) < 5) {
      for (int col = startCol; col <= endCol; col++) {
        for (int row = 0; row < 8; row++) {
          mx->setPoint(row, col, true);
        }
      }
    }
  }
}

void testZones(uint8_t n) {
  P.displayClear();
  P.setFont(n, wledSymbolFont);
  P.setCharSpacing(n, 0);
  P.setPause(n, 500);
  P.setSpeed(n, 25);
  if (n < zoneNumbers) {
    P.setTextEffect(n, PA_GROW_UP, PA_NO_EFFECT);
    String testMessage = "8";
    for (int i = 0; i < int(zones[n].end - zones[n].begin); i++)
      testMessage += 8;
    zoneNewMessage(n, testMessage, "", false);
  }
  if (n == zoneNumbers) {
    P.setCharSpacing(0, 1);
    P.setFont(0, wledFont_cyrillic);
    P.setSpeed(0, 50);
    P.setTextEffect(0, PA_SCROLL_LEFT, PA_NO_EFFECT);
    zoneNewMessage(0, "ip: " + WiFi.localIP().toString(), "", false);
  }
}

void loop() {
  for (uint8_t n = 0; n < zoneNumbers; n++) {
    if (zones[n].updateFont) {
      P.displayClear(n);
      applyZoneFont(n, zones[n].font);
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

  // handle a restart ESP request
  if (restartESP) {
    Serial.print(F("Rebooting ESP..."));
    delay(1000);
    ESP.restart();
  }

  // init display animation
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

  // apply saved config for display
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

      // Clear boot messages (like IP) to prevent them from looping in
      // MQTT mode
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

  // Handle deferred config save
  if (configDirty && (millis() - configDirtyTime > 2000)) {
    Serial.println(F("Saving deferred configuration..."));
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      saveVarsToConfFile("zoneSettings", n);
    }
    configDirty = false;
  }

  if (allTestsFinish) {
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
          mqttClient.setServer(mqttServerAddress.c_str(),
                               mqttServerPort); // Update server params
          lastReconnectAttempt = 0;             // Trigger immediate reconnect
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
    for (uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "owmWeather" && owmApiToken.length() > 0 &&
          owmCity.length() > 0)
        owmWeatherEnable = true;
    }
    if (owmWeatherEnable &&
        currentMillis - owmLastTime >= (unsigned)owmUpdateInterval * 1000)
      owmWeatherUpdate(owmCity, owmUnitsFormat, owmApiToken);

    // zones rutines
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
          // Skip blink logic if dots blink is disabled OR if text doesn't fit
          // (scrolling)
          if (!disableDotsBlink && zones[n].textFits) {
            if (currentMillis - zones[n].previousMillis >= 1000 ||
                zones[n].forceUpdate) {
              if ((zones[n].curTime == curTimeNew ||
                   zones[n].curTime == flashClockDots(curTimeNew)) &&
                  !zones[n].forceUpdate) {
                // Same minute - blink logic
                if (!P.getZoneStatus(n))
                  continue; // Skip if animation running

                zones[n].previousMillis = currentMillis;
                if (zones[n].curTime.indexOf(":") > 0)
                  curTimeNew.replace(":", "¦");
                P.setPause(n, 100);
                P.setTextEffect(n, PA_NO_EFFECT, PA_NO_EFFECT);
                zones[n].curTime = curTimeNew;
                zoneNewMessage(n, zones[n].curTime, "", false);
              } else {
                // Minute changed OR Forced Update
                zones[n].previousMillis = currentMillis;
                zones[n].curTime = curTimeNew;
                P.setPause(n, 100);
                P.setTextEffect(
                    n,
                    zones[n].forceUpdate
                        ? PA_PRINT
                        : stringToTextEffectT(zones[n].scrollEffectIn),
                    PA_NO_EFFECT);
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

      // OWM
      if (zones[n].workMode == "owmWeather") {
        if (owmWeatherEnable) {
          if (currentMillis - zones[n].previousMillis >=
              (unsigned)owmUpdateInterval * 1000) {
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

    if (owmWeatherEnable && owmPrinted) {
      owmLastTime = currentMillis;
      owmPrinted = false;
    }
  }

  displayAnimation();
}