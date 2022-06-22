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

#include <AsyncElegantOTA.h>

#include "htmlFiles.h"
#include <wledFont.h>

WiFiClient espClient;

//// MQTT settings ////
String mqttServerAddress = "";
int mqttServerPort = 1883;
String mqttUsername = "";
String mqttPassword = "";
PubSubClient mqttClient(espClient);
long lastReconnectAttempt = 0;

String MQTTGlobalPrefix = WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15);    // This sets  the topic prefix to the last five chars of the MAC, ie: C0A4

typedef struct {
  String    message, scrollEffect, effectWithoutExit, scrollSpeed, scrollPause, scrollAllign, charSpasing;
} MQTTZoneData;

MQTTZoneData MQTTZones[] = {
  {MQTTGlobalPrefix + "/zone0/text", MQTTGlobalPrefix + "/zone0/scrolleffect", MQTTGlobalPrefix + "/zone0/scrolleffect_without_exit", MQTTGlobalPrefix + "/zone0/scrollspeed", MQTTGlobalPrefix + "/zone0/scrollpause", MQTTGlobalPrefix + "/zone0/scrollalign", MQTTGlobalPrefix + "/zone0/charspasing"},
  {MQTTGlobalPrefix + "/zone1/text", MQTTGlobalPrefix + "/zone1/scrolleffect", MQTTGlobalPrefix + "/zone1/scrolleffect_without_exit", MQTTGlobalPrefix + "/zone1/scrollspeed", MQTTGlobalPrefix + "/zone1/scrollpause", MQTTGlobalPrefix + "/zone1/scrollalign", MQTTGlobalPrefix + "/zone1/charspasing"},
  {MQTTGlobalPrefix + "/zone2/text", MQTTGlobalPrefix + "/zone2/scrolleffect", MQTTGlobalPrefix + "/zone2/scrolleffect_without_exit", MQTTGlobalPrefix + "/zone2/scrollspeed", MQTTGlobalPrefix + "/zone2/scrollpause", MQTTGlobalPrefix + "/zone2/scrollalign", MQTTGlobalPrefix + "/zone2/charspasing"},
};
String MQTTIntensity = MQTTGlobalPrefix + "/intensity";
String MQTTPower = MQTTGlobalPrefix + "/power";
//// MQTT settings ////


// Display config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // type of device hardware https://majicdesigns.github.io/MD_MAX72XX/page_hardware.html
//// Display pinout
#define DATA_PIN  D7                      // WeMos D1 mini GPIO13
#define CS_PIN    D6                      // WeMos D1 mini GPIO12
#define CLK_PIN   D5                      // WeMos D1 mini GPIO14
uint8_t MAX_DEVICES = 12;                 // number of device segments
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

typedef struct {
    uint8_t   begin, end;
    uint8_t   scrollSpeed, charSpasing;
    uint16_t  scrollPause;
    String    scrollAlign, scrollEffectIn, scrollEffectOut, font, workMode, clockDisplayFormat, haSensorId, haSensorPostfix, owmWhatToDisplay;
} ZoneData;

// structure:
ZoneData zones[] = {
  {0, 3, 35, 1, 3000, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature"},
  {4, 5, 35, 1, 3000, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature"},
  {6, 7, 35, 1, 3000, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "wledFont", "manualInput", "HHMM", "", "", "owmTemperature"},
};

uint8_t zoneNumbers = 1;
uint8_t intensity = 7;

char zone0Message[50] = "zone0";
char zone1Message[50] = "zone1";
char zone2Message[50] = "zone2";
bool zone0newMessageAvailable = false;
bool zone1newMessageAvailable = false;
bool zone2newMessageAvailable = false;

char const *wifiAPname = "wled-AP";
char const *wifiAPpassword = "12345678";

unsigned long previousMillis = 0; 
String curTimeZone0 = "00:00";
String curTimeZone1 = "00:00";
String curTimeZone2 = "00:00";
int i = 0;

// Initialize NTP
String ntpServerAddress = "pool.ntp.org";
int ntpTimeZone = 3;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerAddress.c_str());

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server,&dns);

bool restartESP = false;
bool firstLoop = true;
bool secondLoop = false;
bool newConfigAvailable = false;

//flag for saving data
bool shouldSaveConfig = false;
//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

// variables for get weather func
static uint32_t owmLastTime = 0;
int owmUpdateInterval = 60;
String    owmApiToken;
String    owmUnitsFormat;
String    owmCity;

// Home Assistant client variables
String haAddr, haApiHttpType, haApiToken;
int haUpdateInterval = 60;
int haApiPort = 8123;
static uint32_t haLastTime = 0;


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
  if(var == "zoneNumbers")                  return itoa(zoneNumbers, buffer, 10);
  if(var == "zone0Begin")                   return itoa(zones[0].begin, buffer, 10);
  if(var == "zone0End")                     return itoa(zones[0].end, buffer, 10);
  if(var == "zone1Begin")                   return itoa(zones[1].begin, buffer, 10);
  if(var == "zone1End")                     return itoa(zones[1].end, buffer, 10);
  if(var == "zone2Begin")                   return itoa(zones[2].begin, buffer, 10);
  if(var == "zone2End")                     return itoa(zones[2].end, buffer, 10);
  if(var == "wifiSsid")                     return WiFi.SSID();
  if(var == "wifiIp")                       return WiFi.localIP().toString();
  if(var == "wifiGateway")                  return WiFi.gatewayIP().toString();
  if(var == "intensity")                    return itoa(intensity, buffer, 10);
  if(var == "scrollSpeedZone0")             return itoa(zones[0].scrollSpeed, buffer, 10);
  if(var == "scrollSpeedZone1")             return itoa(zones[1].scrollSpeed, buffer, 10);
  if(var == "scrollSpeedZone2")             return itoa(zones[2].scrollSpeed, buffer, 10);
  if(var == "scrollPauseZone0")             return itoa(zones[0].scrollPause, buffer, 10);
  if(var == "scrollPauseZone1")             return itoa(zones[1].scrollPause, buffer, 10);
  if(var == "scrollPauseZone2")             return itoa(zones[2].scrollPause, buffer, 10);
  if(var == "scrollAlignZone0")             return zones[0].scrollAlign;
  if(var == "scrollAlignZone1")             return zones[1].scrollAlign;
  if(var == "scrollAlignZone2")             return zones[2].scrollAlign;
  if(var == "scrollEffectZone0In")          return zones[0].scrollEffectIn;
  if(var == "scrollEffectZone1In")          return zones[1].scrollEffectIn;
  if(var == "scrollEffectZone2In")          return zones[2].scrollEffectIn;
  if(var == "scrollEffectZone0Out")         return zones[0].scrollEffectOut;
  if(var == "scrollEffectZone1Out")         return zones[1].scrollEffectOut;
  if(var == "scrollEffectZone2Out")         return zones[2].scrollEffectOut;
  if(var == "mqttServerAddress")            return mqttServerAddress;
  if(var == "mqttServerPort")               return itoa(mqttServerPort, buffer, 10);
  if(var == "mqttUsername")                 return mqttUsername;
  if(var == "mqttPassword")                 return mqttPassword;
  if(var == "mqttDevicePrefix")             return MQTTGlobalPrefix;
  if(var == "ntpTimeZone")                  return itoa(ntpTimeZone, buffer, 10);
  if(var == "clockDisplayFormatZone0")      return zones[0].clockDisplayFormat;
  if(var == "clockDisplayFormatZone1")      return zones[1].clockDisplayFormat;
  if(var == "clockDisplayFormatZone2")      return zones[2].clockDisplayFormat;
  if(var == "owmApiToken")                  return owmApiToken;
  if(var == "owmUnitsFormat")               return owmUnitsFormat;
  if(var == "owmUpdateInterval")            return itoa(owmUpdateInterval, buffer, 10);
  if(var == "owmWhatToDisplayZone0")        return zones[0].owmWhatToDisplay;
  if(var == "owmWhatToDisplayZone1")        return zones[1].owmWhatToDisplay;
  if(var == "owmWhatToDisplayZone2")        return zones[2].owmWhatToDisplay;
  if(var == "owmCity")                      return owmCity;
  if(var == "fontZone0")                    return zones[0].font;
  if(var == "fontZone1")                    return zones[1].font;
  if(var == "fontZone2")                    return zones[2].font;
  if(var == "haAddr")                       return haAddr;
  if(var == "haApiToken")                   return haApiToken;
  if(var == "haApiHttpType")                return haApiHttpType;
  if(var == "haApiPort")                    return itoa(haApiPort, buffer, 10);
  if(var == "haSensorIdZone0")              return zones[0].haSensorId;
  if(var == "haSensorIdZone1")              return zones[1].haSensorId;
  if(var == "haSensorIdZone2")              return zones[2].haSensorId;
  if(var == "haSensorPostfixZone0")         return zones[0].haSensorPostfix;
  if(var == "haSensorPostfixZone1")         return zones[1].haSensorPostfix;
  if(var == "haSensorPostfixZone2")         return zones[2].haSensorPostfix;
  if(var == "haUpdateInterval")             return itoa(haUpdateInterval, buffer, 10);
  if(var == "charSpasingZone0")             return itoa(zones[0].charSpasing, buffer, 10);
  if(var == "charSpasingZone1")             return itoa(zones[1].charSpasing, buffer, 10);
  if(var == "charSpasingZone2")             return itoa(zones[2].charSpasing, buffer, 10);
  return String();
}


void writeVarToConfFile(String VarName, String VarValue, bool softReloadVars, bool hardReloadVars) {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("!!! ERROR !!! failed to open config file for writing");
    return;
  }

  DynamicJsonDocument doc(4096);
  deserializeJson(doc, configFile);
  configFile.close();
  doc[VarName] = VarValue;
  configFile = LittleFS.open("/config.json", "w");
  serializeJson(doc, configFile);
  configFile.close();

  if(softReloadVars) newConfigAvailable = true;
  if(hardReloadVars) restartESP = true;
}

void ConfigFile_Read_Variable() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("!!! ERROR !!! failed to open config file for writing");
    return;
  }

  DynamicJsonDocument doc(4096);
  deserializeJson(doc, configFile);
  // print config file to serial
  serializeJsonPretty(doc, Serial);
  configFile.close();

  JsonObject postObj = doc.as<JsonObject>();
  if(postObj[F("workModeZone0")])             zones[0].workMode = postObj[F("workModeZone0")].as<String>();
  if(postObj[F("workModeZone1")])             zones[1].workMode = postObj[F("workModeZone1")].as<String>();
  if(postObj[F("workModeZone2")])             zones[2].workMode = postObj[F("workModeZone2")].as<String>();
  if(postObj[F("zoneNumbers")])               zoneNumbers = postObj[F("zoneNumbers")].as<uint8_t>();
  if(postObj[F("zone0Begin")])                zones[0].begin = postObj[F("zone0Begin")].as<uint8_t>();
  if(postObj[F("zone0End")])                  zones[0].end = postObj[F("zone0End")].as<uint8_t>();
  if(postObj[F("zone1Begin")])                zones[1].begin = postObj[F("zone1Begin")].as<uint8_t>();
  if(postObj[F("zone1End")])                  zones[1].end = postObj[F("zone1End")].as<uint8_t>();
  if(postObj[F("zone2Begin")])                zones[2].begin = postObj[F("zone2Begin")].as<uint8_t>();
  if(postObj[F("zone2End")])                  zones[2].end = postObj[F("zone2End")].as<uint8_t>();
  if(postObj[F("intensity")])                 intensity = postObj[F("intensity")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone0")])          zones[0].scrollSpeed = postObj[F("scrollSpeedZone0")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone1")])          zones[1].scrollSpeed = postObj[F("scrollSpeedZone1")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone2")])          zones[2].scrollSpeed = postObj[F("scrollSpeedZone2")].as<uint8_t>();
  if(postObj[F("scrollPauseZone0")])          zones[0].scrollPause = postObj[F("scrollPauseZone0")].as<uint16_t>();
  if(postObj[F("scrollPauseZone1")])          zones[1].scrollPause = postObj[F("scrollPauseZone1")].as<uint16_t>();
  if(postObj[F("scrollPauseZone2")])          zones[2].scrollPause = postObj[F("scrollPauseZone2")].as<uint16_t>();
  if(postObj[F("scrollAlignZone0")])          zones[0].scrollAlign = postObj[F("scrollAlignZone0")].as<String>();
  if(postObj[F("scrollAlignZone1")])          zones[1].scrollAlign = postObj[F("scrollAlignZone1")].as<String>();
  if(postObj[F("scrollAlignZone2")])          zones[2].scrollAlign = postObj[F("scrollAlignZone2")].as<String>();
  if(postObj[F("scrollEffectZone0In")])       zones[0].scrollEffectIn = postObj[F("scrollEffectZone0In")].as<String>();
  if(postObj[F("scrollEffectZone1In")])       zones[1].scrollEffectIn = postObj[F("scrollEffectZone1In")].as<String>();
  if(postObj[F("scrollEffectZone2In")])       zones[2].scrollEffectIn = postObj[F("scrollEffectZone2In")].as<String>();
  if(postObj[F("scrollEffectZone0Out")])      zones[0].scrollEffectOut = postObj[F("scrollEffectZone0Out")].as<String>();
  if(postObj[F("scrollEffectZone1Out")])      zones[1].scrollEffectOut = postObj[F("scrollEffectZone1Out")].as<String>();
  if(postObj[F("scrollEffectZone2Out")])      zones[2].scrollEffectOut = postObj[F("scrollEffectZone2Out")].as<String>();
  if(postObj[F("mqttServerAddress")])         mqttServerAddress = postObj[F("mqttServerAddress")].as<String>();
  if(postObj[F("mqttServerPort")])            mqttServerPort = postObj[F("mqttServerPort")].as<int>();
  if(postObj[F("mqttUsername")])              mqttUsername = postObj[F("mqttUsername")].as<String>();
  if(postObj[F("mqttPassword")])              mqttPassword = postObj[F("mqttPassword")].as<String>();
  if(postObj[F("ntpTimeZone")])               ntpTimeZone = postObj[F("ntpTimeZone")].as<int>();
  if(postObj[F("clockDisplayFormatZone0")])   zones[0].clockDisplayFormat = postObj[F("clockDisplayFormatZone0")].as<String>();
  if(postObj[F("clockDisplayFormatZone1")])   zones[1].clockDisplayFormat = postObj[F("clockDisplayFormatZone1")].as<String>();
  if(postObj[F("clockDisplayFormatZone2")])   zones[2].clockDisplayFormat = postObj[F("clockDisplayFormatZone2")].as<String>();
  if(postObj[F("owmApiToken")])               owmApiToken = postObj[F("owmApiToken")].as<String>();
  if(postObj[F("owmUnitsFormat")])            owmUnitsFormat = postObj[F("owmUnitsFormat")].as<String>();
  if(postObj[F("owmUpdateInterval")])         owmUpdateInterval = postObj[F("owmUpdateInterval")].as<int>();
  if(postObj[F("owmCity")])                   owmCity = postObj[F("owmCity")].as<String>();
  if(postObj[F("owmWhatToDisplayZone0")])     zones[0].owmWhatToDisplay = postObj[F("owmWhatToDisplayZone0")].as<String>();
  if(postObj[F("owmWhatToDisplayZone1")])     zones[1].owmWhatToDisplay = postObj[F("owmWhatToDisplayZone1")].as<String>();
  if(postObj[F("owmWhatToDisplayZone2")])     zones[2].owmWhatToDisplay = postObj[F("owmWhatToDisplayZone2")].as<String>();
  if(postObj[F("fontZone0")])                 zones[0].font = postObj[F("fontZone0")].as<String>();
  if(postObj[F("fontZone1")])                 zones[1].font = postObj[F("fontZone1")].as<String>();
  if(postObj[F("fontZone2")])                 zones[2].font = postObj[F("fontZone2")].as<String>();
  if(postObj[F("haAddr")])                    haAddr = postObj[F("haAddr")].as<String>();
  if(postObj[F("haApiToken")])                haApiToken = postObj[F("haApiToken")].as<String>();
  if(postObj[F("haApiHttpType")])             haApiHttpType = postObj[F("haApiHttpType")].as<String>();
  if(postObj[F("haApiPort")])                 haApiPort = postObj[F("haApiPort")].as<int>();
  if(postObj[F("haSensorIdZone0")])           zones[0].haSensorId = postObj[F("haSensorIdZone0")].as<String>();
  if(postObj[F("haSensorIdZone1")])           zones[1].haSensorId = postObj[F("haSensorIdZone1")].as<String>();
  if(postObj[F("haSensorIdZone2")])           zones[2].haSensorId = postObj[F("haSensorIdZone2")].as<String>();
  if(postObj[F("haSensorPostfixZone0")])      zones[0].haSensorPostfix = postObj[F("haSensorPostfixZone0")].as<String>();
  if(postObj[F("haSensorPostfixZone1")])      zones[1].haSensorPostfix = postObj[F("haSensorPostfixZone1")].as<String>();
  if(postObj[F("haSensorPostfixZone2")])      zones[2].haSensorPostfix = postObj[F("haSensorPostfixZone2")].as<String>();
  if(postObj[F("haUpdateInterval")])          haUpdateInterval = postObj[F("haUpdateInterval")].as<int>();
  if(postObj[F("charSpasingZone0")])          zones[0].charSpasing = postObj[F("charSpasingZone0")].as<int>();
  if(postObj[F("charSpasingZone1")])          zones[1].charSpasing = postObj[F("charSpasingZone1")].as<int>();
  if(postObj[F("charSpasingZone2")])          zones[2].charSpasing = postObj[F("charSpasingZone2")].as<int>();
}

void zone0NewMessage(String newMessage) {
  strcpy(zone0Message, newMessage.c_str());
  zone0newMessageAvailable = true;
}
void zone1NewMessage(String newMessage) {
  strcpy(zone1Message, newMessage.c_str());
  zone1newMessageAvailable = true;
}
void zone2NewMessage(String newMessage) {
  strcpy(zone2Message, newMessage.c_str());
  zone2newMessageAvailable = true;
}

void MQTTCallback(char* topic, byte* payload, int length) {
  String PayloadString = "";
  for (int i = 0; i < length; i++) { PayloadString = PayloadString + (char)payload[i]; }

  Serial.printf("\nNew message arrived");  
  Serial.printf("\nTopic: %s\n", topic);
  Serial.println(PayloadString);

  if (zones[0].workMode == "mqttClient" || zones[1].workMode == "mqttClient" || zones[2].workMode == "mqttClient") {
    if (strcmp(topic, (char*) MQTTIntensity.c_str()) == 0) {
      writeVarToConfFile("scrollEffectZone0In", PayloadString.c_str(), false, false);
      P.setIntensity(PayloadString.toInt());
    }
    if (strcmp(topic, (char*) MQTTPower.c_str()) == 0) {
      if(PayloadString == "on" || PayloadString == "ON" || PayloadString == "TRUE") {
        P.displayShutdown(0);
      }
      if(PayloadString == "off" || PayloadString == "OFF" || PayloadString == "FALSE") {
        P.displayShutdown(1);
      }
    }
  }

  if(zones[0].workMode == "mqttClient") {
    if (strcmp(topic, (char*) MQTTZones[0].message.c_str()) == 0) {
      zone0NewMessage(PayloadString.c_str());
    }
    if (strcmp(topic, (char*) MQTTZones[0].scrollEffect.c_str()) == 0) {
      writeVarToConfFile("scrollEffectZone0In", PayloadString.c_str(), true, false);
      writeVarToConfFile("scrollEffectZone0Out", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[0].effectWithoutExit.c_str()) == 0) {
      writeVarToConfFile("scrollEffectZone0In", PayloadString.c_str(), true, false);
      writeVarToConfFile("scrollEffectZone0Out", "PA_NO_EFFECT", true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[0].scrollSpeed.c_str()) == 0) {
      writeVarToConfFile("scrollSpeedZone0", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[0].scrollPause.c_str()) == 0) {
      writeVarToConfFile("scrollPauseZone0", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[0].scrollAllign.c_str()) == 0) {
      writeVarToConfFile("scrollAlignZone0", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[0].charSpasing.c_str()) == 0) {
      writeVarToConfFile("charSpasingZone0", PayloadString.c_str(), true, false);
    }
  } 

  if(zones[1].workMode == "mqttClient") {
    if (strcmp(topic, (char*) MQTTZones[1].message.c_str()) == 0) {
      zone1NewMessage(PayloadString.c_str());
    }
    if (strcmp(topic, (char*) MQTTZones[1].scrollEffect.c_str()) == 0) {
      writeVarToConfFile("scrollEffectZone1In", PayloadString.c_str(), true, false);
      writeVarToConfFile("scrollEffectZone1Out", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[1].effectWithoutExit.c_str()) == 0) {
      writeVarToConfFile("scrollEffectZone1In", PayloadString.c_str(), true, false);
      writeVarToConfFile("scrollEffectZone1Out", "PA_NO_EFFECT", true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[1].scrollSpeed.c_str()) == 0) {
      writeVarToConfFile("scrollSpeedZone1", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[1].scrollPause.c_str()) == 0) {
      writeVarToConfFile("scrollPauseZone1", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[1].scrollAllign.c_str()) == 0) {
      writeVarToConfFile("scrollAlignZone1", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[1].charSpasing.c_str()) == 0) {
      writeVarToConfFile("charSpasingZone1", PayloadString.c_str(), true, false);
    }
  } 

  if(zones[2].workMode == "mqttClient") {
    if (strcmp(topic, (char*) MQTTZones[2].message.c_str()) == 0) {
      zone2NewMessage(PayloadString.c_str());
    }
    if (strcmp(topic, (char*) MQTTZones[2].scrollEffect.c_str()) == 0) {
      writeVarToConfFile("scrollEffectZone2In", PayloadString.c_str(), true, false);
      writeVarToConfFile("scrollEffectZone2Out", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[2].effectWithoutExit.c_str()) == 0) {
      writeVarToConfFile("scrollEffectZone2In", PayloadString.c_str(), true, false);
      writeVarToConfFile("scrollEffectZone2Out", "PA_NO_EFFECT", true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[2].scrollSpeed.c_str()) == 0) {
      writeVarToConfFile("scrollSpeedZone2", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[2].scrollPause.c_str()) == 0) {
      writeVarToConfFile("scrollPauseZone2", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[2].scrollAllign.c_str()) == 0) {
      writeVarToConfFile("scrollAlignZone2", PayloadString.c_str(), true, false);
    }
    if (strcmp(topic, (char*) MQTTZones[2].charSpasing.c_str()) == 0) {
      writeVarToConfFile("charSpasingZone2", PayloadString.c_str(), true, false);
    }
  } 
}

boolean reconnect() {
  mqttClient.disconnect();
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);

  // Attempt to connect
  if (mqttClient.connect(clientId.c_str(), mqttUsername.c_str(),mqttPassword.c_str())) {
    Serial.printf("\nMQTT subscribe objects");
    mqttClient.subscribe((char*) MQTTIntensity.c_str());
    mqttClient.subscribe((char*) MQTTPower.c_str());

    for ( uint8_t n = 0; n < 3; n++) {
      mqttClient.subscribe((char*) MQTTZones[n].message.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollEffect.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].effectWithoutExit.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollSpeed.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollPause.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].scrollAllign.c_str());
      mqttClient.subscribe((char*) MQTTZones[n].charSpasing.c_str());
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
  P.write(wifiAPname);
  if (MAX_DEVICES > 5) {
    delay(5000);
    P.write(wifiAPpassword);
  }
}

String getCurTime(String curZoneFont, String displayFormat) {
      String t = (String)timeClient.getFormattedTime(); // returns HH:MM:SS
      
      if (displayFormat == "HHMM") {
        t.remove(5,4);
      }
      if (displayFormat == "HH") {
        t.remove(2,7);
      }
      if (displayFormat == "MM") {
        t.remove(5,4);
        t.remove(0,3);
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
      return t;
}

String flashClockDots(String t) {
  if(t.indexOf(":") > 0) t.replace(":", "¦");
  else t.replace("¦", ":");
  return t;
}

void displayAnimation() {
  if (P.displayAnimate()) {
    if (zone0newMessageAvailable) {
        Serial.printf("\nzone0Message availabel: %s", zone0Message);
        zone0newMessageAvailable = false;
        P.setTextBuffer(0, zone0Message);
        P.displayReset(0);
    }

    if (zone1newMessageAvailable) {
        Serial.printf("\nzone1Message availabel: %s", zone1Message);
        zone1newMessageAvailable = false;
        P.setTextBuffer(1, zone1Message);
        P.displayReset(1);
    }

    if (zone2newMessageAvailable) {
        Serial.printf("\nzone2Message availabel: %s", zone2Message);
        zone2newMessageAvailable = false;
        P.setTextBuffer(2, zone2Message);
        P.displayReset(2);
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
      String owmTemp = String(round(postObj[F("main")][F("temp")].as<int>())) + "°";
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
  wifiManager.autoConnect(wifiAPname, wifiAPpassword);
  Serial.println("");
  Serial.printf("Wifi connected to SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("Local ip: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("Subnet: %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());

  
  Serial.printf("MQTT Device Prefix: ");
  Serial.println(MQTTGlobalPrefix);

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
          if (p->name() == "zoneNumbers")                 writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, true);
          if (p->name() == "zone0Begin")                  writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, true);
          if (p->name() == "zone0End")                    writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, true);
          if (p->name() == "zone1Begin")                  writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, true);
          if (p->name() == "zone1End")                    writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, true);
          if (p->name() == "zone2Begin")                  writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, true);
          if (p->name() == "zone2End")                    writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, true);
          if (p->name() == "workModeZone0")               writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "workModeZone1")               writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "workModeZone2")               writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollSpeedZone0")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollSpeedZone1")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollSpeedZone2")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollPauseZone0")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollPauseZone1")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollPauseZone2")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollAlignZone0")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollAlignZone1")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollAlignZone2")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollEffectZone0In")         writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollEffectZone1In")         writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollEffectZone2In")         writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollEffectZone0Out")        writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollEffectZone1Out")        writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "scrollEffectZone2Out")        writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "mqttServerAddress")           writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "mqttServerPort")              writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "mqttUsername")                writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "mqttPassword")                writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "ntpTimeZone")                 writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "clockDisplayFormatZone0")     writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "clockDisplayFormatZone1")     writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "clockDisplayFormatZone2")     writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmApiToken")                 writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmUnitsFormat")              writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmUpdateInterval")           writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmCity")                     writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmWhatToDisplayZone0")       writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmWhatToDisplayZone1")       writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmWhatToDisplayZone2")       writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "fontZone0")                   writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "fontZone1")                   writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "fontZone2")                   writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haAddr")                      writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haApiToken")                  writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haApiHttpType")               writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haApiPort")                   writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haSensorIdZone0")             writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haSensorIdZone1")             writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haSensorIdZone2")             writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haSensorPostfixZone0")        writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haSensorPostfixZone1")        writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haSensorPostfixZone2")        writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "haUpdateInterval")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "charSpasingZone0")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "charSpasingZone1")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "charSpasingZone2")            writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "intensity") {                 writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, false);
                                                          P.setIntensity((p->value()).toInt());
          }
          if (strcmp(p->name().c_str(), "messageZone0") == 0) zone0NewMessage(p->value().c_str());
          if (strcmp(p->name().c_str(), "messageZone1") == 0) zone1NewMessage(p->value().c_str());
          if (strcmp(p->name().c_str(), "messageZone2") == 0) zone2NewMessage(p->value().c_str());
        }
      }
      request->send(200, "application/json", "{\"status\":\"success\"}");
    });

  ConfigFile_Read_Variable();

  // Initializing display
  P.begin(zoneNumbers);
  // Zone 0 initializing
  P.setZone(0, zones[0].begin, zones[0].end);
  P.setSpeed(0, zones[0].scrollSpeed);
  P.setPause(0, zones[0].scrollPause);
  P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), stringToTextEffectT(zones[0].scrollEffectOut));
  P.setTextAlignment(0, stringToTextPositionT(zones[0].scrollAlign));
  P.setCharSpacing(0, zones[0].charSpasing);
  applyZoneFont(0, zones[0].font);
  // Zone 1 initializing
  if(zoneNumbers > 1) {
    P.setZone(1, zones[1].begin, zones[1].end);
    P.setSpeed(1, zones[1].scrollSpeed);
    P.setPause(1, zones[1].scrollPause);
    P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), stringToTextEffectT(zones[1].scrollEffectOut));
    P.setTextAlignment(1, stringToTextPositionT(zones[1].scrollAlign));
    P.setCharSpacing(1, zones[1].charSpasing);
    applyZoneFont(1, zones[1].font);
  }
  // Zone 2 initializing
  if(zoneNumbers > 2) {
    P.setZone(2, zones[2].begin, zones[2].end);
    P.setSpeed(2, zones[2].scrollSpeed);
    P.setPause(2, zones[2].scrollPause);
    P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), stringToTextEffectT(zones[2].scrollEffectOut));
    P.setTextAlignment(2, stringToTextPositionT(zones[2].scrollAlign));
    P.setCharSpacing(2, zones[2].charSpasing);
    applyZoneFont(2, zones[2].font);
  }
  P.displayClear();
  P.setIntensity(intensity);

  // Start MQTT client
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
  mqttClient.setCallback(MQTTCallback);
  
  // Start webserver
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();

  // start NTP client
  timeClient.begin();
  timeClient.setTimeOffset(ntpTimeZone * 3600);
  timeClient.update();
}


void loop() {
  if (secondLoop && P.getZoneStatus(0)) {
    P.displayClear();
    newConfigAvailable = true;
    secondLoop = false;
  }
  if (firstLoop) {
    P.setSpeed(0, 80);
    P.setTextEffect(0, PA_SCROLL_LEFT, PA_NO_EFFECT);
    zone0NewMessage("ip: " + WiFi.localIP().toString());
    firstLoop = false;
    secondLoop = true;
  }
  if (restartESP) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  if (newConfigAvailable) {
    newConfigAvailable = false;
    ConfigFile_Read_Variable();

    P.setSpeed(0, zones[0].scrollSpeed);
    P.setPause(0, zones[0].scrollPause);
    P.setTextAlignment(0, stringToTextPositionT(zones[0].scrollAlign));
    P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), stringToTextEffectT(zones[0].scrollEffectOut));
    P.setCharSpacing(0, zones[0].charSpasing);
    applyZoneFont(0, zones[0].font);

    if (zones[0].workMode == "wallClock" || zones[1].workMode == "wallClock" || zones[2].workMode == "wallClock") {
      timeClient.setTimeOffset(ntpTimeZone * 3600);
      timeClient.update();
      curTimeZone0 = "0";
      curTimeZone1 = "0";
      curTimeZone2 = "0";
    }
    if (zones[0].workMode == "mqttClient" || zones[1].workMode == "mqttClient" || zones[2].workMode == "mqttClient") mqttClient.disconnect();
    if (zones[0].workMode == "mqttClient")  zone0NewMessage("MQTT");
    if (zones[0].workMode == "manualInput") zone0NewMessage("Manual");
    if (zones[0].workMode == "owmWeather") {
      if(zones[0].owmWhatToDisplay == "owmWeatherIcon") {
        P.setFont(0, wledSymbolFont);
        P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), PA_NO_EFFECT);
      }
    }
    
    if(zoneNumbers > 1) {
      P.setSpeed(1, zones[1].scrollSpeed);
      P.setPause(1, zones[1].scrollPause);
      P.setTextAlignment(1, stringToTextPositionT(zones[1].scrollAlign));
      P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), stringToTextEffectT(zones[1].scrollEffectOut));
      P.setCharSpacing(1, zones[1].charSpasing);
      applyZoneFont(1, zones[1].font);
      if (zones[1].workMode == "mqttClient")  zone1NewMessage("MQTT");
      if (zones[1].workMode == "manualInput") zone1NewMessage("Manual");
      if (zones[1].workMode == "owmWeather") {
        if(zones[1].owmWhatToDisplay == "owmWeatherIcon") {
          P.setFont(1, wledSymbolFont);
          P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), PA_NO_EFFECT);
        }
      }
    }
    if(zoneNumbers > 2) {
      P.setSpeed(2, zones[2].scrollSpeed);
      P.setPause(2, zones[2].scrollPause);
      P.setTextAlignment(2, stringToTextPositionT(zones[2].scrollAlign));
      P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), stringToTextEffectT(zones[2].scrollEffectOut));
      P.setCharSpacing(2, zones[2].charSpasing);
      applyZoneFont(2, zones[2].font);
      if (zones[2].workMode == "mqttClient")  zone2NewMessage("MQTT");
      if (zones[2].workMode == "manualInput") zone2NewMessage("Manual");
      if (zones[2].workMode == "owmWeather") {
        if(zones[2].owmWhatToDisplay == "owmWeatherIcon") {
          P.setFont(2, wledSymbolFont);
          P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), PA_NO_EFFECT);
        }
      }
    }

    P.setIntensity(intensity);

    if (zones[0].workMode == "owmWeather" || zones[1].workMode == "owmWeather" || zones[2].workMode == "owmWeather") owmLastTime = -1000000;
    if (zones[0].workMode == "haClient" || zones[1].workMode == "haClient" || zones[2].workMode == "haClient") haLastTime = -1000000;

  } else if (!firstLoop && !secondLoop) {

    if (zones[0].workMode == "mqttClient" || zones[1].workMode == "mqttClient" || zones[2].workMode == "mqttClient" ){
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

    
    if (zones[0].workMode == "wallClock" || zones[1].workMode == "wallClock" || zones[2].workMode == "wallClock") {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 1000) {
        previousMillis = currentMillis;
        timeClient.update();
        if (zones[0].workMode == "wallClock") {
          String curTimeZone0New = getCurTime(zones[0].font, zones[0].clockDisplayFormat);
          if (curTimeZone0 == curTimeZone0New || curTimeZone0 == flashClockDots(curTimeZone0New)) {
            if (zones[0].clockDisplayFormat == "HHMM" || zones[0].clockDisplayFormat == "HHMMSS" ){
              if (curTimeZone0.indexOf(":") > 0) curTimeZone0New.replace(":", "¦");
              P.setTextEffect(0, PA_NO_EFFECT, PA_NO_EFFECT);
              curTimeZone0 = curTimeZone0New;
              zone0NewMessage(curTimeZone0);
            }
          } else {
            P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), PA_NO_EFFECT);
            if (curTimeZone0.indexOf(":") > 0) curTimeZone0New.replace(":", "¦");
            curTimeZone0 = curTimeZone0New;
            zone0NewMessage(curTimeZone0);
          }
        }

        if (zones[1].workMode == "wallClock" && zoneNumbers > 1) {
          String curTimeZone1New = getCurTime(zones[1].font, zones[1].clockDisplayFormat);
          if (curTimeZone1 == curTimeZone1New || curTimeZone1 == flashClockDots(curTimeZone1New)) {
            if (zones[1].clockDisplayFormat == "HHMM" || zones[0].clockDisplayFormat == "HHMMSS" ) {
              if (curTimeZone1.indexOf(":") > 0) curTimeZone1New.replace(":", "¦");
              P.setPause(1, 1);
              P.setTextEffect(1, PA_NO_EFFECT, PA_NO_EFFECT);
              curTimeZone1 = curTimeZone1New;
              zone1NewMessage(curTimeZone1);
            }
          } else {
            P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), PA_NO_EFFECT);
            if (curTimeZone1.indexOf(":") > 0) curTimeZone1New.replace(":", "¦");
            curTimeZone1 = curTimeZone1New;
            zone1NewMessage(curTimeZone1);
          }
        }

        if (zones[2].workMode == "wallClock" && zoneNumbers > 2) {
          String curTimeZone2New = getCurTime(zones[2].font, zones[2].clockDisplayFormat);
          if (curTimeZone2 == curTimeZone2New || curTimeZone2 == flashClockDots(curTimeZone2New)) {
            if (zones[2].clockDisplayFormat == "HHMM" || zones[0].clockDisplayFormat == "HHMMSS" ) {
              if (curTimeZone2.indexOf(":") > 0) curTimeZone2New.replace(":", "¦");
              P.setPause(2, 1);
              P.setTextEffect(2, PA_NO_EFFECT, PA_NO_EFFECT);
              curTimeZone2 = curTimeZone2New;
              zone2NewMessage(curTimeZone2);
            }
          } else {
            P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), PA_NO_EFFECT);
            if (curTimeZone2.indexOf(":") > 0) curTimeZone2New.replace(":", "¦");
            curTimeZone2 = curTimeZone2New;
            zone2NewMessage(curTimeZone2);
          }
        }
      }
    }

    if (zones[0].workMode == "owmWeather" || zones[1].workMode == "owmWeather" || zones[2].workMode == "owmWeather") {
      if (millis() - owmLastTime >= (unsigned)owmUpdateInterval * 1000) {
        owmLastTime = millis();
        if (zones[0].workMode == "owmWeather") zone0NewMessage(openWetherMapGetWeather(zones[0].owmWhatToDisplay));
        if (zones[1].workMode == "owmWeather" && zoneNumbers > 1) zone1NewMessage(openWetherMapGetWeather(zones[1].owmWhatToDisplay));
        if (zones[2].workMode == "owmWeather" && zoneNumbers > 2) zone2NewMessage(openWetherMapGetWeather(zones[2].owmWhatToDisplay));
      }
    }

    if (zones[0].workMode == "haClient" || zones[1].workMode == "haClient" || zones[2].workMode == "haClient") {
      if (millis() - haLastTime >= (unsigned)haUpdateInterval * 1000) {
        if (zones[0].workMode == "haClient") zone0NewMessage(haApiGet(zones[0].haSensorId, zones[0].haSensorPostfix));
        if (zones[1].workMode == "haClient" && zoneNumbers > 1) zone1NewMessage(haApiGet(zones[1].haSensorId, zones[1].haSensorPostfix));
        if (zones[2].workMode == "haClient" && zoneNumbers > 2) zone2NewMessage(haApiGet(zones[2].haSensorId, zones[2].haSensorPostfix));        
        haLastTime = millis();
      }
    }

  }
  displayAnimation();
}