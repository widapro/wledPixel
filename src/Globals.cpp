#include "Globals.h"

// ─── Core Objects
// ──────────────────────────────────────────────────────────────
Preferences preferences;
WiFiClient mqttEspClient;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
uint8_t MAX_DEVICES = 16;
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
PubSubClient mqttClient(mqttEspClient);
AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager *wifiManagerPtr = nullptr;
WiFiUDP ntpUDP;

// ─── Global Flags
// ──────────────────────────────────────────────────────────────
bool globalOtaInProgress = false;
bool otaPlatformMismatch = false;
bool otaPlatformOk = true;
uint8_t otaScanOverlap[32] = {0};
size_t otaScanOverlapLen = 0;
bool shouldReboot = false;
bool shouldUpdateNtp = false;
size_t otaTotalSize = 0;
String restoreJsonBuffer = "";
const char *firmwareVer = "v3.5";
// Platform marker embedded in binary — used for OTA platform validation
// DO NOT REMOVE: this string is scanned in the binary during OTA upload
#if defined(ESP8266)
const char PLATFORM_MARKER[] __attribute__((used)) =
    "WLEDPIXEL_PLATFORM_ESP8266";
#elif defined(ESP32)
const char PLATFORM_MARKER[] __attribute__((used)) = "WLEDPIXEL_PLATFORM_ESP32";
#endif
int nLoop = 0;
bool restartESP = false;
bool allTestsFinish = false;
bool initConfig = false;
char const *wifiAPpassword = "12345678";
unsigned long currentMillis;
unsigned long previousMillis = 0;
bool shouldSaveConfig = false;

// ─── DS18B20
// ───────────────────────────────────────────────────────────────────
bool ds18b20Enable = false;
uint16_t ds18b20UpdateInterval = 30;
unsigned long previousDsMillis = -100000;
String ds18b20UnitsFormat = "Celsius";
String dsTemp = "-127";
char dsTempBuff[5];
bool dsTempToDisplay = false;

// ─── WOPR
// ──────────────────────────────────────────────────────────────────────
WOPRData woprZones[4] = {{false, 0, 100, 7, {0}},
                         {false, 0, 100, 7, {0}},
                         {false, 0, 100, 7, {0}},
                         {false, 0, 100, 7, {0}}};

// ─── MQTT
// ──────────────────────────────────────────────────────────────────────
bool mqttEnable = false;
String mqttServerAddress = "";
uint16_t mqttServerPort = 1883;
String mqttUsername = "";
String mqttPassword = "";
long lastReconnectAttempt = 0;

String shortMACaddr =
    WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15);
String MQTTGlobalPrefix = "wledPixel-" + shortMACaddr;

MQTTZoneData MQTTZones[] = {
    {MQTTGlobalPrefix + "/zone0/text",
     MQTTGlobalPrefix + "/zone0/scrolleffectIn",
     MQTTGlobalPrefix + "/zone0/scrolleffectOut",
     MQTTGlobalPrefix + "/zone0/scrollspeed",
     MQTTGlobalPrefix + "/zone0/scrollpause",
     MQTTGlobalPrefix + "/zone0/scrollalign",
     MQTTGlobalPrefix + "/zone0/charspacing",
     MQTTGlobalPrefix + "/zone0/workmode",
     MQTTGlobalPrefix + "/zone0/scrollInfinite",
     MQTTGlobalPrefix + "/zone0/countdown",
     MQTTGlobalPrefix + "/zone0/countdownFormat"},
    {MQTTGlobalPrefix + "/zone1/text",
     MQTTGlobalPrefix + "/zone1/scrolleffectIn",
     MQTTGlobalPrefix + "/zone1/scrolleffectOut",
     MQTTGlobalPrefix + "/zone1/scrollspeed",
     MQTTGlobalPrefix + "/zone1/scrollpause",
     MQTTGlobalPrefix + "/zone1/scrollalign",
     MQTTGlobalPrefix + "/zone1/charspacing",
     MQTTGlobalPrefix + "/zone1/workmode",
     MQTTGlobalPrefix + "/zone1/scrollInfinite",
     MQTTGlobalPrefix + "/zone1/countdown",
     MQTTGlobalPrefix + "/zone1/countdownFormat"},
    {MQTTGlobalPrefix + "/zone2/text",
     MQTTGlobalPrefix + "/zone2/scrolleffectIn",
     MQTTGlobalPrefix + "/zone2/scrolleffectOut",
     MQTTGlobalPrefix + "/zone2/scrollspeed",
     MQTTGlobalPrefix + "/zone2/scrollpause",
     MQTTGlobalPrefix + "/zone2/scrollalign",
     MQTTGlobalPrefix + "/zone2/charspacing",
     MQTTGlobalPrefix + "/zone2/workmode",
     MQTTGlobalPrefix + "/zone2/scrollInfinite",
     MQTTGlobalPrefix + "/zone2/countdown",
     MQTTGlobalPrefix + "/zone2/countdownFormat"},
    {MQTTGlobalPrefix + "/zone3/text",
     MQTTGlobalPrefix + "/zone3/scrolleffectIn",
     MQTTGlobalPrefix + "/zone3/scrolleffectOut",
     MQTTGlobalPrefix + "/zone3/scrollspeed",
     MQTTGlobalPrefix + "/zone3/scrollpause",
     MQTTGlobalPrefix + "/zone3/scrollalign",
     MQTTGlobalPrefix + "/zone3/charspacing",
     MQTTGlobalPrefix + "/zone3/workmode",
     MQTTGlobalPrefix + "/zone3/scrollInfinite",
     MQTTGlobalPrefix + "/zone3/countdown",
     MQTTGlobalPrefix + "/zone3/countdownFormat"},
};
String MQTTIntensity = MQTTGlobalPrefix + "/intensity";
String MQTTPower = MQTTGlobalPrefix + "/power";
String MQTTStateTopic = MQTTGlobalPrefix + "/state";
bool mqttPublished = false;
bool shouldMqttPublish = false;
bool shouldMqttDisconnect = false;
bool zoneServiceMessageScrolling[4] = {false, false, false, false};
unsigned long zoneScrollCompleteTime[4] = {0, 0, 0, 0};

// ─── NTP
// ───────────────────────────────────────────────────────────────────────
String ntpServer = "us.pool.ntp.org";
float ntpTimeZone = 3.0;
unsigned long previousNTPsyncMillis = millis();
uint16_t ntpUpdateInterval = 6;
NTPClient timeClient(ntpUDP, ntpServer.c_str(), ntpTimeZone * 3600,
                     ntpUpdateInterval * 3600);

// ─── Zones
// ─────────────────────────────────────────────────────────────────────
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
     "DDHHMMSS",
     "blinkForever",
     "",
     "",
     true,
     "",
     "namePrice",
     "",
     "",
     true,
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
     "DDHHMMSS",
     "blinkForever",
     "",
     "",
     true,
     "",
     "namePrice",
     "",
     "",
     true,
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
     "DDHHMMSS",
     "blinkForever",
     "",
     "",
     true,
     "",
     "namePrice",
     "",
     "",
     true,
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
     "DDHHMMSS",
     "blinkForever",
     "",
     "",
     true,
     "",
     "namePrice",
     "",
     "",
     true,
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
char zoneMessages[4][100] = {"zone0", "zone1", "zone2", "zone3"};
char zoneMessagesPending[4][100] = {"", "", "", ""};

// ─── OWM
// ───────────────────────────────────────────────────────────────────────
unsigned long owmLastTime = 0;
uint16_t owmUpdateInterval = 60;
String owmUnitsFormat = "metric";
bool owmWeatherEnable = false;
String owmApiToken;
String owmCity;

// ─── HA
// ────────────────────────────────────────────────────────────────────────
String haAddr, haApiHttpType, haApiToken;
uint16_t haUpdateInterval = 60;
uint16_t haApiPort = 8123;
uint32_t haLastTime = 0;

// ─── Stock Ticker
// ───────────────────────────────────────────────────────────────────
String stockApiToken;
uint16_t stockUpdateInterval = 60;
unsigned long stockLastTime = 0;
bool stockEnable = false;
