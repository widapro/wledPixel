#include <Arduino.h>

#if defined(ESP8266)
  #pragma message "Compiling for ESP8266 board"
  #include <ESPAsyncTCP.h>
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
  #pragma message "Compiling for ESP32 board"
  #include <AsyncTCP.h>
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoECCX08.h>
  #include <WiFiClientSecure.h>
#else
  #error "This ain't a ESP8266 or ESP32!"
#endif


#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ArduinoJson.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <WiFiClientSecure.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include <AsyncElegantOTA.h>

#include "indexPage.h"
#include "settingsPage.h"
#include "owm.h"
#include "wledFont.h"


Preferences preferences;
WiFiClient mqttEspClient;

/// PINOUT ///
#if defined(ESP8266)
  const int oneWireBus = D4;                // WeMos D1 mini GPIO02
  #define DATA_PIN  D7                      // WeMos D1 mini GPIO13
  #define CS_PIN    D6                      // WeMos D1 mini GPIO12
  #define CLK_PIN   D5                      // WeMos D1 mini GPIO14
#elif defined(ESP32)
  const int oneWireBus = 4;                 // ESP32 GPIO04
  #define DATA_PIN       23                 // ESP32 GPIO23
  #define CS_PIN         5                  // ESP32 GPIO5
  #define CLK_PIN        18                 // ESP32 GPIO18
  #define VBAT_PIN 36
  float battv = 0;
  unsigned long previousBatVoltMillis = 0;
  uint16_t BatVoltUpdateInterval = 30; // in seconds
#else
  #error "This ain't a ESP8266 or ESP32!"
#endif

/// GLOBAL ///
const char* firmwareVer = "2.7";
int nLoop = 0;
bool restartESP         = false;
bool allTestsFinish     = false;
bool initConfig = false;
char const *wifiAPpassword  = "12345678";
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


//// MQTT settings ////
bool mqttEnable           = false;
String mqttServerAddress  = "";
uint16_t mqttServerPort   = 1883;
String mqttUsername       = "";
String mqttPassword       = "";
long lastReconnectAttempt = 0;
PubSubClient mqttClient(mqttEspClient);

String shortMACaddr = WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15); // last five chars of the MAC, ie: C0A4;
//String deviceName = shortMACaddr;
String MQTTGlobalPrefix = "wledPixel-" + shortMACaddr;

typedef struct {
  String    message, scrollEffectIn, scrollEffectOut, scrollSpeed, scrollPause, scrollAllign, charspacing, workMode;
} MQTTZoneData;

MQTTZoneData MQTTZones[] = {
  {MQTTGlobalPrefix + "/zone0/text", MQTTGlobalPrefix + "/zone0/scrolleffectIn", MQTTGlobalPrefix + "/zone0/scrolleffectOut", MQTTGlobalPrefix + "/zone0/scrollspeed", MQTTGlobalPrefix + "/zone0/scrollpause", MQTTGlobalPrefix + "/zone0/scrollalign", MQTTGlobalPrefix + "/zone0/charspacing", MQTTGlobalPrefix + "/zone0/workmode"},
  {MQTTGlobalPrefix + "/zone1/text", MQTTGlobalPrefix + "/zone1/scrolleffectIn", MQTTGlobalPrefix + "/zone1/scrolleffectOut", MQTTGlobalPrefix + "/zone1/scrollspeed", MQTTGlobalPrefix + "/zone1/scrollpause", MQTTGlobalPrefix + "/zone1/scrollalign", MQTTGlobalPrefix + "/zone1/charspacing", MQTTGlobalPrefix + "/zone1/workmode"},
  {MQTTGlobalPrefix + "/zone2/text", MQTTGlobalPrefix + "/zone2/scrolleffectIn", MQTTGlobalPrefix + "/zone2/scrolleffectOut", MQTTGlobalPrefix + "/zone2/scrollspeed", MQTTGlobalPrefix + "/zone2/scrollpause", MQTTGlobalPrefix + "/zone2/scrollalign", MQTTGlobalPrefix + "/zone2/charspacing", MQTTGlobalPrefix + "/zone2/workmode"},
//  {MQTTGlobalPrefix + "/zone3/text", MQTTGlobalPrefix + "/zone3/scrolleffectIn", MQTTGlobalPrefix + "/zone3/scrolleffectOut", MQTTGlobalPrefix + "/zone3/scrollspeed", MQTTGlobalPrefix + "/zone3/scrollpause", MQTTGlobalPrefix + "/zone3/scrollalign", MQTTGlobalPrefix + "/zone3/charspacing", MQTTGlobalPrefix + "/zone3/workmode"},
};
String MQTTIntensity  = MQTTGlobalPrefix + "/intensity";
String MQTTPower      = MQTTGlobalPrefix + "/power";
String MQTTStateTopic = MQTTGlobalPrefix + "/state";
bool mqttPublished    = false;


// Display config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // type of device hardware https://majicdesigns.github.io/MD_MAX72XX/page_hardware.html
//// Display pinout

uint8_t MAX_DEVICES = 16;                 // number of device segments
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

typedef struct {
    uint8_t       begin, end;
    uint8_t       scrollSpeed, charspacing;
    uint8_t       scrollPause;
    unsigned long previousMillis;
    String        scrollAlign, scrollEffectIn, scrollEffectOut, font, workMode, clockDisplayFormat, haSensorId, haSensorPostfix, owmWhatToDisplay, mqttPostfix, ds18b20Postfix, curTime;
    bool          newMessageAvailable;
} ZoneData;

// structure:
ZoneData zones[] = {
  {0, 3, 35, 1, 3, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont_cyrillic", "manualInput", "HHMM", "", "", "owmTemperature", "", "", "", false},
  {4, 5, 35, 1, 3, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont_cyrillic", "manualInput", "HHMM", "", "", "owmTemperature", "", "", "", false},
  {6, 7, 35, 1, 3, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont_cyrillic", "manualInput", "HHMM", "", "", "owmTemperature", "", "", "", false},
//  {7, 7, 35, 1, 3000, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature", "", "", ""},
};

uint8_t zoneNumbers         = 1;
uint8_t intensity           = 7;
bool power                  = true;
bool disableServiceMessages = false;
bool disableDotsBlink       = false;

char zone0Message[100] = "zone0";
char zone1Message[100] = "zone1";
char zone2Message[100] = "zone2";
//char zone3Message[50] = "zone3";

// Initialize NTP
String ntpServer = "us.pool.ntp.org";
int8_t ntpTimeZone = 3;
WiFiUDP ntpUDP;
unsigned long previousNTPsyncMillis = millis();
uint16_t ntpUpdateInterval = 6; // in hours
NTPClient timeClient(ntpUDP, ntpServer.c_str(), ntpTimeZone * 3600, ntpUpdateInterval * 3600);

AsyncWebServer server(80);
DNSServer dns;
//AsyncWiFiManager wifiManager(&server,&dns);

//flag for saving wifi data
bool shouldSaveConfig   = false;
//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig      = true;
}

// Open Weather Map client
int             owmLastTime       = 0;
uint16_t        owmUpdateInterval = 60;
String          owmUnitsFormat    = "metric";
bool            owmWeatherEnable  = false;
bool            owmPrinted        = true;
String          owmApiToken;
String          owmCity;

// Home Assistant client
String          haAddr, haApiHttpType, haApiToken;
uint16_t        haUpdateInterval  = 60;
uint16_t        haApiPort         = 8123;
static uint32_t haLastTime        = 0;


// Convert scrollAlig in String to textPosition_t type
textPosition_t stringToTextPositionT(String val) {
  if(val == F("PA_CENTER"))  return PA_CENTER;
  if(val == F("PA_LEFT"))    return PA_LEFT;
  if(val == F("PA_RIGHT"))   return PA_RIGHT;
  return PA_LEFT;
}

void applyZoneFont(int zone, String font) {
  if(font == F("default"))           P.setFont(zone, nullptr);
  if(font == F("wledFont"))          P.setFont(zone, wledFont);
  if(font == F("wledFont_cyrillic")) P.setFont(zone, wledFont_cyrillic);
  if(font == F("wledSymbolFont"))    P.setFont(zone, wledSymbolFont);
}

textEffect_t stringToTextEffectT(String val) {
  if(val == F("PA_NO_EFFECT"))         return PA_NO_EFFECT;
  if(val == F("PA_PRINT"))             return PA_PRINT;
  if(val == F("PA_SCROLL_UP"))         return PA_SCROLL_UP;
  if(val == F("PA_SCROLL_DOWN"))       return PA_SCROLL_DOWN;
  if(val == F("PA_SCROLL_LEFT"))       return PA_SCROLL_LEFT;
  if(val == F("PA_SCROLL_RIGHT"))      return PA_SCROLL_RIGHT;
  if(val == F("PA_SPRITE"))            return PA_SPRITE;
  if(val == F("PA_SLICE"))             return PA_SLICE;
  if(val == F("PA_MESH"))              return PA_MESH;
  if(val == F("PA_FADE"))              return PA_FADE;
  if(val == F("PA_DISSOLVE"))          return PA_DISSOLVE;
  if(val == F("PA_BLINDS"))            return PA_BLINDS;
  if(val == F("PA_RANDOM"))            return PA_RANDOM;
  if(val == F("PA_WIPE"))              return PA_WIPE;
  if(val == F("PA_WIPE_CURSOR"))       return PA_WIPE_CURSOR;
  if(val == F("PA_SCAN_HORIZ"))        return PA_SCAN_HORIZ;
  if(val == F("PA_SCAN_HORIZX"))       return PA_SCAN_HORIZX;
  if(val == F("PA_SCAN_VERT"))         return PA_SCAN_VERT;
  if(val == F("PA_SCAN_VERTX"))        return PA_SCAN_VERTX;
  if(val == F("PA_OPENING"))           return PA_OPENING;
  if(val == F("PA_OPENING_CURSOR"))    return PA_OPENING_CURSOR;
  if(val == F("PA_CLOSING"))           return PA_CLOSING;
  if(val == F("PA_CLOSING_CURSOR"))    return PA_CLOSING_CURSOR;
  if(val == F("PA_SCROLL_UP_LEFT"))    return PA_SCROLL_UP_LEFT;
  if(val == F("PA_SCROLL_UP_RIGHT"))   return PA_SCROLL_UP_RIGHT;
  if(val == F("PA_SCROLL_DOWN_LEFT"))  return PA_SCROLL_DOWN_LEFT;
  if(val == F("PA_SCROLL_DOWN_RIGHT")) return PA_SCROLL_DOWN_RIGHT;
  if(val == F("PA_GROW_UP"))           return PA_GROW_UP;
  if(val == F("PA_GROW_DOWN"))         return PA_GROW_DOWN;
  return PA_SCROLL_LEFT;
}

// Return values for variables in html page
String processor(const String& var){
  char buffer [10];
  if(var == F("workModeZone0"))                return zones[0].workMode;
  if(var == F("workModeZone1"))                return zones[1].workMode;
  if(var == F("workModeZone2"))                return zones[2].workMode;
//  if(var == "workModeZone3")                return zones[3].workMode;
  if(var == F("zoneNumbers"))                  return itoa(zoneNumbers, buffer, 10);
  if(var == F("zone0Begin"))                   return itoa(zones[0].begin, buffer, 10);
  if(var == F("zone0End"))                     return itoa(zones[0].end, buffer, 10);
  if(var == F("zone1Begin"))                   return itoa(zones[1].begin, buffer, 10);
  if(var == F("zone1End"))                     return itoa(zones[1].end, buffer, 10);
  if(var == F("zone2Begin"))                   return itoa(zones[2].begin, buffer, 10);
  if(var == F("zone2End"))                     return itoa(zones[2].end, buffer, 10);
//  if(var == "zone3Begin")                   return itoa(zones[3].begin, buffer, 10);
//  if(var == "zone3End")                     return itoa(zones[3].end, buffer, 10);
  if(var == F("wifiSsid"))                     return WiFi.SSID();
  if(var == F("wifiIp"))                       return WiFi.localIP().toString();
  if(var == F("wifiGateway"))                  return WiFi.gatewayIP().toString();
  if(var == F("intensity"))                    return itoa(intensity + 1, buffer, 10);
  if(var == F("scrollSpeedZone0"))             return itoa(zones[0].scrollSpeed, buffer, 10);
  if(var == F("scrollSpeedZone1"))             return itoa(zones[1].scrollSpeed, buffer, 10);
  if(var == F("scrollSpeedZone2"))             return itoa(zones[2].scrollSpeed, buffer, 10);
//  if(var == "scrollSpeedZone3")             return itoa(zones[3].scrollSpeed, buffer, 10);
  if(var == F("scrollPauseZone0"))             return itoa(zones[0].scrollPause, buffer, 10);
  if(var == F("scrollPauseZone1"))             return itoa(zones[1].scrollPause, buffer, 10);
  if(var == F("scrollPauseZone2"))             return itoa(zones[2].scrollPause, buffer, 10);
//  if(var == "scrollPauseZone3")             return itoa(zones[3].scrollPause, buffer, 10);
  if(var == F("scrollAlignZone0"))             return zones[0].scrollAlign;
  if(var == F("scrollAlignZone1"))             return zones[1].scrollAlign;
  if(var == F("scrollAlignZone2"))             return zones[2].scrollAlign;
//  if(var == "scrollAlignZone3")             return zones[3].scrollAlign;
  if(var == F("scrollEffectZone0In"))          return zones[0].scrollEffectIn;
  if(var == F("scrollEffectZone1In"))          return zones[1].scrollEffectIn;
  if(var == F("scrollEffectZone2In"))          return zones[2].scrollEffectIn;
//  if(var == "scrollEffectZone3In")          return zones[3].scrollEffectIn;
  if(var == F("scrollEffectZone0Out"))         return zones[0].scrollEffectOut;
  if(var == F("scrollEffectZone1Out"))         return zones[1].scrollEffectOut;
  if(var == F("scrollEffectZone2Out"))         return zones[2].scrollEffectOut;
//  if(var == "scrollEffectZone3Out")         return zones[3].scrollEffectOut;
  if(var == F("mqttEnable")) {
    if (mqttEnable) return "true";
    if (!mqttEnable) return "false";
  }
  if(var == F("mqttServerAddress"))            return mqttServerAddress;
  if(var == F("mqttServerPort"))               return itoa(mqttServerPort, buffer, 10);
  if(var == F("mqttUsername"))                 return mqttUsername;
  if(var == F("mqttPassword"))                 return mqttPassword;
  if(var == F("mqttDevicePrefix"))             return MQTTGlobalPrefix;
  if(var == F("mqttTextTopicZone0"))           return MQTTZones[0].message;
  if(var == F("mqttTextTopicZone1"))           return MQTTZones[1].message;
  if(var == F("mqttTextTopicZone2"))           return MQTTZones[2].message;
//  if(var == "mqttTextTopicZone3")           return MQTTZones[3].message;
  if(var == F("mqttPostfixZone0"))             return zones[0].mqttPostfix;
  if(var == F("mqttPostfixZone1"))             return zones[1].mqttPostfix;
  if(var == F("mqttPostfixZone2"))             return zones[2].mqttPostfix;
//  if(var == "mqttPostfixZone3")             return zones[3].mqttPostfix;
  if(var == F("ntpTimeZone"))                  return itoa(ntpTimeZone, buffer, 10);
  if(var == F("ntpUpdateInterval"))            return itoa(ntpUpdateInterval, buffer, 10);
  if(var == F("ntpServer"))                    return ntpServer;
  if(var == F("clockDisplayFormatZone0"))      return zones[0].clockDisplayFormat;
  if(var == F("clockDisplayFormatZone1"))      return zones[1].clockDisplayFormat;
  if(var == F("clockDisplayFormatZone2"))      return zones[2].clockDisplayFormat;
//  if(var == "clockDisplayFormatZone3")      return zones[3].clockDisplayFormat;
  if(var == F("owmApiToken"))                  return owmApiToken;
  if(var == F("owmUnitsFormat"))               return owmUnitsFormat;
  if(var == F("owmUpdateInterval"))            return itoa(owmUpdateInterval, buffer, 10);
  if(var == F("owmWhatToDisplayZone0"))        return zones[0].owmWhatToDisplay;
  if(var == F("owmWhatToDisplayZone1"))        return zones[1].owmWhatToDisplay;
  if(var == F("owmWhatToDisplayZone2"))        return zones[2].owmWhatToDisplay;
//  if(var == "owmWhatToDisplayZone3")        return zones[3].owmWhatToDisplay;
  if(var == F("owmCity"))                      return owmCity;
  if(var == F("fontZone0"))                    return zones[0].font;
  if(var == F("fontZone1"))                    return zones[1].font;
  if(var == F("fontZone2"))                    return zones[2].font;
//  if(var == "fontZone3")                    return zones[3].font;
  if(var == F("haAddr"))                       return haAddr;
  if(var == F("haApiToken"))                   return haApiToken;
  if(var == F("haApiHttpType"))                return haApiHttpType;
  if(var == F("haApiPort"))                    return itoa(haApiPort, buffer, 10);
  if(var == F("haSensorIdZone0"))              return zones[0].haSensorId;
  if(var == F("haSensorIdZone1"))              return zones[1].haSensorId;
  if(var == F("haSensorIdZone2"))              return zones[2].haSensorId;
//  if(var == "haSensorIdZone3")              return zones[3].haSensorId;
  if(var == F("haSensorPostfixZone0"))         return zones[0].haSensorPostfix;
  if(var == F("haSensorPostfixZone1"))         return zones[1].haSensorPostfix;
  if(var == F("haSensorPostfixZone2"))         return zones[2].haSensorPostfix;
//  if(var == "haSensorPostfixZone3")         return zones[3].haSensorPostfix;
  if(var == F("haUpdateInterval"))             return itoa(haUpdateInterval, buffer, 10);
  if(var == F("charspacingZone0"))             return itoa(zones[0].charspacing, buffer, 10);
  if(var == F("charspacingZone1"))             return itoa(zones[1].charspacing, buffer, 10);
  if(var == F("charspacingZone2"))             return itoa(zones[2].charspacing, buffer, 10);
//  if(var == "charspacingZone3")             return itoa(zones[3].charspacing, buffer, 10);
  if(var == F("firmwareVer"))                  return firmwareVer;
  if(var == F("deviceName"))                   return shortMACaddr;
  if(var == F("disableServiceMessages"))       return String(disableServiceMessages);
  if(var == F("disableDotsBlink"))             return String(disableDotsBlink);
  if(var == F("ds18b20Enable"))                return String(ds18b20Enable);
  if(var == F("ds18b20Temp"))                  return String(dsTemp);
  if(var == F("ds18b20UpdateInterval"))        return itoa(ds18b20UpdateInterval, buffer, 10);
  if(var == F("ds18b20UnitsFormat"))           return ds18b20UnitsFormat;
  if(var == F("ds18b20PostfixZone0"))          return zones[0].ds18b20Postfix;
  if(var == F("ds18b20PostfixZone1"))          return zones[1].ds18b20Postfix;
  if(var == F("ds18b20PostfixZone2"))          return zones[2].ds18b20Postfix;
//  if(var == "ds18b20PostfixZone3")          return zones[3].ds18b20Postfix;
  return String();
}

void MQTTPublishHADiscovry(String zone, String device_type) {
  if (mqttClient.connected()) {
    DynamicJsonDocument  root(1072);
    char topic_config[120];
    char buffer[1072];
    boolean result;

    // device block
    JsonObject device                 = root.createNestedObject("dev");
    JsonArray arrDevice               = device.createNestedArray("ids"); 
    arrDevice.add(MQTTGlobalPrefix); 
    device["mf"]                      = "widapro";
    device["mdl"]                     = "wledPixel-v2"; 
    device["name"]                    = "wledPixel" + shortMACaddr;
    device["sw"]                      = firmwareVer;

    // availability block
    JsonArray arrAvailability         = root.createNestedArray("availability");
    JsonObject availability           = arrAvailability.createNestedObject();
    availability["topic"]             = MQTTGlobalPrefix + "/state";
    availability["value_template"]    = "{{ value_json.status }}";

    // light block
    if (device_type == "light") {
      sprintf( topic_config,  "homeassistant/light/%s/config", shortMACaddr.c_str() );
      root["name"]         = MQTTGlobalPrefix;
      root["uniq_id"]      = "wledPixelLight-" + shortMACaddr;
      root["cmd_t"]        = "wledPixel-" + shortMACaddr + "/power";
      root["pl_off"]       = "OFF";
      root["pl_on"]        = "ON";
      root["brightness"]   = "true";
      root["bri_stat_t"]   = "wledPixel-" + shortMACaddr + "/state";
      root["bri_val_tpl"]  = "{{ value_json['brightness'] }}";
      root["bri_cmd_t"]    = "wledPixel-" + shortMACaddr + "/intensity";
      root["bri_scl"]      = 17;
      root["stat_t"]       = "wledPixel-" + shortMACaddr + "/state";
      root["stat_val_tpl"] = "{{ value_json.power }}";
      root["json_attr_t"]  = "wledPixel-" + shortMACaddr + "/state";
      root["qos"]          = 1;
      root["opt"]          = true;
      root["ret"]          = true;
      root["ic"]           = "mdi:view-comfy";
    }

    if (device_type == "workMode") {
      sprintf( topic_config,  "homeassistant/select/%s/zone%sWorkMode/config", shortMACaddr.c_str(), zone.c_str() );
      root["name"]            = "Zone" + zone + " work mode";
      root["uniq_id"]         = "wledPixelLight" + shortMACaddr + "Zone" + zone + "WorkMode";
      root["cmd_t"]           = "wledPixel-" + shortMACaddr + "/zone" + zone + "/workmode";
      root["stat_t"]          = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"]         = "{{ value_json['zone" + zone + "']['workMode'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions    = root.createNestedArray("options");
      arrOptions.add("manualInput");
      arrOptions.add("wallClock");
      arrOptions.add("haClient");
      arrOptions.add("mqttClient");
      arrOptions.add("owmWeather");
      arrOptions.add("intTempSensor");
    }

    if (device_type == "scrollAlign") {
      sprintf( topic_config,  "homeassistant/select/%s/zone%sScrollAlign/config", shortMACaddr.c_str(), zone.c_str() );
      root["name"]            = "Zone" + zone + " scroll align";
      root["uniq_id"]         = "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollAlign";
      root["cmd_t"]           = "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrollalign";
      root["stat_t"]          = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"]         = "{{ value_json['zone" + zone + "']['scrollAlign'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions    = root.createNestedArray("options"); 
      arrOptions.add("PA_LEFT"); 
      arrOptions.add("PA_RIGHT"); 
      arrOptions.add("PA_CENTER"); 
    }

    if (device_type == "scrollEffectIn") {
      sprintf( topic_config,  "homeassistant/select/%s/zone%sScrollEffectIn/config", shortMACaddr.c_str(), zone.c_str() );
      root["name"]            = "Zone" + zone + " scroll effect IN";
      root["uniq_id"]         = "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollEffectIn";
      root["cmd_t"]           = "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrolleffectIn";
      root["stat_t"]          = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"]         = "{{ value_json['zone" + zone + "']['scrollEffectIn'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions    = root.createNestedArray("options"); 
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
      sprintf( topic_config,  "homeassistant/select/%s/zone%sScrollEffectOut/config", shortMACaddr.c_str(), zone.c_str() );
      root["name"]            = "Zone" + zone + " scroll Effect Out";
      root["uniq_id"]         = "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollEffectOut";
      root["cmd_t"]           = "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrolleffectOut";
      root["stat_t"]          = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"]         = "{{ value_json['zone" + zone + "']['scrollEffectOut'] }}";
      root["entity_category"] = "config";
      JsonArray arrOptions    = root.createNestedArray("options"); 
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
      sprintf( topic_config,  "homeassistant/number/%s/zone%sCharspacing/config", shortMACaddr.c_str(), zone.c_str() );
      root["name"]            = "Zone" + zone + " charspacing";
      root["uniq_id"]         = "wledPixelLight" + shortMACaddr + "Zone" + zone + "Charspacing";
      root["cmd_t"]           = "wledPixel-" + shortMACaddr + "/zone" + zone + "/charspacing";
      root["stat_t"]          = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"]         = "{{ value_json['zone" + zone + "']['charspacing'] }}";
      root["entity_category"] = "config";
      root["min"] = 0;
      root["max"] = 3;
    }

    if (device_type == "scrollPause") {
      sprintf( topic_config,  "homeassistant/number/%s/zone%sScrollPause/config", shortMACaddr.c_str(), zone.c_str() );
      root["name"]            = "Zone" + zone + " scroll pause";
      root["uniq_id"]         = "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollPause";
      root["cmd_t"]           = "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrollpause";
      root["stat_t"]          = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"]         = "{{ value_json['zone" + zone + "']['scrollPause'] }}";
      root["entity_category"] = "config";
      root["min"] = 0;
      root["max"] = 30000;
    }

    if (device_type == "scrollSpeed") {
      sprintf( topic_config,  "homeassistant/number/%s/zone%sScrollSpeed/config", shortMACaddr.c_str(), zone.c_str() );
      root["name"]            = "Zone" + zone + " scroll speed";
      root["uniq_id"]         = "wledPixelLight" + shortMACaddr + "Zone" + zone + "ScrollSpeed";
      root["cmd_t"]           = "wledPixel-" + shortMACaddr + "/zone" + zone + "/scrollspeed";
      root["stat_t"]          = "wledPixel-" + shortMACaddr + "/state";
      root["val_tpl"]         = "{{ value_json['zone" + zone + "']['scrollSpeed'] }}";
      root["entity_category"] = "config";
      root["min"] = 0;
      root["max"] = 100;
    }

    if (device_type == "ds18b20") {
      sprintf( topic_config,  "homeassistant/sensor/%s/temperature/config", shortMACaddr.c_str() );
      root["name"]                = "wledPixel-" + shortMACaddr + "_Temperature";
      root["uniq_id"]             = "wledPixelLight" + shortMACaddr + "Temperature";
      root["state_topic"]         = "wledPixel-" + shortMACaddr + "/temperature";
      root["device_class"]        = "temperature";
      root["unit_of_measurement"] = "°C";
    }  

    //result = mqttClient.beginPublish(topic_config, measureJson(root), true);
    //if( result == false ) Serial.println(F( "MQTT HA config error begin!" ));
    //serializeJson(root, mqttClient);
    

    size_t n = serializeJson(root, buffer);
    
    Serial.print(F("\nMQTT publish HA device state: "));
    mqttPublished = mqttClient.publish(topic_config, buffer, n);
    if (mqttPublished) Serial.println(F("OK"));
    else Serial.println(F("fail"));

    result = mqttClient.endPublish();
    if( result == false ) Serial.print(F("\nERROR: MQTT HA config error end!" ));

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
    doc["brightness"] = intensity;
    doc["wifiSsid"] = WiFi.SSID();
    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      doc["zone" + String(n)]["workMode"]         = zones[n].workMode;
      doc["zone" + String(n)]["charspacing"]      = zones[n].charspacing;
      doc["zone" + String(n)]["scrollAlign"]      = zones[n].scrollAlign;
      doc["zone" + String(n)]["scrollPause"]      = zones[n].scrollPause;
      doc["zone" + String(n)]["scrollSpeed"]      = zones[n].scrollSpeed;
      doc["zone" + String(n)]["scrollEffectIn"]   = zones[n].scrollEffectIn;
      doc["zone" + String(n)]["scrollEffectOut"]  = zones[n].scrollEffectOut;
    }

    serializeJson(doc, Serial);
    doc.shrinkToFit();
    doc.garbageCollect();
    size_t n = serializeJson(doc, buffer);
    
    Serial.print(F("MQTT publish device state: "));
    mqttPublished = mqttClient.publish(MQTTStateTopic.c_str(), buffer, n);
    if (mqttPublished) Serial.println(F("OK"));
    else Serial.println(F("fail"));
  }
}

void saveVarsToConfFile(String groupName, uint8_t n) {
  preferences.begin(groupName.c_str(), false);

  if (groupName == "systemSettings") {
    preferences.putString("deviceName",           shortMACaddr);
    preferences.putBool("disablServMess", disableServiceMessages);
  }

  if (groupName == "displaySettings") {
    preferences.putUChar("zoneNumbers",  zoneNumbers);
    preferences.putUChar("zones0Begin",  zones[0].begin);
    preferences.putUChar("zones0End",    zones[0].end);
    preferences.putUChar("zones1Begin",  zones[1].begin);
    preferences.putUChar("zones1End",    zones[1].end);
    preferences.putUChar("zones2Begin",  zones[2].begin);
    preferences.putUChar("zones2End",    zones[2].end);
  }

  if (groupName == "zoneSettings") {
    preferences.putString((String("zone") + n + "WorkMode").c_str(),       zones[n].workMode);
    preferences.putUChar((String("zone")  + n + "ScrolSpeed").c_str(),     zones[n].scrollSpeed);
    preferences.putUChar((String("zone")  + n + "ScrolPause").c_str(),     zones[n].scrollPause);
    preferences.putString((String("zone") + n + "ScrolAlign").c_str(),     zones[n].scrollAlign);
    preferences.putString((String("zone") + n + "ScrolEfIn").c_str(),      zones[n].scrollEffectIn);
    preferences.putString((String("zone") + n + "ScrolEfOut").c_str(),     zones[n].scrollEffectOut);
    preferences.putString((String("zone") + n + "Font").c_str(),           zones[n].font);
    preferences.putUChar((String("zone")  + n + "Charspac").c_str(),       zones[n].charspacing);
    preferences.putString((String("zone") + n + "mqttTexTop").c_str(),     MQTTZones[n].message);
    preferences.putString((String("zone") + n + "mqttPosfix").c_str(),     zones[n].mqttPostfix);
    preferences.putString((String("zone") + n + "ClDispForm").c_str(),     zones[n].clockDisplayFormat);
    preferences.putString((String("zone") + n + "OwmWhDisp").c_str(),      zones[n].owmWhatToDisplay);
    preferences.putString((String("zone") + n + "HaSensorId").c_str(),     zones[n].haSensorId);
    preferences.putString((String("zone") + n + "HaSensorPf").c_str(),     zones[n].haSensorPostfix);
    preferences.putString((String("zone") + n + "Ds18b20Pf").c_str(),      zones[n].ds18b20Postfix);
  }

  if (groupName == "mqttSettings") {
    preferences.putBool("mqttEnable",           mqttEnable);
    preferences.putString("mqttServerAddr",  mqttServerAddress);
    preferences.putUShort("mqttServerPort",     mqttServerPort);
    preferences.putString("mqttUsername",       mqttUsername);
    preferences.putString("mqttPassword",       mqttPassword);
  }

  if (groupName == "wallClockSett") {
    preferences.putChar("ntpTimeZone",         ntpTimeZone);
    preferences.putBool("disableDotBlink",    disableDotsBlink);
    preferences.putUShort("ntpUpdateInt", ntpUpdateInterval);
    preferences.putString("ntpServer",         ntpServer);
  }

  if (groupName == "owmSettings") {
    preferences.putString("owmApiToken",        owmApiToken);
    preferences.putString("owmUnitsFormat",     owmUnitsFormat);
    preferences.putUShort("owmUpdateInt",  owmUpdateInterval);
    preferences.putString("owmCity",            owmCity);
  }

  if (groupName == "haSettings") {
    preferences.putString("haAddr",           haAddr);
    preferences.putString("haApiToken",       haApiToken);
    preferences.putString("haApiHttpType",    haApiHttpType);
    preferences.putUShort("haApiPort",        haApiPort);
    preferences.putUShort("haUpdateInt", haUpdateInterval);
  }

  if (groupName == "ds18b20Settings") {
    preferences.putBool("ds18b20Enable",           ds18b20Enable);
    preferences.putUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    preferences.putString("ds18b20UnitFr",    ds18b20UnitsFormat);
  }

  if (groupName == "intensity") {
    preferences.putUChar("intensity", intensity);
  }

  preferences.end();   
}

void readConfig(String groupName, uint8_t n) {
  preferences.begin(groupName.c_str(), true);

  if (groupName == "systemSettings") {
    shortMACaddr           = preferences.getString("deviceName", shortMACaddr);
    disableServiceMessages = preferences.getBool("disablServMess", disableServiceMessages);
  }

  if (groupName == "displaySettings") {
    preferences.begin("displaySettings", true);
    zoneNumbers     = preferences.getUChar("zoneNumbers", zoneNumbers);
    zones[0].begin  = preferences.getUChar("zones0Begin", zones[0].begin);
    zones[0].end    = preferences.getUChar("zones0End", zones[0].end);
    zones[1].begin  = preferences.getUChar("zones1Begin", zones[1].begin);
    zones[1].end    = preferences.getUChar("zones1End", zones[1].end);
    zones[2].begin  = preferences.getUChar("zones2Begin", zones[2].begin);
    zones[2].end    = preferences.getUChar("zones2End", zones[2].end);
  }

  if (groupName == "zoneSettings") {
    zones[n].workMode            = preferences.getString((String("zone") + n + "WorkMode").c_str(), zones[n].workMode);
    zones[n].scrollSpeed         = preferences.getUChar((String("zone")  + n + "ScrolSpeed").c_str(), zones[n].scrollSpeed);
    zones[n].scrollPause         = preferences.getUChar((String("zone")  + n + "ScrolPause").c_str(), zones[n].scrollPause);
    zones[n].scrollAlign         = preferences.getString((String("zone") + n + "ScrolAlign").c_str(), zones[n].scrollAlign);
    zones[n].scrollEffectIn      = preferences.getString((String("zone") + n + "ScrolEfIn").c_str(), zones[n].scrollEffectIn);
    zones[n].scrollEffectOut     = preferences.getString((String("zone") + n + "ScrolEfOut").c_str(), zones[n].scrollEffectOut);
    zones[n].font                = preferences.getString((String("zone") + n + "Font").c_str(), zones[n].font);
    zones[n].charspacing         = preferences.getUChar((String("zone")  + n + "Charspac").c_str(), zones[n].charspacing);
    MQTTZones[n].message         = preferences.getString((String("zone") + n + "mqttTexTop").c_str(), MQTTZones[n].message);
    zones[n].mqttPostfix         = preferences.getString((String("zone") + n + "mqttPosfix").c_str(), zones[n].mqttPostfix);
    zones[n].clockDisplayFormat  = preferences.getString((String("zone") + n + "ClDispForm").c_str(), zones[n].clockDisplayFormat);
    zones[n].owmWhatToDisplay    = preferences.getString((String("zone") + n + "OwmWhDisp").c_str(), zones[n].owmWhatToDisplay);
    zones[n].haSensorId          = preferences.getString((String("zone") + n + "HaSensorId").c_str(), zones[n].haSensorId);
    zones[n].haSensorPostfix     = preferences.getString((String("zone") + n + "HaSensorPf").c_str(), zones[n].haSensorPostfix);
    zones[n].ds18b20Postfix      = preferences.getString((String("zone") + n + "Ds18b20Pf").c_str(), zones[n].ds18b20Postfix);
  }

  if (groupName == "mqttSettings") {
    mqttEnable        = preferences.getBool("mqttEnable",           mqttEnable);
    mqttServerAddress = preferences.getString("mqttServerAddr",  mqttServerAddress);
    mqttServerPort    = preferences.getUShort("mqttServerPort",     mqttServerPort);
    mqttUsername      = preferences.getString("mqttUsername",       mqttUsername);
    mqttPassword      = preferences.getString("mqttPassword",       mqttPassword);
  }

  if (groupName == "wallClockSett") {
    ntpTimeZone       = preferences.getChar("ntpTimeZone",         ntpTimeZone);
    disableDotsBlink  = preferences.getBool("disableDotBlink",    disableDotsBlink);
    ntpUpdateInterval = preferences.getUShort("ntpUpdateInt", ntpUpdateInterval);
    ntpServer         = preferences.getString("ntpServer",         ntpServer);
  }

  if (groupName == "owmSettings") {
    owmApiToken       = preferences.getString("owmApiToken",        owmApiToken);
    owmUnitsFormat    = preferences.getString("owmUnitsFormat",     owmUnitsFormat);
    owmUpdateInterval = preferences.getUShort("owmUpdateInt",  owmUpdateInterval);
    owmCity           = preferences.getString("owmCity",            owmCity);
  }

  if (groupName == "haSettings") {
    haAddr            = preferences.getString("haAddr",           haAddr);
    haApiToken        = preferences.getString("haApiToken",       haApiToken);
    haApiHttpType     = preferences.getString("haApiHttpType",    haApiHttpType);
    haApiPort         = preferences.getUShort("haApiPort",        haApiPort);
    haUpdateInterval  = preferences.getUShort("haUpdateInt", haUpdateInterval);
  }

  if (groupName == "ds18b20Settings") {
    ds18b20Enable         = preferences.getBool("ds18b20Enable",           ds18b20Enable);
    ds18b20UpdateInterval = preferences.getUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    ds18b20UnitsFormat    = preferences.getString("ds18b20UnitFr",    ds18b20UnitsFormat);
  }

  if (groupName == "intensity") {
    intensity = preferences.getUChar("intensity", intensity);
  }

  preferences.end();
}

void readAllConfig() {
  readConfig("systemSettings", 99);
  readConfig("displaySettings", 99);
  for ( uint8_t n = 0; n < zoneNumbers; n++) {
    readConfig("zoneSettings", n);
  }
  readConfig("mqttSettings", 99);
  readConfig("wallClockSett", 99);
  readConfig("owmSettings", 99);
  readConfig("haSettings", 99);
  readConfig("ds18b20Settings", 99);
  readConfig("intensity", 99);
}

void zoneNewMessage(int zone, String newMessage, String postfix) {
  if ( zone == 0) {
    strcpy(zone0Message, (newMessage + postfix).c_str());
    zones[0].newMessageAvailable = true;
  }
  if ( zone == 1) {
    strcpy(zone1Message, (newMessage + postfix).c_str());
    zones[1].newMessageAvailable = true;
  }
  if ( zone == 2) {
    strcpy(zone2Message, (newMessage + postfix).c_str());
    zones[2].newMessageAvailable = true;
  }
//  if ( zone == 3) {
//    strcpy(zone3Message, (newMessage + postfix).c_str());
//    zone3newMessageAvailable = true;
//  }
}

// check a string to see if it is numeric
bool isNumeric(String str){
  for(byte i=0;i<str.length();i++) {
    if(isDigit(str.charAt(i))) return true;
  }
  return false;
}

void ntpUpdateTime() {
  if (timeClient.update()) {
    previousNTPsyncMillis = currentMillis;
    Serial.print(F("\nNTP time synchronized successfully"));
  } else {
    previousNTPsyncMillis = -10000000;
    timeClient.forceUpdate();
    delay(1000);
    Serial.print(F("\nERROR: NTP time synchronization failed"));
  }
}

void MQTTCallback(char* topic, byte* payload, int length) {
  String topicStr(topic);
  String PayloadString = "";

  for (int i = 0; i < length; i++) { PayloadString = PayloadString + (char)payload[i]; }

  Serial.print(F("\nMQTT new message arrived"));
  Serial.print(F("\nTopic: "));
  Serial.println(topic);
  Serial.print(PayloadString);

  if (topicStr == MQTTIntensity) {
    if (isNumeric(PayloadString)) {
      if (PayloadString.toInt() > 0) {
        intensity = PayloadString.toInt()-1;
        if (!power) {
          power = true;
          P.displayShutdown(0);
        }
        P.setIntensity(intensity);
        saveVarsToConfFile("intensity", 99);
      }
      if (PayloadString.toInt() == 0) {
        P.displayShutdown(1);
        power = false;
      }
    } else {
      Serial.print(F("Supports are only numeric values"));
    }
    
  }
  if (topicStr == MQTTPower) {
    if(PayloadString == "on" || PayloadString == "ON" || PayloadString == "TRUE") {
      P.displayShutdown(0);
      power = true;
    }
    if(PayloadString == "off" || PayloadString == "OFF" || PayloadString == "FALSE") {
      P.displayShutdown(1);
      power = false;
    }
  }
  
  for ( uint8_t n = 0; n < zoneNumbers; n++) {
    bool zoneSettingsChanged = false;
    if(zones[n].workMode == "mqttClient" && topicStr == MQTTZones[n].message) zoneNewMessage(n, PayloadString.c_str(), zones[n].mqttPostfix);

    if (topicStr == MQTTZones[n].scrollEffectIn) {
      zones[n].scrollEffectIn = PayloadString.c_str();
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
      zoneSettingsChanged = true;
    }
    if (topicStr == MQTTZones[n].scrollEffectOut) {
      zones[n].scrollEffectOut = PayloadString.c_str();
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
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
      }

      zoneSettingsChanged = true;
    }
    if (zoneSettingsChanged) saveVarsToConfFile("zoneSettings", n);
  }
}

boolean reconnect() {
  //mqttClient.disconnect();
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);

  // Attempt to connect
  if (mqttClient.connect(MQTTGlobalPrefix.c_str(), mqttUsername.c_str(),mqttPassword.c_str())) {
    MQTTPublishHADiscovry("0", "light");
    MQTTPublishHADiscovry("0", "ds18b20");

    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      MQTTPublishHADiscovry(String(n), "scrollAlign");
      MQTTPublishHADiscovry(String(n), "charspacing");
      MQTTPublishHADiscovry(String(n), "workMode");
      MQTTPublishHADiscovry(String(n), "scrollPause");
      MQTTPublishHADiscovry(String(n), "scrollSpeed");
      MQTTPublishHADiscovry(String(n), "scrollEffectIn");
      MQTTPublishHADiscovry(String(n), "scrollEffectOut");
      

      if (zones[n].workMode == "mqttClient" && MQTTZones[n].message != "" && MQTTZones[n].message != " ") mqttClient.subscribe((char*) MQTTZones[n].message.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollEffectIn.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollEffectOut.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollSpeed.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollPause.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollAllign.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].charspacing.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].workMode.c_str());
    }
    MQTTPublishState();
    Serial.println(F("MQTT subscribe objects"));
    mqttClient.subscribe((char*) MQTTIntensity.c_str());
    mqttClient.subscribe((char*) MQTTPower.c_str());
  }  else  {
    Serial.print(F("MQTT failed, rc= "));
    Serial.println(mqttClient.state());
    Serial.print(F(" try again in 5 seconds"));
    P.setTextBuffer(0, "mq err");
    P.displayReset(0);
  }
  return mqttClient.connected();
}

void wifiApWelcomeMessage(AsyncWiFiManager *wifiManager) {
  P.begin(zoneNumbers);
  P.setZone(0, zones[0].begin, zones[0].end);
  P.displayClear();
  P.setTextAlignment(0, stringToTextPositionT(zones[0].scrollAlign));
  P.setIntensity(7);
  P.setSpeed(0, 100);
  P.write(MQTTGlobalPrefix.c_str());
  if (MAX_DEVICES > 5) {
    delay(5000);
    P.write(wifiAPpassword);
  }
}

String getCurTime(String curZoneFont, String displayFormat) {
      if (currentMillis - previousNTPsyncMillis >= (unsigned)ntpUpdateInterval * 3600 * 1000) ntpUpdateTime();

      String t = (String)timeClient.getFormattedTime(); // returns HH:MM:SS
      
      if (displayFormat == "HHMM") {
        t.remove(5,4);
        return t;
      }
      if (displayFormat == "HH") {
        t.remove(2,7);
        return t;
      }
      if (displayFormat == "MM") {
        t.remove(5,4);
        t.remove(0,3);
        return t;
      }

      time_t epochTime = timeClient.getEpochTime(); // returns the Unix epoch
      //Get a time structure
      struct tm *ptm = gmtime ((time_t *)&epochTime); 
      int monthDay = ptm->tm_mday;
      int currentMonth = ptm->tm_mon+1;
      
      String currentMonthStr = "";
      if(currentMonth < 10) currentMonthStr += '0';
      currentMonthStr += currentMonth;

      String monthDayStr = "";
      if(monthDay < 10) monthDayStr += '0';
      monthDayStr += monthDay;
      
      //String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
      String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      String weekDaysCyrillic[7] = {"½", "§", "©", "®", "µ", "¶", "¼"};
      String weekDay;
      if (curZoneFont == "wledFont_cyrillic") {
        weekDay = weekDaysCyrillic[timeClient.getDay()];  
      } else {
        weekDay = weekDays[timeClient.getDay()];
      }
      int currentYear = ptm->tm_year+1900;

      if (displayFormat == "ddmmyyyy") t = monthDayStr + "." + currentMonthStr + "." + String(currentYear);
      if (displayFormat == "ddmm") t = monthDayStr + "." + currentMonthStr;
      if (displayFormat == "ddmmaa") t = monthDayStr + "." + currentMonthStr + String(weekDay);
      if (displayFormat == "aa") t = String(weekDay);
      if (displayFormat == "ddmmaahhmm") {
        t.remove(5,4);
        t = monthDayStr + "." + currentMonthStr + " " + String(weekDay) + " " + t;
      }
      return t;
}

String flashClockDots(String t) {
  if(t.indexOf(":") > 0) t.replace(":", "¦");
  else t.replace("¦", ":");
  return t;
}

void displayAnimation() {
  if (P.displayAnimate()) {
    if (zones[0].newMessageAvailable && P.getZoneStatus(0)) {
        Serial.print(F("\nzone0 Message availabel: "));
        Serial.println(zone0Message);
        zones[0].newMessageAvailable = false;
        P.setTextBuffer(0, zone0Message);
        P.displayReset(0);
    }

    if (zones[1].newMessageAvailable && P.getZoneStatus(1)) {
        Serial.print(F("\nzone1 Message availabel: "));
        Serial.println(zone1Message);
        zones[1].newMessageAvailable = false;
        P.setTextBuffer(1, zone1Message);
        P.displayReset(1);
    }

    if (zones[2].newMessageAvailable && P.getZoneStatus(2)) {
        Serial.print(F("\nzone2 Message availabel: "));
        Serial.println(zone2Message);
        zones[2].newMessageAvailable = false;
        P.setTextBuffer(2, zone2Message);
        P.displayReset(2);
    }

//    if (zone3newMessageAvailable && P.getZoneStatus(3)) {
//        Serial.printf("\nzone3Message availabel: %s", zone3Message);
//        zone3newMessageAvailable = false;
//        P.setTextBuffer(3, zone3Message);
//        P.displayReset(3);
//    }
  }
}

String haApiGet(String sensorId, String sensorPostfix) {
  Serial.printf("\nHA updating...");
  bool https;
  if (haApiHttpType == "https") https = true;
  if (haApiHttpType == "http") https = false;
  JsonObject haApiRespondPostObj = httpsRequest(haAddr, haApiPort, "/api/states/" + sensorId, "Bearer " + haApiToken, https);
  return haApiRespondPostObj[F("state")].as<String>() + sensorPostfix;
}

void setup() {
  Serial.begin(115200);
  Serial.print(F("Start serial...."));

  readAllConfig();
 
  #if defined(ESP32)
    WiFi.mode(WIFI_STA);
    pinMode(VBAT_PIN, INPUT);
  #endif

  AsyncWiFiManager wifiManager(&server,&dns);
  bool wifiRes;
  //WiFi.persistent(true);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setAPCallback(wifiApWelcomeMessage);
  wifiRes = wifiManager.autoConnect(MQTTGlobalPrefix.c_str(), wifiAPpassword);
  if(!wifiRes) {
    Serial.print(F("Wifi failed to connect"));
  } else {
    WiFi.hostname(MQTTGlobalPrefix.c_str());
    Serial.print(F(""));
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
  }

  
  
  // Initializing display
  P.begin(zoneNumbers);
  for ( byte n = 0; n < zoneNumbers; n++) {
    P.setZone(n, zones[n].begin, zones[n].end);
  }
  P.setIntensity(intensity);

  // Start MQTT client
  if (mqttEnable) {
    mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
    mqttClient.setCallback(MQTTCallback);
    mqttClient.setBufferSize(4096);
  }  
  
  // start NTP client
  timeClient.begin();
  timeClient.setTimeOffset(ntpTimeZone * 3600);
  timeClient.setUpdateInterval(ntpUpdateInterval * 3600);
  timeClient.setPoolServerName(ntpServer.c_str());
  ntpUpdateTime();

  // Start Dallas ds18b20
  //if (ds18b20Enable) {
  //  oneWire.reset();
  //  oneWire.search(addrS1); //getting the address of Sesnor-1
  //}

  // Web server routings
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", indexPage, processor);
  });

  server.on("/settings", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", settingsPage, processor);
  });

  server.on("/api/temperature", HTTP_ANY, [](AsyncWebServerRequest *request){
    if (ds18b20Enable) request->send(200, "text/plain", String(dsTemp).c_str());
    else request->send(404);
  });
  
  server.on("/api/message", HTTP_ANY, [](AsyncWebServerRequest *request){
    Serial.print(F("API request received "));
      int params = request->params();
      Serial.print(params);
      Serial.println(F(" params sent in"));

      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

          if (strcmp(p->name().c_str(), "messageZone0") == 0) zoneNewMessage(0, p->value().c_str(), "");
          if (strcmp(p->name().c_str(), "messageZone1") == 0) zoneNewMessage(1, p->value().c_str(), "");
          if (strcmp(p->name().c_str(), "messageZone2") == 0) zoneNewMessage(2, p->value().c_str(), "");
//          if (strcmp(p->name().c_str(), "messageZone3") == 0) zoneNewMessage(3, p->value().c_str(), "");
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
  });


  server.on("/api/config", HTTP_ANY, [](AsyncWebServerRequest *request) {
      Serial.print(F("\nAPI request received "));
      int params = request->params();
      Serial.print(params);
      Serial.println(F(" params sent in"));
      AsyncWebParameter* key = request->getParam(0);
      uint8_t n = 99;
      bool finishRequest = false;
      for(int i=1;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());

          if (key->value() == "systemSettings") {
              if (p->name() == "disableServiceMessages") {
                if (strcmp(p->value().c_str(),"true") == 0)   disableServiceMessages = true;
                if (strcmp(p->value().c_str(),"false") == 0)  disableServiceMessages = false;
              }
              if (p->name() == "deviceName")                  shortMACaddr           = p->value().c_str();
              
              finishRequest = true;    
          }

          if (key->value() == "displaySettings") {
              if (p->name() == "zoneNumbers") zoneNumbers     = p->value().toInt();
              if (p->name() == "zone0Begin")  zones[0].begin  = p->value().toInt();
              if (p->name() == "zone0End")    zones[0].end    = p->value().toInt();
              if (p->name() == "zone1Begin")  zones[1].begin  = p->value().toInt();
              if (p->name() == "zone1End")    zones[1].end    = p->value().toInt();
              if (p->name() == "zone2Begin")  zones[2].begin  = p->value().toInt();
              if (p->name() == "zone2End")    zones[2].end    = p->value().toInt();

              restartESP = true;
              finishRequest = true; 
          }
          
          if (key->value() == "zoneSettings") {
              if (p->name() == "zone") n = p->value().toInt();
              if (p->name() == "workMode") {
                  zones[n].workMode = p->value().c_str();
                  if (zones[n].workMode == "owmWeather")   owmLastTime = -1000000;
                  if (zones[n].workMode == "haClient")     haLastTime  = -1000000;
                  if (zones[n].workMode == "mqttClient" && mqttEnable) mqttClient.disconnect();
                  if (zones[n].workMode == "wallClock") {
                      timeClient.setTimeOffset(ntpTimeZone * 3600);
                      ntpUpdateTime();
                      zones[n].previousMillis = -1000000;
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
                  P.setTextAlignment(n, stringToTextPositionT(zones[n].scrollAlign));
              }
              if (p->name() == "scrollEffectIn") {
                  zones[n].scrollEffectIn = p->value().c_str();
                  P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
              }
              if (p->name() == "scrollEffectOut") {
                  zones[n].scrollEffectOut = p->value().c_str();
                  P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
              }
              if (p->name() == "mqttTextTopic") {
                  if (MQTTZones[n].message != p->value().c_str() && mqttEnable) {
                      MQTTZones[n].message = p->value().c_str();
                      mqttClient.disconnect();
                  }
              }
              if (p->name() == "mqttPostfix") {
                  if (zones[n].mqttPostfix != p->value().c_str() && mqttEnable) {
                      zones[n].mqttPostfix = p->value().c_str();
                      mqttClient.disconnect();
                  }
              }
              if (p->name() == "clockDisplayFormat") {
                  zones[n].clockDisplayFormat = p->value().c_str();
              }
              if (p->name() == "owmWhatToDisplay") {
                  if (zones[n].workMode == "owmWeather") {
                    zones[n].owmWhatToDisplay = p->value().c_str();
                    if(zones[n].owmWhatToDisplay == "owmWeatherIcon") {
                        P.setFont(n, wledSymbolFont);
                        zones[n].font = p->value().c_str();
                        P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), PA_NO_EFFECT);
                    }
                  }
              }
              if (p->name() == "font") {
                  zones[n].font = p->value().c_str();
                  applyZoneFont(n, zones[n].font);
              }
              if (p->name() == "charspacing") {
                  zones[n].charspacing = p->value().toInt();
                  P.setCharSpacing(n, zones[n].charspacing);
              }
              if (p->name() == "ds18b20Postfix") {
                  zones[n].ds18b20Postfix = p->value().c_str();
                  previousDsMillis        = -1000;
              }
              if (p->name() == "haSensorId")       zones[n].haSensorId       = p->value().c_str();
              if (p->name() == "haSensorPostfix")  zones[n].haSensorPostfix  = p->value().c_str();

              finishRequest = true; 
          }

          if (key->value() == "mqttSettings") {
            if (p->name() == "mqttEnable") {
              if (strcmp(p->value().c_str(),"true") == 0)   mqttEnable = true;
              if (strcmp(p->value().c_str(),"false") == 0)  mqttEnable = false;
            }
            if (p->name() == "mqttServerAddress")           mqttServerAddress = p->value().c_str();
            if (p->name() == "mqttServerPort")              mqttServerPort = p->value().toInt();
            if (p->name() == "mqttUsername")                mqttUsername = p->value().c_str();
            if (p->name() == "mqttPassword")                mqttPassword = p->value().c_str();

            finishRequest = true; 
          }

          if (key->value() == "wallClockSett") {
            if (p->name() == "ntpTimeZone") {
              ntpTimeZone = p->value().toInt();
              timeClient.setTimeOffset(ntpTimeZone * 3600);
            }
            if (p->name() == "disableDotsBlink") {
              if (strcmp(p->value().c_str(),"true") == 0)   disableDotsBlink = true;
              if (strcmp(p->value().c_str(),"false") == 0)  disableDotsBlink = false;
              for ( uint8_t i = 0; i < zoneNumbers; i++) {
                zones[i].previousMillis = currentMillis + 60001;
              }
            }
            if (p->name() == "ntpUpdateInterval") {
              ntpUpdateInterval = p->value().toInt();
              timeClient.setUpdateInterval(ntpUpdateInterval * 3600);
            }
            if (p->name() == "ntpServer") {
              ntpServer = p->value().c_str();
              timeClient.setPoolServerName(ntpServer.c_str());
            }
            finishRequest = true; 
          }

          if (key->value() == "owmSettings") {
            if (p->name() == "owmApiToken")       owmApiToken = p->value().c_str();
            if (p->name() == "owmUnitsFormat")    owmUnitsFormat = p->value().c_str();
            if (p->name() == "owmUpdateInterval") owmUpdateInterval = p->value().toInt();
            if (p->name() == "owmCity")           owmCity = p->value().c_str();

            finishRequest = true; 
          }

          if (key->value() == "haSettings") {
            if (p->name() == "haAddr")            haAddr = p->value().c_str();
            if (p->name() == "haUpdateInterval")  haUpdateInterval = p->value().toInt();
            if (p->name() == "haApiToken")        haApiToken = p->value().c_str();
            if (p->name() == "haApiHttpType")     haApiHttpType = p->value().c_str();
            if (p->name() == "haApiPort")         haApiPort = p->value().toInt();

            finishRequest = true; 
          }

          if (key->value() == "ds18b20Settings") {
            if (p->name() == "ds18b20Enable") {
              if (strcmp(p->value().c_str(),"true") == 0)   ds18b20Enable = true;
              if (strcmp(p->value().c_str(),"false") == 0)  ds18b20Enable = false;
            }
            if (p->name() == "ds18b20UpdateInterval")       ds18b20UpdateInterval = p->value().toInt();
            if (p->name() == "ds18b20UnitsFormat") {
              ds18b20UnitsFormat = p->value().c_str();
              previousDsMillis = -1000;
            }

            finishRequest = true; 
          }
          if (key->value() == "intensity") {
            if (p->name() == "intensity") {
              if ((p->value()).toInt() > 0) {
                intensity = (p->value()).toInt()-1;
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
      }
      
      if (finishRequest) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
        saveVarsToConfFile(key->value(), n);
        if (key->value() == "wallClockSett") ntpUpdateTime();
        //readConfig(key->value(), n);
      }
      
  });

  // Start webserver
  server.begin();

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);

  // Start ds18b20
  sensors.begin();
}

void testZones(uint8_t n) {
  P.displayClear();
  P.setFont(n, wledSymbolFont);
  P.setCharSpacing(n,0);
  P.setPause(n, 500);
  P.setSpeed(n, 25);
  if (n < zoneNumbers) {
    P.setTextEffect(n, PA_GROW_UP, PA_NO_EFFECT);
    String testMessage = "8";
    for ( int i = 0; i < int(zones[n].end - zones[n].begin); i++) testMessage += 8;
    zoneNewMessage(n, testMessage, "");
  } 
  if (n == zoneNumbers) {
    P.setCharSpacing(0,1);
    P.setFont(0, wledFont_cyrillic);
    P.setSpeed(0, 50);
    P.setTextEffect(0, PA_SCROLL_LEFT, PA_NO_EFFECT);
    zoneNewMessage(0, "ip: " + WiFi.localIP().toString(), "");
  }
}

void loop() {
  currentMillis = millis();
  
  #if defined(ESP32)
    if (currentMillis - previousBatVoltMillis >= (unsigned)BatVoltUpdateInterval * 1000) {
      previousBatVoltMillis = currentMillis;
      battv = ((float)analogRead(VBAT_PIN) / 4095) * 3.3 * 2 * 1.068;
      Serial.printf("Battery voltage: %f", battv);
      if (mqttEnable) mqttClient.publish(String("wledPixel-" + shortMACaddr + "/batteryVoltage").c_str(), String(battv).c_str());
    }
  #endif

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
      allTestsFinish      = true;
      initConfig          = true;
    }
    if (nLoop == 0) { 
      testZones(nLoop);
      nLoop++;
    } else if (nLoop <= zoneNumbers && P.getZoneStatus(nLoop-1)) {
      testZones(nLoop);
      nLoop++;
    }
  }

  // apply saved config for display
  if (initConfig) {
    Serial.print(F("Init configuration"));
    initConfig = false;
    readAllConfig();
    
    if (mqttEnable) mqttClient.disconnect();
    P.setIntensity(intensity);
    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      P.setSpeed(n, zones[n].scrollSpeed);
      P.setPause(n, zones[n].scrollPause * 1000);
      P.setTextAlignment(n, stringToTextPositionT(zones[n].scrollAlign));
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
      P.setCharSpacing(n, zones[n].charspacing);
      applyZoneFont(n, zones[n].font);

      if (zones[n].workMode == "wallClock") {
        zones[n].curTime = getCurTime(zones[n].font, zones[n].clockDisplayFormat);
        zones[n].previousMillis = -1000000;
      }

      if (zones[n].workMode == "mqttClient" && !disableServiceMessages) zoneNewMessage(n, "MQTT", "");
      if (zones[n].workMode == "manualInput" && !disableServiceMessages) zoneNewMessage(n, "Manual", "");
      if (zones[n].workMode == "owmWeather") {
        owmLastTime = -1000000;
        if(zones[n].owmWhatToDisplay == "owmWeatherIcon") {
          P.setFont(n, wledSymbolFont);
          P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), PA_NO_EFFECT);
        }
      }
      if (zones[n].workMode == "haClient") haLastTime = -1000000;
    }
  }
  
  if (allTestsFinish) {
    if (mqttEnable && mqttServerAddress.length() > 0){
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
        if (!mqttPublished) MQTTPublishState();
      }
    }

    // DS18B20 measurement
    if (ds18b20Enable) {
      if (currentMillis - previousDsMillis >= (unsigned)ds18b20UpdateInterval * 1000) {
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
            Serial.printf("\nds18b20 measurement temperature: %s F", dsTemp.c_str());
          } else {
            dtostrf(dsTempC, 4, 1, dsTempBuff);
            dsTemp = (char*)dsTempBuff;
            Serial.printf("\nds18b20 measurement temperature: %s C", dsTemp.c_str());
          }
          if (mqttEnable) mqttClient.publish(String("wledPixel-" + shortMACaddr + "/temperature").c_str(), dsTemp.c_str());
        }
      }
    }

    // OWM update
    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      if (zones[n].workMode == "owmWeather" && owmApiToken.length() > 0 && owmCity.length() > 0) owmWeatherEnable = true;
    }
    if (owmWeatherEnable && currentMillis - owmLastTime >= (unsigned)owmUpdateInterval * 1000) owmWeatherUpdate(owmCity, owmUnitsFormat, owmApiToken);

    // zones rutines
    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      // Wall Clock
      if (zones[n].workMode == "wallClock") {
        String curTimeNew = getCurTime(zones[n].font, zones[n].clockDisplayFormat);
        if (zones[n].clockDisplayFormat == "HHMM" || zones[n].clockDisplayFormat == "HHMMSS" || zones[n].clockDisplayFormat == "ddmmaahhmm") {
          if (!disableDotsBlink) {
            if (currentMillis - zones[n].previousMillis >= 1000) {
                zones[n].previousMillis = currentMillis;
                    if (zones[n].curTime == curTimeNew || zones[n].curTime == flashClockDots(curTimeNew)) {
                        if (zones[n].curTime.indexOf(":") > 0) curTimeNew.replace(":", "¦");
                        P.setPause(n,100);
                        P.setTextEffect(n, PA_NO_EFFECT, PA_NO_EFFECT);
                        zones[n].curTime = curTimeNew;
                        zoneNewMessage(n, zones[n].curTime, "");
                    } else {
                        if ( zones[n].curTime.indexOf(":") > 0 ) curTimeNew.replace(":", "¦");
                        zones[n].curTime = curTimeNew;
                        P.setPause(n,100);
                        P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), PA_NO_EFFECT);
                        zoneNewMessage(n, zones[n].curTime, "");
                    }
            }
          } else {
            if (currentMillis - zones[n].previousMillis >= 60000) {
                zones[n].previousMillis = currentMillis;
                zones[n].curTime = curTimeNew;
                zoneNewMessage(n, zones[n].curTime, "");   
            }
          }
        } else {
          if (currentMillis - zones[n].previousMillis >= 60000) {
            zones[n].previousMillis = currentMillis;
            zones[n].curTime = curTimeNew;
            zoneNewMessage(n, zones[n].curTime, "");
          }
        }
      }

      // OWM
      if (zones[n].workMode == "owmWeather") {
        if (owmWeatherEnable) {
          if (currentMillis - owmLastTime >= (unsigned)owmUpdateInterval * 1000) {
            zoneNewMessage(n, openWetherMapGetWeather(zones[n].owmWhatToDisplay, owmUnitsFormat), "");
            owmPrinted = true;
          }
        } else {
          zoneNewMessage(n, "err", "");
          Serial.print(F("\nERROR: Open Weather Map config error"));
        }
      }

      // HA client
      if (zones[n].workMode == "haClient") {
        if (currentMillis - haLastTime >= (unsigned)haUpdateInterval * 1000) {
          zoneNewMessage(n, haApiGet(zones[n].haSensorId, zones[n].haSensorPostfix), "");
          haLastTime = currentMillis;
        }
      }

      // DS18B20
      if (zones[n].workMode == "intTempSensor" && dsTempToDisplay == true) {
        zoneNewMessage(n, String(dsTemp), zones[n].ds18b20Postfix);
        dsTempToDisplay = false;
      }
      // Manual
      if (zones[n].workMode == "manualInput") delay(1);
    }

    if (owmWeatherEnable && owmPrinted) {
      owmLastTime = currentMillis;
      owmPrinted  = false;
    }

  } 

  displayAnimation(); 
}