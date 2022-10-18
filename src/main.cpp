#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <AsyncElegantOTA.h>

#include "htmlFiles.h"
#include <wledFont.h>

WiFiClient espClient;
WiFiClient mqttEspClient;

/// GLOBAL ///
const char* firmwareVer = "2.5";

//// DS18B20 ////
const int oneWireBus = D4;  // WeMos D1 mini GPIO02
byte addrS1[8];
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
unsigned long previousDsMillis = -1000;

String ds18b20Enable = "false";
int ds18b20UpdateInterval = 30; // in seconds
String ds18b20UnitsFormat = "Celsius";
char dsTemp[5];
bool dsTempToDisplay = false;


//// MQTT settings ////
String mqttServerAddress  = "";
int mqttServerPort        = 1883;
String mqttUsername       = "";
String mqttPassword       = "";
long lastReconnectAttempt = 0;
PubSubClient mqttClient(mqttEspClient);

String shortMACaddr = WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15); // last five chars of the MAC, ie: C0A4;
String deviceName = shortMACaddr;
String MQTTGlobalPrefix = "wledPixel-" + deviceName;

typedef struct {
  String    message, scrollEffectIn, scrollEffectOut, scrollSpeed, scrollPause, scrollAllign, charspacing, workMode;
} MQTTZoneData;

MQTTZoneData MQTTZones[] = {
  {MQTTGlobalPrefix + "/zone0/text", MQTTGlobalPrefix + "/zone0/scrolleffectIn", MQTTGlobalPrefix + "/zone0/scrolleffectOut", MQTTGlobalPrefix + "/zone0/scrollspeed", MQTTGlobalPrefix + "/zone0/scrollpause", MQTTGlobalPrefix + "/zone0/scrollalign", MQTTGlobalPrefix + "/zone0/charspacing", MQTTGlobalPrefix + "/zone0/workmode"},
  {MQTTGlobalPrefix + "/zone1/text", MQTTGlobalPrefix + "/zone1/scrolleffectIn", MQTTGlobalPrefix + "/zone1/scrolleffectOut", MQTTGlobalPrefix + "/zone1/scrollspeed", MQTTGlobalPrefix + "/zone1/scrollpause", MQTTGlobalPrefix + "/zone1/scrollalign", MQTTGlobalPrefix + "/zone1/charspacing", MQTTGlobalPrefix + "/zone1/workmode"},
  {MQTTGlobalPrefix + "/zone2/text", MQTTGlobalPrefix + "/zone2/scrolleffectIn", MQTTGlobalPrefix + "/zone2/scrolleffectOut", MQTTGlobalPrefix + "/zone2/scrollspeed", MQTTGlobalPrefix + "/zone2/scrollpause", MQTTGlobalPrefix + "/zone2/scrollalign", MQTTGlobalPrefix + "/zone2/charspacing", MQTTGlobalPrefix + "/zone2/workmode"},
  {MQTTGlobalPrefix + "/zone3/text", MQTTGlobalPrefix + "/zone3/scrolleffectIn", MQTTGlobalPrefix + "/zone3/scrolleffectOut", MQTTGlobalPrefix + "/zone3/scrollspeed", MQTTGlobalPrefix + "/zone3/scrollpause", MQTTGlobalPrefix + "/zone3/scrollalign", MQTTGlobalPrefix + "/zone3/charspacing", MQTTGlobalPrefix + "/zone3/workmode"},
};
String MQTTIntensity = MQTTGlobalPrefix + "/intensity";
String MQTTPower = MQTTGlobalPrefix + "/power";
String MQTTStateTopic = MQTTGlobalPrefix + "/state";
//// MQTT settings ////


// Display config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // type of device hardware https://majicdesigns.github.io/MD_MAX72XX/page_hardware.html
//// Display pinout
#define DATA_PIN  D7                      // WeMos D1 mini GPIO13
#define CS_PIN    D6                      // WeMos D1 mini GPIO12
#define CLK_PIN   D5                      // WeMos D1 mini GPIO14
uint8_t MAX_DEVICES = 16;                 // number of device segments
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

typedef struct {
    uint8_t   begin, end;
    uint8_t   scrollSpeed, charspacing;
    uint16_t  scrollPause;
    unsigned long previousMillis;
    String    scrollAlign, scrollEffectIn, scrollEffectOut, font, workMode, clockDisplayFormat, haSensorId, haSensorPostfix, owmWhatToDisplay, mqttPostfix, ds18b20Postfix, curTimeZone;
} ZoneData;

// structure:
ZoneData zones[] = {
  {0, 2, 35, 1, 3000, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature", "", "", ""},
  {3, 4, 35, 1, 3000, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature", "", "", ""},
  {5, 6, 35, 1, 3000, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature", "", "", ""},
  {7, 7, 35, 1, 3000, 0, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature", "", "", ""},
};

uint8_t zoneNumbers = 1;
uint8_t intensity   = 7;
String power = "ON";
String disableServiceMessages = "false";
String disableDotsBlink = "false";

char zone0Message[50] = "zone0";
char zone1Message[50] = "zone1";
char zone2Message[50] = "zone2";
char zone3Message[50] = "zone3";

bool zone0newMessageAvailable = false;
bool zone1newMessageAvailable = false;
bool zone2newMessageAvailable = false;
bool zone3newMessageAvailable = false;

char const *wifiAPpassword  = "12345678";

unsigned long currentMillis;
unsigned long previousMillis = 0; 
String curTimeZone0 = "00:00";
String curTimeZone1 = "00:00";
String curTimeZone2 = "00:00";
String curTimeZone3 = "00:00";

// Initialize NTP
String ntpServerAddress = "pool.ntp.org";
int ntpTimeZone = 3;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerAddress.c_str());

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server,&dns);

bool restartESP         = false;

bool zone0Test          = true;
bool zone1Test          = true;
bool zone2Test          = true;
bool zone3Test          = true;
bool ipInfo             = true;
bool allTestsFinish     = false;

bool newConfigAvailable = false;

//flag for saving data
bool shouldSaveConfig   = false;
//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig      = true;
}

// variables for get weather func
static uint32_t owmLastTime = 0;
int             owmUpdateInterval = 60;
String          owmApiToken;
String          owmUnitsFormat;
String          owmCity;

// Home Assistant client variables
String haAddr, haApiHttpType, haApiToken;
int haUpdateInterval        = 60;
int haApiPort               = 8123;
static uint32_t haLastTime  = 0;


// Convert scrollAlig in String to textPosition_t type
textPosition_t stringToTextPositionT(String val) {
  if(val == "PA_CENTER") return PA_CENTER;
  if(val == "PA_LEFT") return PA_LEFT;
  if(val == "PA_RIGHT") return PA_RIGHT;
  return PA_LEFT;
}

void applyZoneFont(int zone, String font) {
  if(font == "default") P.setFont(zone, nullptr);
  if(font == "wledFont") P.setFont(zone, wledFont);
  if(font == "wledFont_cyrillic") P.setFont(zone, wledFont_cyrillic);
  if(font == "wledSymbolFont") P.setFont(zone, wledSymbolFont);
}

textEffect_t stringToTextEffectT(String val) {
  if(val == "PA_NO_EFFECT") return PA_NO_EFFECT;
  if(val == "PA_PRINT") return PA_PRINT;
  if(val == "PA_SCROLL_UP") return PA_SCROLL_UP;
  if(val == "PA_SCROLL_DOWN") return PA_SCROLL_DOWN;
  if(val == "PA_SCROLL_LEFT") return PA_SCROLL_LEFT;
  if(val == "PA_SCROLL_RIGHT") return PA_SCROLL_RIGHT;
  if(val == "PA_SPRITE") return PA_SPRITE;
  if(val == "PA_SLICE") return PA_SLICE;
  if(val == "PA_MESH") return PA_MESH;
  if(val == "PA_FADE") return PA_FADE;
  if(val == "PA_DISSOLVE") return PA_DISSOLVE;
  if(val == "PA_BLINDS") return PA_BLINDS;
  if(val == "PA_RANDOM") return PA_RANDOM;
  if(val == "PA_WIPE") return PA_WIPE;
  if(val == "PA_WIPE_CURSOR") return PA_WIPE_CURSOR;
  if(val == "PA_SCAN_HORIZ") return PA_SCAN_HORIZ;
  if(val == "PA_SCAN_HORIZX") return PA_SCAN_HORIZX;
  if(val == "PA_SCAN_VERT") return PA_SCAN_VERT;
  if(val == "PA_SCAN_VERTX") return PA_SCAN_VERTX;
  if(val == "PA_OPENING") return PA_OPENING;
  if(val == "PA_OPENING_CURSOR") return PA_OPENING_CURSOR;
  if(val == "PA_CLOSING") return PA_CLOSING;
  if(val == "PA_CLOSING_CURSOR") return PA_CLOSING_CURSOR;
  if(val == "PA_SCROLL_UP_LEFT") return PA_SCROLL_UP_LEFT;
  if(val == "PA_SCROLL_UP_RIGHT") return PA_SCROLL_UP_RIGHT;
  if(val == "PA_SCROLL_DOWN_LEFT") return PA_SCROLL_DOWN_LEFT;
  if(val == "PA_SCROLL_DOWN_RIGHT") return PA_SCROLL_DOWN_RIGHT;
  if(val == "PA_GROW_UP") return PA_GROW_UP;
  if(val == "PA_GROW_DOWN") return PA_GROW_DOWN;
  return PA_SCROLL_LEFT;
}

// Return values for variables in html page
String processor(const String& var){
  char buffer [10];
  if(var == "workModeZone0")                return zones[0].workMode;
  if(var == "workModeZone1")                return zones[1].workMode;
  if(var == "workModeZone2")                return zones[2].workMode;
  if(var == "workModeZone3")                return zones[3].workMode;
  if(var == "zoneNumbers")                  return itoa(zoneNumbers, buffer, 10);
  if(var == "zone0Begin")                   return itoa(zones[0].begin, buffer, 10);
  if(var == "zone0End")                     return itoa(zones[0].end, buffer, 10);
  if(var == "zone1Begin")                   return itoa(zones[1].begin, buffer, 10);
  if(var == "zone1End")                     return itoa(zones[1].end, buffer, 10);
  if(var == "zone2Begin")                   return itoa(zones[2].begin, buffer, 10);
  if(var == "zone2End")                     return itoa(zones[2].end, buffer, 10);
  if(var == "zone3Begin")                   return itoa(zones[3].begin, buffer, 10);
  if(var == "zone3End")                     return itoa(zones[3].end, buffer, 10);
  if(var == "wifiSsid")                     return WiFi.SSID();
  if(var == "wifiIp")                       return WiFi.localIP().toString();
  if(var == "wifiGateway")                  return WiFi.gatewayIP().toString();
  if(var == "intensity")                    return itoa(intensity + 1, buffer, 10);
  if(var == "scrollSpeedZone0")             return itoa(zones[0].scrollSpeed, buffer, 10);
  if(var == "scrollSpeedZone1")             return itoa(zones[1].scrollSpeed, buffer, 10);
  if(var == "scrollSpeedZone2")             return itoa(zones[2].scrollSpeed, buffer, 10);
  if(var == "scrollSpeedZone3")             return itoa(zones[3].scrollSpeed, buffer, 10);
  if(var == "scrollPauseZone0")             return itoa(zones[0].scrollPause, buffer, 10);
  if(var == "scrollPauseZone1")             return itoa(zones[1].scrollPause, buffer, 10);
  if(var == "scrollPauseZone2")             return itoa(zones[2].scrollPause, buffer, 10);
  if(var == "scrollPauseZone3")             return itoa(zones[3].scrollPause, buffer, 10);
  if(var == "scrollAlignZone0")             return zones[0].scrollAlign;
  if(var == "scrollAlignZone1")             return zones[1].scrollAlign;
  if(var == "scrollAlignZone2")             return zones[2].scrollAlign;
  if(var == "scrollAlignZone3")             return zones[3].scrollAlign;
  if(var == "scrollEffectZone0In")          return zones[0].scrollEffectIn;
  if(var == "scrollEffectZone1In")          return zones[1].scrollEffectIn;
  if(var == "scrollEffectZone2In")          return zones[2].scrollEffectIn;
  if(var == "scrollEffectZone3In")          return zones[3].scrollEffectIn;
  if(var == "scrollEffectZone0Out")         return zones[0].scrollEffectOut;
  if(var == "scrollEffectZone1Out")         return zones[1].scrollEffectOut;
  if(var == "scrollEffectZone2Out")         return zones[2].scrollEffectOut;
  if(var == "scrollEffectZone3Out")         return zones[3].scrollEffectOut;
  if(var == "mqttServerAddress")            return mqttServerAddress;
  if(var == "mqttServerPort")               return itoa(mqttServerPort, buffer, 10);
  if(var == "mqttUsername")                 return mqttUsername;
  if(var == "mqttPassword")                 return mqttPassword;
  if(var == "mqttDevicePrefix")             return MQTTGlobalPrefix;
  if(var == "mqttTextTopicZone0")           return MQTTZones[0].message;
  if(var == "mqttTextTopicZone1")           return MQTTZones[1].message;
  if(var == "mqttTextTopicZone2")           return MQTTZones[2].message;
  if(var == "mqttTextTopicZone3")           return MQTTZones[3].message;
  if(var == "mqttPostfixZone0")             return zones[0].mqttPostfix;
  if(var == "mqttPostfixZone1")             return zones[1].mqttPostfix;
  if(var == "mqttPostfixZone2")             return zones[2].mqttPostfix;
  if(var == "mqttPostfixZone3")             return zones[3].mqttPostfix;
  if(var == "ntpTimeZone")                  return itoa(ntpTimeZone, buffer, 10);
  if(var == "clockDisplayFormatZone0")      return zones[0].clockDisplayFormat;
  if(var == "clockDisplayFormatZone1")      return zones[1].clockDisplayFormat;
  if(var == "clockDisplayFormatZone2")      return zones[2].clockDisplayFormat;
  if(var == "clockDisplayFormatZone3")      return zones[3].clockDisplayFormat;
  if(var == "owmApiToken")                  return owmApiToken;
  if(var == "owmUnitsFormat")               return owmUnitsFormat;
  if(var == "owmUpdateInterval")            return itoa(owmUpdateInterval, buffer, 10);
  if(var == "owmWhatToDisplayZone0")        return zones[0].owmWhatToDisplay;
  if(var == "owmWhatToDisplayZone1")        return zones[1].owmWhatToDisplay;
  if(var == "owmWhatToDisplayZone2")        return zones[2].owmWhatToDisplay;
  if(var == "owmWhatToDisplayZone3")        return zones[3].owmWhatToDisplay;
  if(var == "owmCity")                      return owmCity;
  if(var == "fontZone0")                    return zones[0].font;
  if(var == "fontZone1")                    return zones[1].font;
  if(var == "fontZone2")                    return zones[2].font;
  if(var == "fontZone3")                    return zones[3].font;
  if(var == "haAddr")                       return haAddr;
  if(var == "haApiToken")                   return haApiToken;
  if(var == "haApiHttpType")                return haApiHttpType;
  if(var == "haApiPort")                    return itoa(haApiPort, buffer, 10);
  if(var == "haSensorIdZone0")              return zones[0].haSensorId;
  if(var == "haSensorIdZone1")              return zones[1].haSensorId;
  if(var == "haSensorIdZone2")              return zones[2].haSensorId;
  if(var == "haSensorIdZone3")              return zones[3].haSensorId;
  if(var == "haSensorPostfixZone0")         return zones[0].haSensorPostfix;
  if(var == "haSensorPostfixZone1")         return zones[1].haSensorPostfix;
  if(var == "haSensorPostfixZone2")         return zones[2].haSensorPostfix;
  if(var == "haSensorPostfixZone3")         return zones[3].haSensorPostfix;
  if(var == "haUpdateInterval")             return itoa(haUpdateInterval, buffer, 10);
  if(var == "charspacingZone0")             return itoa(zones[0].charspacing, buffer, 10);
  if(var == "charspacingZone1")             return itoa(zones[1].charspacing, buffer, 10);
  if(var == "charspacingZone2")             return itoa(zones[2].charspacing, buffer, 10);
  if(var == "charspacingZone3")             return itoa(zones[3].charspacing, buffer, 10);
  if(var == "firmwareVer")                  return firmwareVer;
  if(var == "deviceName")                   return deviceName;
  if(var == "disableServiceMessages")       return disableServiceMessages;
  if(var == "disableDotsBlink")             return disableDotsBlink;
  if(var == "ds18b20Enable")                return ds18b20Enable;
  if(var == "ds18b20UpdateInterval")        return itoa(ds18b20UpdateInterval, buffer, 10);
  if(var == "ds18b20UnitsFormat")           return ds18b20UnitsFormat;
  if(var == "ds18b20PostfixZone0")          return zones[0].ds18b20Postfix;
  if(var == "ds18b20PostfixZone1")          return zones[1].ds18b20Postfix;
  if(var == "ds18b20PostfixZone2")          return zones[2].ds18b20Postfix;
  if(var == "ds18b20PostfixZone3")          return zones[3].ds18b20Postfix;
  return String();
}

void MQTTPublishHADiscovry(String zone, String device_type) {
  DynamicJsonDocument  root(2048);
  char topic_config[100];
  //char buffer[1024];
  boolean result;

  // device block
  JsonObject device                 = root.createNestedObject("dev");
  JsonArray arrDevice               = device.createNestedArray("ids"); 
  arrDevice.add(MQTTGlobalPrefix); 
  device["mf"]                      = "widapro";
  device["mdl"]                     = "wledPixel-v2"; 
  device["name"]                    = MQTTGlobalPrefix;
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
    arrOptions.add("mqttClient"); 
    arrOptions.add("manualInput"); 
    arrOptions.add("wallClock"); 
    arrOptions.add("owmWeather"); 
    arrOptions.add("haClient"); 
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
    root["stat_t"]              = "wledPixel-" + shortMACaddr + "/temperature";
    root["entity_category"]     = "config";
    root["device_class"]        = "temperature";
    root["unit_of_measurement"] = "°C";
  }  

  result = mqttClient.beginPublish(topic_config, measureJson(root), true);
  if( result == false ) Serial.println( "MQTT HA config error begin!" );  
  serializeJson(root, mqttClient);
  result = mqttClient.endPublish();
  if( result == false ) Serial.println( "MQTT HA config error end!" ); 

}

void MQTTPublishState() {
  if (mqttClient.connected()) {
    DynamicJsonDocument doc(1024);
    char buffer[1024];
    
    doc["status"] = "online";
    doc["power"] = power;
    doc["brightness"] = intensity;
    doc["wifiSsid"] = WiFi.SSID();
    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      doc["zone" + String(n)]["workMode"]    = zones[n].workMode;
      doc["zone" + String(n)]["charspacing"] = zones[n].charspacing;
      doc["zone" + String(n)]["scrollAlign"] = zones[n].scrollAlign;
      doc["zone" + String(n)]["scrollPause"] = zones[n].scrollPause;
      doc["zone" + String(n)]["scrollSpeed"] = zones[n].scrollSpeed;
      doc["zone" + String(n)]["scrollEffectIn"] = zones[n].scrollEffectIn;
      doc["zone" + String(n)]["scrollEffectOut"] = zones[n].scrollEffectOut;
    }

    serializeJson(doc, Serial);
    size_t n = serializeJson(doc, buffer);
    
    Serial.printf("\nMQTT publish device state: ");
    bool published = mqttClient.publish(MQTTStateTopic.c_str(), buffer, n);
    if (published) {
      Serial.println("OK");
    } else {
      Serial.println("fail");
    }
  }
}

void writeAllVarsToConfFile() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("!!! ERROR !!! failed to open config file for writing");
    return;
  }

  DynamicJsonDocument doc(3072);
  deserializeJson(doc, configFile);
  configFile.close();
  doc["zoneNumbers"] = String(zoneNumbers);
  doc["zone0Begin"] = String(zones[0].begin);
  doc["zone0End"] = String(zones[0].end);
  doc["zone1Begin"] = String(zones[1].begin);
  doc["zone1End"] = String(zones[1].end);
  doc["zone2Begin"] = String(zones[2].begin);
  doc["zone2End"] = String(zones[2].end);
  doc["zone3Begin"] = String(zones[3].begin);
  doc["zone3End"] = String(zones[3].end);
  // zone0
  doc["workModeZone0"] = zones[0].workMode;
  doc["scrollSpeedZone0"] = String(zones[0].scrollSpeed);
  doc["scrollPauseZone0"] = String(zones[0].scrollPause);
  doc["scrollAlignZone0"] = zones[0].scrollAlign;
  doc["scrollEffectZone0In"] = zones[0].scrollEffectIn;
  doc["scrollEffectZone0Out"] = zones[0].scrollEffectOut;
  doc["mqttPostfixZone0"] = zones[0].mqttPostfix;
  doc["clockDisplayFormatZone0"] = zones[0].clockDisplayFormat;
  doc["owmWhatToDisplayZone0"] = zones[0].owmWhatToDisplay;
  doc["fontZone0"] = zones[0].font;
  doc["charspacingZone0"] = String(zones[0].charspacing);
  doc["haSensorIdZone0"] = zones[0].haSensorId;
  doc["haSensorPostfixZone0"] = zones[0].haSensorPostfix;
  doc["mqttTextTopicZone0"] = MQTTZones[0].message;
  doc["ds18b20PostfixZone0"] = zones[0].ds18b20Postfix;
  // zone1
  doc["workModeZone1"] = zones[1].workMode;
  doc["scrollSpeedZone1"] = String(zones[1].scrollSpeed);
  doc["scrollPauseZone1"] = String(zones[1].scrollPause);
  doc["scrollAlignZone1"] = zones[1].scrollAlign;
  doc["scrollEffectZone1In"] = zones[1].scrollEffectIn;
  doc["scrollEffectZone1Out"] = zones[1].scrollEffectOut;
  doc["mqttPostfixZone1"] = zones[1].mqttPostfix;
  doc["clockDisplayFormatZone1"] = zones[1].clockDisplayFormat;
  doc["owmWhatToDisplayZone1"] = zones[1].owmWhatToDisplay;
  doc["fontZone1"] = zones[1].font;
  doc["charspacingZone1"] = String(zones[1].charspacing);
  doc["haSensorIdZone1"] = zones[1].haSensorId;
  doc["haSensorPostfixZone1"] = zones[1].haSensorPostfix;
  doc["mqttTextTopicZone1"] = MQTTZones[1].message;
  doc["ds18b20PostfixZone1"] = zones[1].ds18b20Postfix;
  // zone2
  doc["workModeZone2"] = zones[2].workMode;
  doc["scrollSpeedZone2"] = String(zones[2].scrollSpeed);
  doc["scrollPauseZone2"] = String(zones[2].scrollPause);
  doc["scrollAlignZone2"] = zones[2].scrollAlign;
  doc["scrollEffectZone2In"] = zones[2].scrollEffectIn;
  doc["scrollEffectZone2Out"] = zones[2].scrollEffectOut;
  doc["mqttPostfixZone2"] = zones[2].mqttPostfix;
  doc["clockDisplayFormatZone2"] = zones[2].clockDisplayFormat;
  doc["owmWhatToDisplayZone2"] = zones[2].owmWhatToDisplay;
  doc["fontZone2"] = zones[2].font;
  doc["charspacingZone2"] = String(zones[2].charspacing);
  doc["haSensorIdZone2"] = zones[2].haSensorId;
  doc["haSensorPostfixZone2"] = zones[2].haSensorPostfix;
  doc["mqttTextTopicZone2"] = MQTTZones[2].message;
  doc["ds18b20PostfixZone2"] = zones[2].ds18b20Postfix;
  // zone3
  doc["workModeZone3"] = zones[3].workMode;
  doc["scrollSpeedZone3"] = String(zones[3].scrollSpeed);
  doc["scrollPauseZone3"] = String(zones[3].scrollPause);
  doc["scrollAlignZone3"] = zones[3].scrollAlign;
  doc["scrollEffectZone3In"] = zones[3].scrollEffectIn;
  doc["scrollEffectZone3Out"] = zones[3].scrollEffectOut;
  doc["mqttPostfixZone3"] = zones[3].mqttPostfix;
  doc["clockDisplayFormatZone3"] = zones[3].clockDisplayFormat;
  doc["owmWhatToDisplayZone3"] = zones[3].owmWhatToDisplay;
  doc["fontZone3"] = zones[3].font;
  doc["charspacingZone3"] = String(zones[3].charspacing);
  doc["haSensorIdZone3"] = zones[3].haSensorId;
  doc["haSensorPostfixZone3"] = zones[3].haSensorPostfix;
  doc["mqttTextTopicZone3"] = MQTTZones[3].message;
  doc["ds18b20PostfixZone3"] = zones[3].ds18b20Postfix;  
  
  doc["mqttServerAddress"] = mqttServerAddress;
  doc["mqttServerPort"] = String(mqttServerPort);
  doc["mqttUsername"] = mqttUsername;
  doc["mqttPassword"] = mqttPassword;
  doc["ntpTimeZone"] = String(ntpTimeZone);
  doc["owmApiToken"] = owmApiToken;
  doc["owmUnitsFormat"] = owmUnitsFormat;
  doc["owmUpdateInterval"] = String(owmUpdateInterval);
  doc["owmCity"] = owmCity;
  doc["haAddr"] = haAddr;
  doc["haApiToken"] = haApiToken;
  doc["haApiHttpType"] = haApiHttpType;
  doc["haApiPort"] = String(haApiPort);
  doc["deviceName"] = deviceName;
  doc["disableServiceMessages"] = disableServiceMessages;
  doc["disableDotsBlink"] = disableDotsBlink;
  doc["intensity"] = String(intensity);
  doc["ds18b20Enable"] = ds18b20Enable;
  doc["ds18b20UpdateInterval"] = String(ds18b20UpdateInterval);
  doc["ds18b20UnitsFormat"] = ds18b20UnitsFormat;

  configFile = LittleFS.open("/config.json", "w");
  serializeJson(doc, configFile);
  configFile.close();

  MQTTPublishState();
}

void ConfigFile_Read_Variable() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("!!! ERROR !!! failed to open config file for writing");
    return;
  }

  DynamicJsonDocument doc(3072);
  deserializeJson(doc, configFile);
  // print config file to serial
  serializeJsonPretty(doc, Serial);
  configFile.close();

  JsonObject postObj = doc.as<JsonObject>();
  if(postObj[F("workModeZone0")])             zones[0].workMode = postObj[F("workModeZone0")].as<String>();
  if(postObj[F("workModeZone1")])             zones[1].workMode = postObj[F("workModeZone1")].as<String>();
  if(postObj[F("workModeZone2")])             zones[2].workMode = postObj[F("workModeZone2")].as<String>();
  if(postObj[F("workModeZone3")])             zones[3].workMode = postObj[F("workModeZone3")].as<String>();
  if(postObj[F("zoneNumbers")])               zoneNumbers = postObj[F("zoneNumbers")].as<uint8_t>();
  if(postObj[F("zone0Begin")])                zones[0].begin = postObj[F("zone0Begin")].as<uint8_t>();
  if(postObj[F("zone0End")])                  zones[0].end = postObj[F("zone0End")].as<uint8_t>();
  if(postObj[F("zone1Begin")])                zones[1].begin = postObj[F("zone1Begin")].as<uint8_t>();
  if(postObj[F("zone1End")])                  zones[1].end = postObj[F("zone1End")].as<uint8_t>();
  if(postObj[F("zone2Begin")])                zones[2].begin = postObj[F("zone2Begin")].as<uint8_t>();
  if(postObj[F("zone2End")])                  zones[2].end = postObj[F("zone2End")].as<uint8_t>();
  if(postObj[F("zone3Begin")])                zones[3].begin = postObj[F("zone3Begin")].as<uint8_t>();
  if(postObj[F("zone3End")])                  zones[3].end = postObj[F("zone3End")].as<uint8_t>();
  if(postObj[F("intensity")])                 intensity = postObj[F("intensity")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone0")])          zones[0].scrollSpeed = postObj[F("scrollSpeedZone0")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone1")])          zones[1].scrollSpeed = postObj[F("scrollSpeedZone1")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone2")])          zones[2].scrollSpeed = postObj[F("scrollSpeedZone2")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone3")])          zones[3].scrollSpeed = postObj[F("scrollSpeedZone3")].as<uint8_t>();
  if(postObj[F("scrollPauseZone0")])          zones[0].scrollPause = postObj[F("scrollPauseZone0")].as<uint16_t>();
  if(postObj[F("scrollPauseZone1")])          zones[1].scrollPause = postObj[F("scrollPauseZone1")].as<uint16_t>();
  if(postObj[F("scrollPauseZone2")])          zones[2].scrollPause = postObj[F("scrollPauseZone2")].as<uint16_t>();
  if(postObj[F("scrollPauseZone3")])          zones[3].scrollPause = postObj[F("scrollPauseZone3")].as<uint16_t>();
  if(postObj[F("scrollAlignZone0")])          zones[0].scrollAlign = postObj[F("scrollAlignZone0")].as<String>();
  if(postObj[F("scrollAlignZone1")])          zones[1].scrollAlign = postObj[F("scrollAlignZone1")].as<String>();
  if(postObj[F("scrollAlignZone2")])          zones[2].scrollAlign = postObj[F("scrollAlignZone2")].as<String>();
  if(postObj[F("scrollAlignZone3")])          zones[3].scrollAlign = postObj[F("scrollAlignZone3")].as<String>();
  if(postObj[F("scrollEffectZone0In")])       zones[0].scrollEffectIn = postObj[F("scrollEffectZone0In")].as<String>();
  if(postObj[F("scrollEffectZone1In")])       zones[1].scrollEffectIn = postObj[F("scrollEffectZone1In")].as<String>();
  if(postObj[F("scrollEffectZone2In")])       zones[2].scrollEffectIn = postObj[F("scrollEffectZone2In")].as<String>();
  if(postObj[F("scrollEffectZone3In")])       zones[3].scrollEffectIn = postObj[F("scrollEffectZone3In")].as<String>();
  if(postObj[F("scrollEffectZone0Out")])      zones[0].scrollEffectOut = postObj[F("scrollEffectZone0Out")].as<String>();
  if(postObj[F("scrollEffectZone1Out")])      zones[1].scrollEffectOut = postObj[F("scrollEffectZone1Out")].as<String>();
  if(postObj[F("scrollEffectZone2Out")])      zones[2].scrollEffectOut = postObj[F("scrollEffectZone2Out")].as<String>();
  if(postObj[F("scrollEffectZone3Out")])      zones[3].scrollEffectOut = postObj[F("scrollEffectZone3Out")].as<String>();
  if(postObj[F("mqttServerAddress")])         mqttServerAddress = postObj[F("mqttServerAddress")].as<String>();
  if(postObj[F("mqttServerPort")])            mqttServerPort = postObj[F("mqttServerPort")].as<int>();
  if(postObj[F("mqttUsername")])              mqttUsername = postObj[F("mqttUsername")].as<String>();
  if(postObj[F("mqttPassword")])              mqttPassword = postObj[F("mqttPassword")].as<String>();
  if(postObj[F("mqttTextTopicZone0")])        MQTTZones[0].message = postObj[F("mqttTextTopicZone0")].as<String>();
  if(postObj[F("mqttTextTopicZone1")])        MQTTZones[1].message = postObj[F("mqttTextTopicZone1")].as<String>();
  if(postObj[F("mqttTextTopicZone2")])        MQTTZones[2].message = postObj[F("mqttTextTopicZone2")].as<String>();
  if(postObj[F("mqttTextTopicZone3")])        MQTTZones[3].message = postObj[F("mqttTextTopicZone3")].as<String>();
  if(postObj[F("mqttPostfixZone0")])          zones[0].mqttPostfix = postObj[F("mqttPostfixZone0")].as<String>();
  if(postObj[F("mqttPostfixZone1")])          zones[1].mqttPostfix = postObj[F("mqttPostfixZone1")].as<String>();
  if(postObj[F("mqttPostfixZone2")])          zones[2].mqttPostfix = postObj[F("mqttPostfixZone2")].as<String>();
  if(postObj[F("mqttPostfixZone3")])          zones[3].mqttPostfix = postObj[F("mqttPostfixZone3")].as<String>();
  if(postObj[F("ntpTimeZone")])               ntpTimeZone = postObj[F("ntpTimeZone")].as<int>();
  if(postObj[F("clockDisplayFormatZone0")])   zones[0].clockDisplayFormat = postObj[F("clockDisplayFormatZone0")].as<String>();
  if(postObj[F("clockDisplayFormatZone1")])   zones[1].clockDisplayFormat = postObj[F("clockDisplayFormatZone1")].as<String>();
  if(postObj[F("clockDisplayFormatZone2")])   zones[2].clockDisplayFormat = postObj[F("clockDisplayFormatZone2")].as<String>();
  if(postObj[F("clockDisplayFormatZone3")])   zones[3].clockDisplayFormat = postObj[F("clockDisplayFormatZone3")].as<String>();
  if(postObj[F("owmApiToken")])               owmApiToken = postObj[F("owmApiToken")].as<String>();
  if(postObj[F("owmUnitsFormat")])            owmUnitsFormat = postObj[F("owmUnitsFormat")].as<String>();
  if(postObj[F("owmUpdateInterval")])         owmUpdateInterval = postObj[F("owmUpdateInterval")].as<int>();
  if(postObj[F("owmCity")])                   owmCity = postObj[F("owmCity")].as<String>();
  if(postObj[F("owmWhatToDisplayZone0")])     zones[0].owmWhatToDisplay = postObj[F("owmWhatToDisplayZone0")].as<String>();
  if(postObj[F("owmWhatToDisplayZone1")])     zones[1].owmWhatToDisplay = postObj[F("owmWhatToDisplayZone1")].as<String>();
  if(postObj[F("owmWhatToDisplayZone2")])     zones[2].owmWhatToDisplay = postObj[F("owmWhatToDisplayZone2")].as<String>();
  if(postObj[F("owmWhatToDisplayZone3")])     zones[3].owmWhatToDisplay = postObj[F("owmWhatToDisplayZone3")].as<String>();
  if(postObj[F("fontZone0")])                 zones[0].font = postObj[F("fontZone0")].as<String>();
  if(postObj[F("fontZone1")])                 zones[1].font = postObj[F("fontZone1")].as<String>();
  if(postObj[F("fontZone2")])                 zones[2].font = postObj[F("fontZone2")].as<String>();
  if(postObj[F("fontZone3")])                 zones[3].font = postObj[F("fontZone3")].as<String>();
  if(postObj[F("haAddr")])                    haAddr = postObj[F("haAddr")].as<String>();
  if(postObj[F("haApiToken")])                haApiToken = postObj[F("haApiToken")].as<String>();
  if(postObj[F("haApiHttpType")])             haApiHttpType = postObj[F("haApiHttpType")].as<String>();
  if(postObj[F("haApiPort")])                 haApiPort = postObj[F("haApiPort")].as<int>();
  if(postObj[F("haSensorIdZone0")])           zones[0].haSensorId = postObj[F("haSensorIdZone0")].as<String>();
  if(postObj[F("haSensorIdZone1")])           zones[1].haSensorId = postObj[F("haSensorIdZone1")].as<String>();
  if(postObj[F("haSensorIdZone2")])           zones[2].haSensorId = postObj[F("haSensorIdZone2")].as<String>();
  if(postObj[F("haSensorIdZone3")])           zones[3].haSensorId = postObj[F("haSensorIdZone3")].as<String>();
  if(postObj[F("haSensorPostfixZone0")])      zones[0].haSensorPostfix = postObj[F("haSensorPostfixZone0")].as<String>();
  if(postObj[F("haSensorPostfixZone1")])      zones[1].haSensorPostfix = postObj[F("haSensorPostfixZone1")].as<String>();
  if(postObj[F("haSensorPostfixZone2")])      zones[2].haSensorPostfix = postObj[F("haSensorPostfixZone2")].as<String>();
  if(postObj[F("haSensorPostfixZone3")])      zones[3].haSensorPostfix = postObj[F("haSensorPostfixZone3")].as<String>();
  if(postObj[F("haUpdateInterval")])          haUpdateInterval = postObj[F("haUpdateInterval")].as<int>();
  if(postObj[F("charspacingZone0")])          zones[0].charspacing = postObj[F("charspacingZone0")].as<int>();
  if(postObj[F("charspacingZone1")])          zones[1].charspacing = postObj[F("charspacingZone1")].as<int>();
  if(postObj[F("charspacingZone2")])          zones[2].charspacing = postObj[F("charspacingZone2")].as<int>();
  if(postObj[F("charspacingZone3")])          zones[3].charspacing = postObj[F("charspacingZone3")].as<int>();
  if(postObj[F("power")])                     power = postObj[F("power")].as<String>();
  if(postObj[F("deviceName")])                deviceName = postObj[F("deviceName")].as<String>();
  if(postObj[F("disableServiceMessages")])    disableServiceMessages = postObj[F("disableServiceMessages")].as<String>();
  if(postObj[F("disableDotsBlink")])          disableDotsBlink = postObj[F("disableDotsBlink")].as<String>();
  if(postObj[F("ds18b20Enable")])             ds18b20Enable = postObj[F("ds18b20Enable")].as<String>();
  if(postObj[F("ds18b20UpdateInterval")])     ds18b20UpdateInterval = postObj[F("ds18b20UpdateInterval")].as<int>();
  if(postObj[F("ds18b20UnitsFormat")])        ds18b20UnitsFormat = postObj[F("ds18b20UnitsFormat")].as<String>();
  if(postObj[F("ds18b20PostfixZone0")])       zones[0].ds18b20Postfix = postObj[F("ds18b20PostfixZone0")].as<String>();
  if(postObj[F("ds18b20PostfixZone1")])       zones[1].ds18b20Postfix = postObj[F("ds18b20PostfixZone1")].as<String>();
  if(postObj[F("ds18b20PostfixZone2")])       zones[2].ds18b20Postfix = postObj[F("ds18b20PostfixZone2")].as<String>();
  if(postObj[F("ds18b20PostfixZone3")])       zones[3].ds18b20Postfix = postObj[F("ds18b20PostfixZone3")].as<String>();
}

void zoneNewMessage(int zone, String newMessage, String postfix) {
  if ( zone == 0) {
    strcpy(zone0Message, (newMessage + postfix).c_str());
    zone0newMessageAvailable = true;
  }
  if ( zone == 1) {
    strcpy(zone1Message, (newMessage + postfix).c_str());
    zone1newMessageAvailable = true;
  }
  if ( zone == 2) {
    strcpy(zone2Message, (newMessage + postfix).c_str());
    zone2newMessageAvailable = true;
  }
  if ( zone == 3) {
    strcpy(zone3Message, (newMessage + postfix).c_str());
    zone3newMessageAvailable = true;
  }
}

// check a string to see if it is numeric
bool isNumeric(String str){
  for(byte i=0;i<str.length();i++) {
    if(isDigit(str.charAt(i))) return true;
  }
  return false;
}

void MQTTCallback(char* topic, byte* payload, int length) {
  String topicStr(topic);
  String PayloadString = "";

  for (int i = 0; i < length; i++) { PayloadString = PayloadString + (char)payload[i]; }

  Serial.printf("\nNew message arrived");  
  Serial.printf("\nTopic: %s\n", topic);
  Serial.println(PayloadString);

  if (topicStr == MQTTIntensity) {
    if (isNumeric(PayloadString)) {
      intensity = PayloadString.toInt()-1;
      P.setIntensity(intensity);
    } else {
      Serial.print("Supports are only numeric values");
    }
    
  }
  if (topicStr == MQTTPower) {
    if(PayloadString == "on" || PayloadString == "ON" || PayloadString == "TRUE") {
      P.displayShutdown(0);
      power = "ON";
    }
    if(PayloadString == "off" || PayloadString == "OFF" || PayloadString == "FALSE") {
      P.displayShutdown(1);
      power = "OFF";
    }
  }
  
  for ( uint8_t n = 0; n < zoneNumbers; n++) {
    if(zones[n].workMode == "mqttClient" && topicStr == MQTTZones[n].message) zoneNewMessage(n, PayloadString.c_str(), zones[n].mqttPostfix);

    if (topicStr == MQTTZones[n].scrollEffectIn) {
      zones[n].scrollEffectIn = PayloadString.c_str();
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
    }
    if (topicStr == MQTTZones[n].scrollEffectOut) {
      zones[n].scrollEffectOut = PayloadString.c_str();
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
    }
    if (topicStr == MQTTZones[n].scrollSpeed) {
      zones[n].scrollSpeed = PayloadString.toInt();
      P.setSpeed(n, zones[n].scrollSpeed);
    }
    if (topicStr == MQTTZones[n].scrollPause) {
      zones[n].scrollPause = PayloadString.toInt();
      P.setPause(n, zones[n].scrollPause);
    }
    if (topicStr == MQTTZones[n].scrollAllign) {
      zones[n].scrollAlign = PayloadString.c_str();
      P.setTextAlignment(n, stringToTextPositionT(zones[n].scrollAlign));
    }
    if (topicStr == MQTTZones[n].charspacing) {
      zones[n].charspacing = PayloadString.toInt();
      P.setCharSpacing(n, zones[n].charspacing);
    }
    if (topicStr == MQTTZones[n].workMode) {
      zones[n].workMode = PayloadString.c_str();
    }
  }
  writeAllVarsToConfFile();
}

boolean reconnect() {
  //mqttClient.disconnect();
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);

  // Attempt to connect
  if (mqttClient.connect(MQTTGlobalPrefix.c_str(), mqttUsername.c_str(),mqttPassword.c_str())) {
    MQTTPublishHADiscovry("0", "light");
    MQTTPublishState();

    Serial.printf("\nMQTT subscribe objects");
    mqttClient.subscribe((char*) MQTTIntensity.c_str());
    mqttClient.subscribe((char*) MQTTPower.c_str());
    
    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      MQTTPublishHADiscovry(String(n), "scrollAlign");
      MQTTPublishHADiscovry(String(n), "charspacing");
      MQTTPublishHADiscovry(String(n), "workMode");
      MQTTPublishHADiscovry(String(n), "scrollPause");
      MQTTPublishHADiscovry(String(n), "scrollSpeed");
      MQTTPublishHADiscovry(String(n), "scrollEffectIn");
      MQTTPublishHADiscovry(String(n), "scrollEffectOut");
      MQTTPublishHADiscovry(String(n), "ds18b20");

      if (zones[n].workMode == "mqttClient" && MQTTZones[n].message != "" && MQTTZones[n].message != " ") mqttClient.subscribe((char*) MQTTZones[n].message.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollEffectIn.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollEffectOut.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollSpeed.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollPause.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollAllign.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].charspacing.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].workMode.c_str());
    }
  }
  else
  {
    Serial.printf("\nfailed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 5 seconds");
  }
  return mqttClient.connected();
}

void setupLittleFS() {
  Serial.println("Start setup Filesystem...");
  if (LittleFS.begin()) {
    Serial.println("Filesystem mounted");
    
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      configFile.close();
      Serial.println("config file exist");
    } else {
      File configFile = LittleFS.open("/config.json", "w");
      configFile.close();
      Serial.println("Config file was created");
    }
  } else {
    Serial.println("!!! ERROR !!! Filesystem not mounted...");
    LittleFS.format();
  }
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
    if (zone0newMessageAvailable && P.getZoneStatus(0)) {
        Serial.printf("\nzone0Message availabel: %s", zone0Message);
        zone0newMessageAvailable = false;
        P.setTextBuffer(0, zone0Message);
        P.displayReset(0);
    }

    if (zone1newMessageAvailable && P.getZoneStatus(1)) {
        Serial.printf("\nzone1Message availabel: %s", zone1Message);
        zone1newMessageAvailable = false;
        P.setTextBuffer(1, zone1Message);
        P.displayReset(1);
    }

    if (zone2newMessageAvailable && P.getZoneStatus(2)) {
        Serial.printf("\nzone2Message availabel: %s", zone2Message);
        zone2newMessageAvailable = false;
        P.setTextBuffer(2, zone2Message);
        P.displayReset(2);
    }

    if (zone3newMessageAvailable && P.getZoneStatus(3)) {
        Serial.printf("\nzone3Message availabel: %s", zone3Message);
        zone3newMessageAvailable = false;
        P.setTextBuffer(3, zone3Message);
        P.displayReset(3);
    }
  }
}

String httpsRequest(String addr, int port, String path, String token, bool https) {
  String payload;
  bool httpBegin;

  static BearSSL::WiFiClientSecure httpClientSsl;
  HTTPClient *httpClient = nullptr; 
  httpClient = new HTTPClient;
  httpClientSsl.setInsecure();
  httpClientSsl.setBufferSizes(2048, 2048);
  httpClientSsl.setTimeout(2000);
    
  if (https) httpBegin = httpClient->begin(httpClientSsl, addr, port, path, true);
  if (!https) httpBegin = httpClient->begin(espClient, addr, port, path, false);
  if (httpBegin) {
    httpClient->addHeader("Authorization", token);
    int httpCode = httpClient->GET();
    if (httpCode == HTTP_CODE_OK) {
      payload = httpClient->getString();
    } else {
      Serial.printf("api error: %s", httpClient->errorToString(httpCode).c_str());
      payload = "err" + httpCode;
    }
  } else { 
    Serial.print("\napi error start connection");
    payload = "err";
  }
  httpClient->end();
  delete httpClient;
  return payload;
}

String openWetherMapGetWeather(String whatToDisplay) {
  String payload = httpsRequest("api.openweathermap.org", 443, "/data/2.5/weather?q=" + owmCity + "&units=" + owmUnitsFormat + "&appid=" + owmApiToken, "", true);
  if (payload != "err") {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);
    JsonObject postObj = doc.as<JsonObject>();
    if (whatToDisplay == "owmHumidity") return postObj[F("main")][F("humidity")].as<String>() + "%";
    if (whatToDisplay == "owmPressure") return postObj[F("main")][F("pressure")].as<String>();
    if (whatToDisplay == "owmWindSpeed") return postObj[F("wind")][F("speed")].as<String>();
    if (whatToDisplay == "owmTemperature") {
      String owmTemp = String((int)(postObj[F("main")][F("temp")].as<float>())) + "°";
      if(owmUnitsFormat == "metric") owmTemp = owmTemp + "C";
      if(owmUnitsFormat == "imperial") owmTemp = owmTemp + "F";
      return owmTemp;
    };
    if (whatToDisplay == "owmWeatherIcon") {
      String owmWeatherIcon = postObj[F("weather")][0][F("icon")].as<String>();
      if(owmWeatherIcon == "01d") owmWeatherIcon = "S";                             // 83   - 'S'   Sun day
      if(owmWeatherIcon == "01n") owmWeatherIcon = "M";                             // 77   - 'M'   Moon night
      if(owmWeatherIcon == "02d") owmWeatherIcon = "s";                             // 115  - 's'   few clouds sun
      if(owmWeatherIcon == "02n") owmWeatherIcon = "m";                             // 109  - 'm'   few clouds night
      if(owmWeatherIcon == "03d" || owmWeatherIcon == "03n") owmWeatherIcon = "c";  // 99   - 'c'   scattered clouds
      if(owmWeatherIcon == "04d" || owmWeatherIcon == "04n") owmWeatherIcon = "C";  // 67   - 'C'   broken clouds
      if(owmWeatherIcon == "09d" || owmWeatherIcon == "09n") owmWeatherIcon = "R";  // 82   - 'R'   shower rain
      if(owmWeatherIcon == "10d" || owmWeatherIcon == "10n") owmWeatherIcon = "r";  // 114  - 'r'   rain
      if(owmWeatherIcon == "11d" || owmWeatherIcon == "11n") owmWeatherIcon = "T";  // 84   - 'T'   thunderstorm
      if(owmWeatherIcon == "13d" || owmWeatherIcon == "13n") owmWeatherIcon = "F";  // 70   - 'F'   snow
      if(owmWeatherIcon == "50d" || owmWeatherIcon == "50n") owmWeatherIcon = "f";  // 102  - 'f'   mist (fog)
      return owmWeatherIcon;
    }
  }
    return "err";
}

String haApiGet(String sensorId, String sensorPostfix) {
  bool https;
  if (haApiHttpType == "https") https = true;
  if (haApiHttpType == "http") https = false;
  String payload = httpsRequest(haAddr, haApiPort, "/api/states/" + sensorId, "Bearer " + haApiToken, https);
  if (payload != "err") {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    JsonObject postObj = doc.as<JsonObject>();
    return postObj[F("state")].as<String>() + sensorPostfix;
  }
  return "err";
}

void setup() {
  Serial.begin(115200);
  Serial.println("Start serial....");
  setupLittleFS();
  
  //WiFi.mode(WIFI_STA);
  //WiFi.persistent(true);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setAPCallback(wifiApWelcomeMessage);
  wifiManager.autoConnect(MQTTGlobalPrefix.c_str(), wifiAPpassword);
  WiFi.hostname(MQTTGlobalPrefix.c_str());
  Serial.println("");
  Serial.printf("Wifi connected to SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("Local ip: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("Subnet: %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("HostName: %s\n", MQTTGlobalPrefix.c_str());
  
  Serial.printf("MQTT Device Prefix: %s\n", MQTTGlobalPrefix.c_str());

  // Web server routings
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_index, processor);
  });
  
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_settings, processor);
  });

  //// route POST request for config, store data to config file
  server.on("/configpost", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      Serial.printf("%d params sent in\n", params);
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          if (p->name() == "zoneNumbers")  {
            zoneNumbers = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone0Begin") {
            zones[0].begin = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone0End") {
            zones[0].end = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone1Begin") {
            zones[1].begin = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone1End") {
            zones[1].end = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone2Begin") {
            zones[2].begin = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone2End") {
            zones[2].end = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone3Begin") {
            zones[3].begin = p->value().toInt();
            restartESP = true;
          }
          if (p->name() == "zone3End") {
            zones[3].end = p->value().toInt();
            restartESP = true;
          }
          // zone 0
          if (p->name() == "workModeZone0") {
            zones[0].workMode = p->value().c_str();
            if (zones[0].workMode == "owmWeather") owmLastTime = -1000000;
            if (zones[0].workMode == "mqttClient") mqttClient.disconnect();
            if (zones[0].workMode == "haClient") haLastTime = -1000000;
            if (zones[0].workMode == "wallClock") {
              timeClient.setTimeOffset(ntpTimeZone * 3600);
              timeClient.update();
              zones[0].previousMillis = -1000000;
            }
          }
          if (p->name() == "scrollSpeedZone0") { 
            zones[0].scrollSpeed = p->value().toInt();
            P.setSpeed(0, zones[0].scrollSpeed);
          }
          if (p->name() == "scrollPauseZone0") {
            zones[0].scrollPause = p->value().toInt();
            P.setPause(0, zones[0].scrollPause);
          }
          if (p->name() == "scrollAlignZone0") {
            zones[0].scrollAlign = p->value().c_str();
            P.setTextAlignment(0, stringToTextPositionT(zones[0].scrollAlign));
          }
          if (p->name() == "scrollEffectZone0In") {
            zones[0].scrollEffectIn = p->value().c_str();
            P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), stringToTextEffectT(zones[0].scrollEffectOut));
          }
          if (p->name() == "scrollEffectZone0Out") {
            zones[0].scrollEffectOut = p->value().c_str();
            P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), stringToTextEffectT(zones[0].scrollEffectOut));
          }
          if (p->name() == "mqttTextTopicZone0") {
            if (MQTTZones[0].message != p->value().c_str()) {
              MQTTZones[0].message = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "mqttPostfixZone0") {
            if (zones[0].mqttPostfix != p->value().c_str()) {
              zones[0].mqttPostfix = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "clockDisplayFormatZone0") {
            zones[0].clockDisplayFormat = p->value().c_str();
            curTimeZone0 = getCurTime(zones[0].font, zones[0].clockDisplayFormat);
          }
          if (p->name() == "owmWhatToDisplayZone0") {
            zones[0].owmWhatToDisplay = p->value().c_str();
            if(zones[0].owmWhatToDisplay == "owmWeatherIcon") {
              P.setFont(0, wledSymbolFont);
              P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), PA_NO_EFFECT);
            }
          }
          if (p->name() == "fontZone0") {
            zones[0].font = p->value().c_str();
            applyZoneFont(0, zones[0].font);
          }
          if (p->name() == "haSensorIdZone0") zones[0].haSensorId = p->value().c_str();
          if (p->name() == "charspacingZone0") {
            zones[0].charspacing = p->value().toInt();
            P.setCharSpacing(0, zones[0].charspacing);
          }
          if (p->name() == "haSensorPostfixZone0") zones[0].haSensorPostfix = p->value().c_str();
          if (p->name() == "ds18b20PostfixZone0") {
            zones[0].ds18b20Postfix = p->value().c_str();
            previousDsMillis = -1000;
          }
          // zone 1
          if (p->name() == "workModeZone1") {
            zones[1].workMode = p->value().c_str();
            if (zones[1].workMode == "owmWeather") owmLastTime = -1000000;
            if (zones[1].workMode == "mqttClient") mqttClient.disconnect();
            if (zones[1].workMode == "haClient") haLastTime = -1000000;
            if (zones[1].workMode == "wallClock") {
              timeClient.setTimeOffset(ntpTimeZone * 3600);
              timeClient.update();
              zones[1].previousMillis = -1000000;
            }
          }
          if (p->name() == "scrollSpeedZone1") { 
            zones[1].scrollSpeed = p->value().toInt();
            P.setSpeed(1, zones[1].scrollSpeed);
          }
          if (p->name() == "scrollPauseZone1") {
            zones[1].scrollPause = p->value().toInt();
            P.setPause(1, zones[1].scrollPause);
          }
          if (p->name() == "scrollAlignZone1") {
            zones[1].scrollAlign = p->value().c_str();
            P.setTextAlignment(1, stringToTextPositionT(zones[1].scrollAlign));
          }
          if (p->name() == "scrollEffectZone1In") {
            zones[1].scrollEffectIn = p->value().c_str();
            P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), stringToTextEffectT(zones[1].scrollEffectOut));
          }
          if (p->name() == "scrollEffectZone1Out") {
            zones[1].scrollEffectOut = p->value().c_str();
            P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), stringToTextEffectT(zones[1].scrollEffectOut));
          }
          if (p->name() == "mqttTextTopicZone1") {
            if (MQTTZones[1].message != p->value().c_str()) {
              MQTTZones[1].message = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "mqttPostfixZone1") {
            if (zones[1].mqttPostfix != p->value().c_str()) {
              zones[1].mqttPostfix = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "clockDisplayFormatZone1") {
            zones[1].clockDisplayFormat = p->value().c_str();
            curTimeZone1 = getCurTime(zones[1].font, zones[1].clockDisplayFormat);
          }
          if (p->name() == "owmWhatToDisplayZone1") {
            zones[1].owmWhatToDisplay = p->value().c_str();
            if(zones[1].owmWhatToDisplay == "owmWeatherIcon") {
              P.setFont(1, wledSymbolFont);
              P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), PA_NO_EFFECT);
            }
          }
          if (p->name() == "fontZone1") {
            zones[1].font = p->value().c_str();
            applyZoneFont(1, zones[1].font);
          }
          if (p->name() == "haSensorIdZone1") zones[1].haSensorId = p->value().c_str();
          if (p->name() == "haSensorPostfixZone1") zones[1].haSensorPostfix = p->value().c_str();
          if (p->name() == "charspacingZone1") {
            zones[1].charspacing = p->value().toInt();
            P.setCharSpacing(1, zones[1].charspacing);
          }
          if (p->name() == "ds18b20PostfixZone1") {
            zones[1].ds18b20Postfix = p->value().c_str();
            previousDsMillis = -1000;
          }
          // zone 2
          if (p->name() == "workModeZone2") {
            zones[2].workMode = p->value().c_str();
            if (zones[2].workMode == "owmWeather") owmLastTime = -1000000;
            if (zones[2].workMode == "mqttClient") mqttClient.disconnect();
            if (zones[2].workMode == "haClient") haLastTime = -1000000;
            if (zones[2].workMode == "wallClock") {
              timeClient.setTimeOffset(ntpTimeZone * 3600);
              timeClient.update();
              zones[2].previousMillis = -1000000;
            }
          }
          if (p->name() == "scrollSpeedZone2") { 
            zones[2].scrollSpeed = p->value().toInt();
            P.setSpeed(2, zones[2].scrollSpeed);
          }
          if (p->name() == "scrollPauseZone2") {
            zones[2].scrollPause = p->value().toInt();
            P.setPause(2, zones[2].scrollPause);
          }
          if (p->name() == "scrollAlignZone2") {
            zones[2].scrollAlign = p->value().c_str();
            P.setTextAlignment(2, stringToTextPositionT(zones[2].scrollAlign));
          }
          if (p->name() == "scrollEffectZone2In") {
            zones[2].scrollEffectIn = p->value().c_str();
            P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), stringToTextEffectT(zones[2].scrollEffectOut));
          }
          if (p->name() == "scrollEffectZone2Out") {
            zones[2].scrollEffectOut = p->value().c_str();
            P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), stringToTextEffectT(zones[2].scrollEffectOut));
          }
          if (p->name() == "mqttTextTopicZone2") {
            if (MQTTZones[2].message != p->value().c_str()) {
              MQTTZones[2].message = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "mqttPostfixZone2") {
            if (zones[2].mqttPostfix != p->value().c_str()) {
              zones[2].mqttPostfix = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "clockDisplayFormatZone2") {
            zones[2].clockDisplayFormat = p->value().c_str();
            curTimeZone2 = getCurTime(zones[2].font, zones[2].clockDisplayFormat);
          }
          if (p->name() == "owmWhatToDisplayZone2") {
            zones[2].owmWhatToDisplay = p->value().c_str();
            if(zones[2].owmWhatToDisplay == "owmWeatherIcon") {
              P.setFont(2, wledSymbolFont);
              P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), PA_NO_EFFECT);
            }
          }
          if (p->name() == "fontZone2") {
            zones[2].font = p->value().c_str();
            applyZoneFont(2, zones[2].font);
          }
          if (p->name() == "haSensorIdZone2") zones[2].haSensorId = p->value().c_str();
          if (p->name() == "haSensorPostfixZone2") zones[2].haSensorPostfix = p->value().c_str();
          if (p->name() == "charspacingZone2") {
            zones[2].charspacing = p->value().toInt();
            P.setCharSpacing(2, zones[2].charspacing);
          }
          if (p->name() == "ds18b20PostfixZone2") {
            zones[2].ds18b20Postfix = p->value().c_str();
            previousDsMillis = -1000;
          }
          // zone 3
          if (p->name() == "workModeZone3") {
            zones[3].workMode = p->value().c_str();
            if (zones[3].workMode == "owmWeather") owmLastTime = -1000000;
            if (zones[3].workMode == "mqttClient") mqttClient.disconnect();
            if (zones[3].workMode == "haClient") haLastTime = -1000000;
            if (zones[3].workMode == "wallClock") {
              timeClient.setTimeOffset(ntpTimeZone * 3600);
              timeClient.update();
              zones[3].previousMillis = -1000000;
            }
          }
          if (p->name() == "scrollSpeedZone3") { 
            zones[3].scrollSpeed = p->value().toInt();
            P.setSpeed(3, zones[3].scrollSpeed);
          }
          if (p->name() == "scrollPauseZone3") {
            zones[3].scrollPause = p->value().toInt();
            P.setPause(3, zones[3].scrollPause);
          }
          if (p->name() == "scrollAlignZone3") {
            zones[3].scrollAlign = p->value().c_str();
            P.setTextAlignment(3, stringToTextPositionT(zones[3].scrollAlign));
          }
          if (p->name() == "scrollEffectZone3In") {
            zones[3].scrollEffectIn = p->value().c_str();
            P.setTextEffect(3, stringToTextEffectT(zones[3].scrollEffectIn), stringToTextEffectT(zones[3].scrollEffectOut));
          }
          if (p->name() == "scrollEffectZone3Out") {
            zones[3].scrollEffectOut = p->value().c_str();
            P.setTextEffect(3, stringToTextEffectT(zones[3].scrollEffectIn), stringToTextEffectT(zones[3].scrollEffectOut));
          }
          if (p->name() == "mqttTextTopicZone3") {
            if (MQTTZones[3].message != p->value().c_str()) {
              MQTTZones[3].message = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "mqttPostfixZone3") {
            if (zones[3].mqttPostfix != p->value().c_str()) {
              zones[3].mqttPostfix = p->value().c_str();
              mqttClient.disconnect();
            }
          }
          if (p->name() == "clockDisplayFormatZone3") {
            zones[3].clockDisplayFormat = p->value().c_str();
            curTimeZone3 = getCurTime(zones[3].font, zones[3].clockDisplayFormat);
          }
          if (p->name() == "owmWhatToDisplayZone3") {
            zones[3].owmWhatToDisplay = p->value().c_str();
            if(zones[3].owmWhatToDisplay == "owmWeatherIcon") {
              P.setFont(3, wledSymbolFont);
              P.setTextEffect(3, stringToTextEffectT(zones[3].scrollEffectIn), PA_NO_EFFECT);
            }
          }
          if (p->name() == "fontZone3") {
            zones[3].font = p->value().c_str();
            applyZoneFont(3, zones[3].font);
          }
          if (p->name() == "haSensorIdZone3") zones[3].haSensorId = p->value().c_str();
          if (p->name() == "haSensorPostfixZone3") zones[3].haSensorPostfix = p->value().c_str();
          if (p->name() == "charspacingZone3") {
            zones[3].charspacing = p->value().toInt();
            P.setCharSpacing(3, zones[3].charspacing);
          }
          if (p->name() == "ds18b20PostfixZone3") {
            zones[3].ds18b20Postfix = p->value().c_str();
            previousDsMillis = -1000;
          }

          if (p->name() == "mqttServerAddress") mqttServerAddress = p->value().c_str();
          if (p->name() == "mqttServerPort") mqttServerPort = p->value().toInt();
          if (p->name() == "mqttUsername") mqttUsername = p->value().c_str();
          if (p->name() == "mqttPassword") mqttPassword = p->value().c_str();
          if (p->name() == "ntpTimeZone") {
            ntpTimeZone = p->value().toInt();
            timeClient.setTimeOffset(ntpTimeZone * 3600);
            timeClient.update();
          }
          if (p->name() == "owmApiToken") owmApiToken = p->value().c_str();
          if (p->name() == "owmUnitsFormat") owmUnitsFormat = p->value().c_str();
          if (p->name() == "owmUpdateInterval") owmUpdateInterval = p->value().toInt();
          if (p->name() == "owmCity") owmCity = p->value().c_str();
          if (p->name() == "haAddr") haAddr = p->value().c_str();
          if (p->name() == "haUpdateInterval") haUpdateInterval = p->value().toInt();
          if (p->name() == "haApiToken") haApiToken = p->value().c_str();
          if (p->name() == "haApiHttpType") haApiHttpType = p->value().c_str();
          if (p->name() == "haApiPort") haApiPort = p->value().toInt();
          if (p->name() == "deviceName") deviceName = p->value().c_str();
          if (p->name() == "disableServiceMessages") disableServiceMessages = p->value().c_str();
          if (p->name() == "disableDotsBlink") disableDotsBlink = p->value().c_str();
          if (p->name() == "ds18b20Enable") ds18b20Enable = p->value().c_str();
          if (p->name() == "ds18b20UpdateInterval") ds18b20UpdateInterval = p->value().toInt();
          if (p->name() == "ds18b20UnitsFormat") {
            ds18b20UnitsFormat = p->value().c_str();
            previousDsMillis = -1000;
          }
          if (p->name() == "intensity") {                 intensity = (p->value()).toInt()-1;
                                                          P.setIntensity(intensity);
          }
          if (strcmp(p->name().c_str(), "messageZone0") == 0) zoneNewMessage(0, p->value().c_str(), "");
          if (strcmp(p->name().c_str(), "messageZone1") == 0) zoneNewMessage(1, p->value().c_str(), "");
          if (strcmp(p->name().c_str(), "messageZone2") == 0) zoneNewMessage(2, p->value().c_str(), "");
          if (strcmp(p->name().c_str(), "messageZone3") == 0) zoneNewMessage(3, p->value().c_str(), "");
        }
      }
      request->send(200, "application/json", "{\"status\":\"success\"}");
      writeAllVarsToConfFile();
    });

  ConfigFile_Read_Variable();

  // Initializing display
  P.begin(zoneNumbers);
  for ( uint8_t n = 0; n < zoneNumbers; n++) {
    P.setZone(n, zones[n].begin, zones[n].end);
  }
  P.setIntensity(intensity);

  // Start MQTT client
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
  mqttClient.setCallback(MQTTCallback);
  mqttClient.setBufferSize(2048);
  
  // Start webserver
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();

  // start NTP client
  timeClient.begin();
  timeClient.setTimeOffset(ntpTimeZone * 3600);
  timeClient.update();

  // Start Dallas ds18b20
  sensors.begin();
  oneWire.reset();
  oneWire.search(addrS1); //getting the address of Sesnor-1
}

void loop() {
  currentMillis = millis();

  if (!ipInfo && P.getZoneStatus(0) && !allTestsFinish) {
    allTestsFinish      = true;
    newConfigAvailable  = true;
  }
  
  if (!zone3Test && ipInfo) {
    if (P.getZoneStatus(3)) {
      P.displayClear();
      P.setCharSpacing(0,1);
      P.setFont(0, wledFont_cyrillic);
      P.setSpeed(0, 50);
      P.setTextEffect(0, PA_SCROLL_LEFT, PA_NO_EFFECT);
      zoneNewMessage(0, "ip: " + WiFi.localIP().toString(), "");
      ipInfo = false;
    }
  }

  if (!zone2Test && zone3Test) {
    if (P.getZoneStatus(2)) {
      P.displayClear();
      P.setFont(3, wledSymbolFont);
      P.setTextEffect(3, PA_GROW_UP, PA_NO_EFFECT);
      P.setCharSpacing(3,0);
      P.setPause(3, 500);
      P.setSpeed(3, 25);
      String testMessage = "8";
      for ( int i = 0; i < int(zones[3].end - zones[3].begin); i++) testMessage += 8;
      zoneNewMessage(3, testMessage, "");
      zone3Test = false;
      
    }      
  }

  if (!zone1Test && zone2Test) {
    if (P.getZoneStatus(1)) {
      P.displayClear();
      P.setFont(2, wledSymbolFont);
      P.setTextEffect(2, PA_GROW_UP, PA_NO_EFFECT);
      P.setCharSpacing(2,0);
      P.setPause(2, 500);
      P.setSpeed(2, 25);
      String testMessage = "8";
      for ( int i = 0; i < int(zones[2].end - zones[2].begin); i++) testMessage += 8;
      zoneNewMessage(2, testMessage, "");
      zone2Test = false;
      
    }      
  }

  if (!zone0Test && zone1Test) {
    if (P.getZoneStatus(0)) {
      P.displayClear();
      P.setFont(1, wledSymbolFont);
      P.setTextEffect(1, PA_GROW_UP, PA_NO_EFFECT);
      P.setCharSpacing(1,0);
      P.setPause(1, 500);
      P.setSpeed(1, 25);
      String testMessage = "8";
      for ( int i = 0; i < int(zones[1].end - zones[1].begin); i++) testMessage += 8;
      zoneNewMessage(1, testMessage, "");
      zone1Test = false;
    }      
  }

  if (zone0Test) {
    P.setFont(0, wledSymbolFont);
    P.setTextEffect(0, PA_GROW_UP, PA_NO_EFFECT);
    P.setCharSpacing(0,0);
    P.setPause(0, 500);
    P.setSpeed(0, 25);
    String testMessage = "8";
    for ( int i = 0; i < int(zones[0].end - zones[0].begin); i++) testMessage += 8;
    zoneNewMessage(0, testMessage, "");
    zone0Test = false;
  }

  if (restartESP) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

  if (newConfigAvailable) {
    newConfigAvailable = false;
    ConfigFile_Read_Variable();
    
    mqttClient.disconnect();
    P.setIntensity(intensity);

    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      P.setSpeed(n, zones[n].scrollSpeed);
      P.setPause(n, zones[n].scrollPause);
      P.setTextAlignment(n, stringToTextPositionT(zones[n].scrollAlign));
      P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), stringToTextEffectT(zones[n].scrollEffectOut));
      P.setCharSpacing(n, zones[n].charspacing);
      applyZoneFont(n, zones[n].font);

      if (zones[n].workMode == "wallClock") {
        timeClient.setTimeOffset(ntpTimeZone * 3600);
        timeClient.update();
        zones[n].curTimeZone = getCurTime(zones[n].font, zones[n].clockDisplayFormat);
        zones[n].previousMillis = -1000000;
      }

      if (zones[n].workMode == "mqttClient" && disableServiceMessages == "false") zoneNewMessage(n, "MQTT", "");
      if (zones[n].workMode == "manualInput" && disableServiceMessages == "false") zoneNewMessage(n, "Manual", "");
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
    if (mqttServerAddress.length() > 0){
      if (!mqttClient.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
          lastReconnectAttempt = now;
          if (reconnect()) {  // Attempt to reconnect
            lastReconnectAttempt = 0;
          }
        }
      } else {
        mqttClient.loop();  // Client connected
      }
    }

    if (ds18b20Enable == "true") {
          if (currentMillis - previousDsMillis >= (unsigned)ds18b20UpdateInterval * 1000) {
            previousDsMillis = currentMillis;
            sensors.requestTemperatures();
            float dsTempC = sensors.getTempC(addrS1);
            dtostrf(dsTempC, -5, 1, dsTemp);
            
            mqttClient.publish(String("wledPixel-" + shortMACaddr + "/temperature").c_str(), dsTemp);
            
            if (ds18b20UnitsFormat == "Fahrenheit") {
              float dsTempF = (dsTempC*1.8)+32;
              dtostrf(dsTempF, -5, 1, dsTemp);
            }
            
            dsTempToDisplay = true;
          }
        }

    for ( uint8_t n = 0; n < zoneNumbers; n++) {
      // Wall Clock
      if (zones[n].workMode == "wallClock") {
        String curTimeZoneNew = getCurTime(zones[n].font, zones[n].clockDisplayFormat);
        if (zones[n].clockDisplayFormat == "HHMM" || zones[n].clockDisplayFormat == "HHMMSS" || zones[n].clockDisplayFormat == "ddmmaahhmm") {
          if (disableDotsBlink == "false") {
            if (currentMillis - zones[n].previousMillis >= 1000) {
                zones[n].previousMillis = currentMillis;
                    if (zones[n].curTimeZone == curTimeZoneNew || zones[n].curTimeZone == flashClockDots(curTimeZoneNew)) {
                        if (zones[n].curTimeZone.indexOf(":") > 0) curTimeZoneNew.replace(":", "¦");
                        P.setPause(n,100);
                        P.setTextEffect(n, PA_NO_EFFECT, PA_NO_EFFECT);
                        zones[n].curTimeZone = curTimeZoneNew;
                        zoneNewMessage(n, zones[n].curTimeZone, "");
                    } else {
                        if ( zones[n].curTimeZone.indexOf(":") > 0 ) curTimeZoneNew.replace(":", "¦");
                        zones[n].curTimeZone = curTimeZoneNew;
                        P.setPause(n,100);
                        P.setTextEffect(n, stringToTextEffectT(zones[n].scrollEffectIn), PA_NO_EFFECT);
                        zoneNewMessage(n, zones[n].curTimeZone, "");
                    }
            }
          } else {
            if (currentMillis - zones[n].previousMillis >= 60000) {
                zones[n].previousMillis = currentMillis;
                zones[n].curTimeZone = curTimeZoneNew;
                zoneNewMessage(n, zones[n].curTimeZone, "");   
            }
          }
        } else {
          if (currentMillis - zones[n].previousMillis >= 60000) {
            zones[n].previousMillis = currentMillis;
            zones[n].curTimeZone = curTimeZoneNew;
            zoneNewMessage(n, zones[n].curTimeZone, "");
          }
        }
      }

      // OWM client
      if (zones[n].workMode == "owmWeather") {
        if (currentMillis - owmLastTime >= (unsigned)owmUpdateInterval * 1000) {
          zoneNewMessage(n, openWetherMapGetWeather(zones[n].owmWhatToDisplay), "");
          owmLastTime = currentMillis;
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
      }
    }

  }
  
  displayAnimation();
}