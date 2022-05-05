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

#include "htmlFiles.h"
#include <wledFont.h>

  // MQTT settings //
String mqttServerAddress = "";
int mqttServerPort = 1883;
String mqttUsername = "";
String mqttPassword = "";
String mqttZone0Topic = "zone0_text";
String mqttZone1Topic = "zone1_text";
String mqttZone2Topic = "zone2_text";


WiFiClient espClient;
//WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
long lastReconnectAttempt = 0;

// Display config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // type of device hardware https://majicdesigns.github.io/MD_MAX72XX/page_hardware.html
//// Display pinout
#define DATA_PIN  D7                      // WeMos D1 mini GPIO13
#define CS_PIN    D6                      // WeMos D1 mini GPIO12
#define CLK_PIN   D5                      // WeMos D1 mini GPIO14
uint8_t MAX_DEVICES = 12;                            // number of device segments
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
uint8_t scrollSpeedZone0 = 35;                      // default frame delay value
uint8_t scrollSpeedZone1 = 35;                      // default frame delay value
uint8_t scrollSpeedZone2 = 35;                      // default frame delay value

textEffect_t scrollEffectZone0In = PA_SCROLL_LEFT;  // default scroll effect
textEffect_t scrollEffectZone0Out = PA_NO_EFFECT;   // default scroll effect
String scrollEffectStringZone0In = "pa_scroll_left";
String scrollEffectStringZone0Out = "pa_no_effect";

textEffect_t scrollEffectZone1In = PA_SCROLL_LEFT;  // default scroll effect
textEffect_t scrollEffectZone1Out = PA_NO_EFFECT;   // default scroll effect
String scrollEffectStringZone1In = "pa_scroll_left";
String scrollEffectStringZone1Out = "pa_no_effect";

textEffect_t scrollEffectZone2In = PA_SCROLL_LEFT;  // default scroll effect
textEffect_t scrollEffectZone2Out = PA_NO_EFFECT;   // default scroll effect
String scrollEffectStringZone2In = "pa_scroll_left";
String scrollEffectStringZone2Out = "pa_no_effect";

textPosition_t scrollAlignZone0 = PA_CENTER;        // default alligment
String scrollAlignStringZone0 = "pa_center";

textPosition_t scrollAlignZone1 = PA_CENTER;        // default alligment
String scrollAlignStringZone1 = "pa_center";

textPosition_t scrollAlignZone2 = PA_CENTER;        // default alligment
String scrollAlignStringZone2 = "pa_center";


uint16_t scrollPauseZone0 = 3000;                   // in milliseconds
uint16_t scrollPauseZone1 = 3000;                   // in milliseconds
uint16_t scrollPauseZone2 = 3000;                   // in milliseconds

char zone0Message[100] = "zone0";                        // welcome message
char zone1Message[100] = "zone1";                    // logo from custom font
char zone2Message[100] = "zone2";                    // logo from custom font
String zone0MessageMQTT;
String zone1MessageMQTT;
String zone2MessageMQTT;
String zone0MessageManual;
String zone1MessageManual;
String zone2MessageManual;
bool zone0newMessageAvailable = false;
bool zone1newMessageAvailable = false;
bool zone2newMessageAvailable = false;
bool zone0newMessageAvailableMQTT = false;
bool zone1newMessageAvailableMQTT = false;
bool zone2newMessageAvailableMQTT = false;
bool zone0newMessageAvailableManual = false;
bool zone1newMessageAvailableManual = false;
bool zone2newMessageAvailableManual = false;

char const *wifiAPname = "wled-AP";
char const *wifiAPpassword = "12345678";

static uint32_t lastTime = 0; // millis() memory
String curTime = "00:00";
int i = 0;

// Initialize NTP
String ntpServerAddress = "pool.ntp.org";
int ntpTimeZone = 3;
int clockDisplayUpdateTime = 60;
String clockDisplayFormat = "HHMM";
WiFiUDP ntpUDP;
//int timeOffset = 0;
NTPClient timeClient(ntpUDP, ntpServerAddress.c_str());

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server,&dns);

bool restartEsp = false;
bool firstLoop = true;
bool secondLoop = false;
bool newConfigAvailable = false;

//flag for saving data
bool shouldSaveConfig = false;
//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

// Variables to save from html
uint8_t zoneNumbers = 1;
uint8_t zone0Begin = 0;
uint8_t zone0End = 2;
uint8_t zone1Begin = 3;
uint8_t zone1End = 3;
uint8_t zone2Begin = 4;
uint8_t zone2End = 7;
uint8_t intensity = 7;

String workModeZone0 = "manualInput";
String workModeZone1 = "manualInput";
String workModeZone2 = "manualInput";

// variables for get weather func
static uint32_t owmLastTime = 0;
int owmUpdateInterval = 60;

static BearSSL::WiFiClientSecure owmClient;
static HTTPClient owmHttpGet;

String    owmApiToken;
String    owmUnitsFormat;
String    owmCity;
String    owmTemp;
String    owmPressure;
String    owmHumidity;
String    owmWindSpeed;
String    owmWeatherIcon;
String    owmWhatToDisplayZone0;
String    owmWhatToDisplayZone1;
String    owmWhatToDisplayZone2;


// Convert scrollAligString to scrollAlign
textPosition_t scrollAlignStringToScrollAlign(String val) {
  if(val == "pa_center") return PA_CENTER;
  if(val == "pa_left") return PA_LEFT;
  if(val == "pa_right") return PA_RIGHT;
  return PA_LEFT;
}

textEffect_t scrollEffectStringToScrollEffect(String val) {
  if(val == "pa_no_effect") return PA_NO_EFFECT;
  if(val == "pa_print") return PA_PRINT;
  if(val == "pa_scroll_up") return PA_SCROLL_UP;
  if(val == "pa_scroll_down") return PA_SCROLL_DOWN;
  if(val == "pa_scroll_left") return PA_SCROLL_LEFT;
  if(val == "pa_scroll_right") return PA_SCROLL_RIGHT;
  if(val == "pa_sprite") return PA_SPRITE;
  if(val == "pa_slice") return PA_SLICE;
  if(val == "pa_mesh") return PA_MESH;
  if(val == "pa_fade") return PA_FADE;
  if(val == "pa_dissolve") return PA_DISSOLVE;
  if(val == "pa_blinds") return PA_BLINDS;
  if(val == "pa_random") return PA_RANDOM;
  if(val == "pa_wipe") return PA_WIPE;
  if(val == "pa_wipe_cursor") return PA_WIPE_CURSOR;
  if(val == "pa_scan_horiz") return PA_SCAN_HORIZ;
  if(val == "pa_scan_horizx") return PA_SCAN_HORIZX;
  if(val == "pa_scan_vert") return PA_SCAN_VERT;
  if(val == "pa_scan_vertx") return PA_SCAN_VERTX;
  if(val == "pa_opening") return PA_OPENING;
  if(val == "pa_opening_cursor") return PA_OPENING_CURSOR;
  if(val == "pa_closing") return PA_CLOSING;
  if(val == "pa_closing_cursor") return PA_CLOSING_CURSOR;
  if(val == "pa_scroll_up_left") return PA_SCROLL_UP_LEFT;
  if(val == "pa_scroll_up_right") return PA_SCROLL_UP_RIGHT;
  if(val == "pa_scroll_down_left") return PA_SCROLL_DOWN_LEFT;
  if(val == "pa_scroll_down_right") return PA_SCROLL_DOWN_RIGHT;
  if(val == "pa_grow_up") return PA_GROW_UP;
  if(val == "pa_grow_down") return PA_GROW_DOWN;
  return PA_SCROLL_LEFT;
}


// Return values for variables in html page
String processor(const String& var){
  char buffer [10];
  if(var == "workModeZone0")                return workModeZone0;
  if(var == "workModeZone1")                return workModeZone1;
  if(var == "workModeZone2")                return workModeZone2;
  if(var == "zoneNumbers")                  return itoa(zoneNumbers, buffer, 10);
  if(var == "zone0Begin")                   return itoa(zone0Begin, buffer, 10);
  if(var == "zone0End")                     return itoa(zone0End, buffer, 10);
  if(var == "zone1Begin")                   return itoa(zone1Begin, buffer, 10);
  if(var == "zone1End")                     return itoa(zone1End, buffer, 10);
  if(var == "zone2Begin")                   return itoa(zone2Begin, buffer, 10);
  if(var == "zone2End")                     return itoa(zone2End, buffer, 10);
  if(var == "wifiSsid")                     return WiFi.SSID();
  if(var == "wifiIp")                       return WiFi.localIP().toString();
  if(var == "wifiGateway")                  return WiFi.gatewayIP().toString();
  if(var == "intensity")                    return itoa(intensity, buffer, 10);
  if(var == "scrollSpeedZone0")             return itoa(scrollSpeedZone0, buffer, 10);
  if(var == "scrollSpeedZone1")             return itoa(scrollSpeedZone1, buffer, 10);
  if(var == "scrollSpeedZone2")             return itoa(scrollSpeedZone2, buffer, 10);
  if(var == "scrollPauseZone0")             return itoa(scrollPauseZone0, buffer, 10);
  if(var == "scrollPauseZone1")             return itoa(scrollPauseZone1, buffer, 10);
  if(var == "scrollPauseZone2")             return itoa(scrollPauseZone2, buffer, 10);
  if(var == "scrollAlignStringZone0")       return scrollAlignStringZone0;
  if(var == "scrollAlignStringZone1")       return scrollAlignStringZone1;
  if(var == "scrollAlignStringZone2")       return scrollAlignStringZone2;
  if(var == "scrollEffectStringZone0In")    return scrollEffectStringZone0In;
  if(var == "scrollEffectStringZone0Out")   return scrollEffectStringZone0Out;
  if(var == "scrollEffectStringZone1In")    return scrollEffectStringZone1In;
  if(var == "scrollEffectStringZone1Out")   return scrollEffectStringZone1Out;
  if(var == "scrollEffectStringZone2In")    return scrollEffectStringZone2In;
  if(var == "scrollEffectStringZone2Out")   return scrollEffectStringZone2Out;
  if(var == "mqttServerAddress")            return mqttServerAddress;
  if(var == "mqttServerPort")               return itoa(mqttServerPort, buffer, 10);
  if(var == "mqttUsername")                 return mqttUsername;
  if(var == "mqttPassword")                 return mqttPassword;
  if(var == "mqttZone0Topic")               return mqttZone0Topic;
  if(var == "mqttZone1Topic")               return mqttZone1Topic;
  if(var == "mqttZone2Topic")               return mqttZone2Topic;
  if(var == "ntpTimeZone")                  return itoa(ntpTimeZone, buffer, 10);
  if(var == "clockDisplayUpdateTime")       return itoa(clockDisplayUpdateTime, buffer, 10);
  if(var == "clockDisplayFormat")           return clockDisplayFormat;
  if(var == "owmApiToken")                  return owmApiToken;
  if(var == "owmUnitsFormat")               return owmUnitsFormat;
  if(var == "owmUpdateInterval")            return itoa(owmUpdateInterval, buffer, 10);
  if(var == "owmWhatToDisplayZone0")        return owmWhatToDisplayZone0;
  if(var == "owmWhatToDisplayZone1")        return owmWhatToDisplayZone1;
  if(var == "owmWhatToDisplayZone2")        return owmWhatToDisplayZone2;
  if(var == "owmCity")                      return owmCity;
  return String();
}


void ConfigFile_Save_Variable(String VarName, String VarValue) {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("!!! ERROR !!! failed to open config file for writing");
    return;
  }

  DynamicJsonDocument doc(2048);
  deserializeJson(doc, configFile);
  configFile.close();

  doc[VarName] = VarValue;
  configFile = LittleFS.open("/config.json", "w");
  serializeJson(doc, configFile);
  configFile.close();
}

void ConfigFile_Read_Variable() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("!!! ERROR !!! failed to open config file for writing");
    return;
  }

  DynamicJsonDocument doc(2048);
  deserializeJson(doc, configFile);
  // print config file to serial
  serializeJsonPretty(doc, Serial);
  configFile.close();

  JsonObject postObj = doc.as<JsonObject>();
  if(postObj[F("workModeZone0")])             workModeZone0 = postObj[F("workModeZone0")].as<String>();
  if(postObj[F("workModeZone1")])             workModeZone1 = postObj[F("workModeZone1")].as<String>();
  if(postObj[F("workModeZone2")])             workModeZone2 = postObj[F("workModeZone2")].as<String>();
  if(postObj[F("zoneNumbers")])               zoneNumbers = postObj[F("zoneNumbers")].as<uint8_t>();
  if(postObj[F("zone0Begin")])                zone0Begin = postObj[F("zone0Begin")].as<uint8_t>();
  if(postObj[F("zone0End")])                  zone0End = postObj[F("zone0End")].as<uint8_t>();
  if(postObj[F("zone1Begin")])                zone1Begin = postObj[F("zone1Begin")].as<uint8_t>();
  if(postObj[F("zone1End")])                  zone1End = postObj[F("zone1End")].as<uint8_t>();
  if(postObj[F("zone2Begin")])                zone2Begin = postObj[F("zone2Begin")].as<uint8_t>();
  if(postObj[F("zone2End")])                  zone2End = postObj[F("zone2End")].as<uint8_t>();
  if(postObj[F("intensity")])                 intensity = postObj[F("intensity")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone0")])          scrollSpeedZone0 = postObj[F("scrollSpeedZone0")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone1")])          scrollSpeedZone1 = postObj[F("scrollSpeedZone1")].as<uint8_t>();
  if(postObj[F("scrollSpeedZone2")])          scrollSpeedZone2 = postObj[F("scrollSpeedZone2")].as<uint8_t>();
  if(postObj[F("scrollPauseZone0")])          scrollPauseZone0 = postObj[F("scrollPauseZone0")].as<uint16_t>();
  if(postObj[F("scrollPauseZone1")])          scrollPauseZone1 = postObj[F("scrollPauseZone1")].as<uint16_t>();
  if(postObj[F("scrollPauseZone2")])          scrollPauseZone2 = postObj[F("scrollPauseZone2")].as<uint16_t>();
  if(postObj[F("scrollAlignStringZone0")]) {
                                              scrollAlignStringZone0 = postObj[F("scrollAlignStringZone0")].as<String>();
                                              scrollAlignZone0 = scrollAlignStringToScrollAlign(scrollAlignStringZone0);
  }
  if(postObj[F("scrollAlignStringZone1")]) {
                                              scrollAlignStringZone1 = postObj[F("scrollAlignStringZone1")].as<String>();
                                              scrollAlignZone1 = scrollAlignStringToScrollAlign(scrollAlignStringZone1);
  }
  if(postObj[F("scrollAlignStringZone2")]) {
                                              scrollAlignStringZone2 = postObj[F("scrollAlignStringZone2")].as<String>();
                                              scrollAlignZone2 = scrollAlignStringToScrollAlign(scrollAlignStringZone2);
  }
  if(postObj[F("scrollEffectStringZone0In")]) {
                                              scrollEffectStringZone0In = postObj[F("scrollEffectStringZone0In")].as<String>();
                                              scrollEffectZone0In = scrollEffectStringToScrollEffect(scrollEffectStringZone0In);
  }
  if(postObj[F("scrollEffectStringZone0Out")]) {
                                              scrollEffectStringZone0Out = postObj[F("scrollEffectStringZone0Out")].as<String>();
                                              scrollEffectZone0Out = scrollEffectStringToScrollEffect(scrollEffectStringZone0Out);
  }
  if(postObj[F("scrollEffectStringZone1In")]) {
                                              scrollEffectStringZone1In = postObj[F("scrollEffectStringZone1In")].as<String>();
                                              scrollEffectZone1In = scrollEffectStringToScrollEffect(scrollEffectStringZone1In);
  }
  if(postObj[F("scrollEffectStringZone1Out")]) {
                                              scrollEffectStringZone1Out = postObj[F("scrollEffectStringZone1Out")].as<String>();
                                              scrollEffectZone1Out = scrollEffectStringToScrollEffect(scrollEffectStringZone1Out);
  }
  if(postObj[F("scrollEffectStringZone2In")]) {
                                              scrollEffectStringZone2In = postObj[F("scrollEffectStringZone2In")].as<String>();
                                              scrollEffectZone2In = scrollEffectStringToScrollEffect(scrollEffectStringZone2In);
  }
  if(postObj[F("scrollEffectStringZone2Out")]) {
                                              scrollEffectStringZone2Out = postObj[F("scrollEffectStringZone2Out")].as<String>();
                                              scrollEffectZone2Out = scrollEffectStringToScrollEffect(scrollEffectStringZone2Out);
  } 
  if(postObj[F("mqttServerAddress")])         mqttServerAddress = postObj[F("mqttServerAddress")].as<String>();
  if(postObj[F("mqttServerPort")])            mqttServerPort = postObj[F("mqttServerPort")].as<int>();
  if(postObj[F("mqttUsername")])              mqttUsername = postObj[F("mqttUsername")].as<String>();
  if(postObj[F("mqttPassword")])              mqttPassword = postObj[F("mqttPassword")].as<String>();
  if(postObj[F("mqttZone0Topic")])            mqttZone0Topic = postObj[F("mqttZone0Topic")].as<String>();
  if(postObj[F("mqttZone1Topic")])            mqttZone1Topic = postObj[F("mqttZone1Topic")].as<String>();
  if(postObj[F("mqttZone2Topic")])            mqttZone2Topic = postObj[F("mqttZone2Topic")].as<String>();
  if(postObj[F("ntpTimeZone")])               ntpTimeZone = postObj[F("ntpTimeZone")].as<int>();
  if(postObj[F("clockDisplayUpdateTime")])    clockDisplayUpdateTime = postObj[F("clockDisplayUpdateTime")].as<int>();
  if(postObj[F("clockDisplayFormat")])        clockDisplayFormat = postObj[F("clockDisplayFormat")].as<String>();
  if(postObj[F("owmApiToken")])               owmApiToken = postObj[F("owmApiToken")].as<String>();
  if(postObj[F("owmUnitsFormat")])            owmUnitsFormat = postObj[F("owmUnitsFormat")].as<String>();
  if(postObj[F("owmUpdateInterval")])         owmUpdateInterval = postObj[F("owmUpdateInterval")].as<int>();
  if(postObj[F("owmCity")])                   owmCity = postObj[F("owmCity")].as<String>();
  if(postObj[F("owmWhatToDisplayZone0")])     owmWhatToDisplayZone0 = postObj[F("owmWhatToDisplayZone0")].as<String>();
  if(postObj[F("owmWhatToDisplayZone1")])     owmWhatToDisplayZone1 = postObj[F("owmWhatToDisplayZone1")].as<String>();
  if(postObj[F("owmWhatToDisplayZone2")])     owmWhatToDisplayZone2 = postObj[F("owmWhatToDisplayZone2")].as<String>();
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

  if(strcmp(topic, (char*) mqttZone0Topic.c_str()) == 0) {
    zone0MessageMQTT = PayloadString.c_str();
    zone0newMessageAvailableMQTT = true; 
  }
  if(strcmp(topic, (char*) mqttZone1Topic.c_str()) == 0) {
    zone1MessageMQTT = PayloadString.c_str();
    zone1newMessageAvailableMQTT = true; 
  }
  if(strcmp(topic, (char*) mqttZone2Topic.c_str()) == 0) {
    zone2MessageMQTT = PayloadString.c_str();
    zone2newMessageAvailableMQTT = true; 
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
    mqttClient.subscribe((char*) mqttZone0Topic.c_str());
    mqttClient.subscribe((char*) mqttZone1Topic.c_str());
    mqttClient.subscribe((char*) mqttZone2Topic.c_str());
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
  P.setZone(0, zone0Begin, zone0End);
  P.displayClear();
  P.setTextAlignment(0, scrollAlignZone0);
  P.setIntensity(7);
  P.setSpeed(0, 100);
  P.write(wifiAPname);
  if (MAX_DEVICES > 5) {
    delay(5000);
    P.write(wifiAPpassword);
  }
}

String getCurTime() {
      lastTime = millis();
      String t = (String)timeClient.getFormattedTime(); // return HH:MM:SS

      if (clockDisplayFormat == "HHMM") {
        t.remove(5,4);
      }
      if (clockDisplayFormat == "HH") {
        t.remove(2,7);
      }
      if (clockDisplayFormat == "MM") {
        t.remove(5,4);
        t.remove(0,3);
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
    if (zone0newMessageAvailable) {
      
//      if (P.getZoneStatus(0)) {
        Serial.printf("\nzone0Message availabel: %s", zone0Message);
        zone0newMessageAvailable = false;
        P.setTextBuffer(0, zone0Message);
        P.displayReset(0);
    }
  

    if (zone1newMessageAvailable) {
//      if (P.getZoneStatus(1)) {
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

int openWetherMapGetWeather() {
  int status = 0;
  String URL = "https://api.openweathermap.org/data/2.5/weather?";
  URL = URL + "q=" + owmCity + "&units=" + owmUnitsFormat + "&appid=" + owmApiToken;

  owmClient.setInsecure();
  owmClient.setBufferSizes(1024, 1024);
  owmClient.setTimeout(2000);

  if (owmHttpGet.begin(owmClient, URL)) {
    int httpCode = owmHttpGet.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = owmHttpGet.getString();

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      JsonObject postObj = doc.as<JsonObject>();
      if(postObj[F("main")]) {
        int owmTempInt = round(postObj[F("main")][F("temp")].as<int>());
        owmTemp = String(owmTempInt) + "°";
        if(owmUnitsFormat == "metric") owmTemp = owmTemp + "C";
        if(owmUnitsFormat == "imperial") owmTemp = owmTemp + "F";
        owmPressure = postObj[F("main")][F("pressure")].as<String>();
        owmHumidity = postObj[F("main")][F("humidity")].as<String>() + "%";
      }
      if(postObj[F("wind")])      owmWindSpeed = postObj[F("wind")][F("speed")].as<int>();
      if(postObj[F("weather")]) {
        owmWeatherIcon = postObj[F("weather")][0][F("icon")].as<String>();
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

      }
    }
    else {
      status = httpCode;
      Serial.printf("OpenWeather api error: %s", owmHttpGet.errorToString(httpCode).c_str());
    }
  } else { 
    Serial.print("OpenWeather api error start connection");
    status = 1;
  }
  owmHttpGet.end();

  return status;
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

  // Web server routings
  //// route root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_index, processor);
  });
  
  //// route /config
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_settings, processor);
  });
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_settings, processor);
  });

  //// respond with json
  server.on("/api/wifi-info", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      DynamicJsonDocument json(1024);
      json["status"] = "ok";
      json["ssid"] = WiFi.SSID();
      json["ip"] = WiFi.localIP().toString();
      serializeJson(json, *response);
      request->send(response);
  });

  //// route POST request for config, store data to config file
  server.on("/configpost", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      Serial.printf("%d params sent in\n", params);
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          if (p->name() == "workModeZone0") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "workModeZone1") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "workModeZone2") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "zoneNumbers") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            restartEsp = true;
          }          
          if (p->name() == "zone0Begin") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            restartEsp = true;
          }
          if (p->name() == "zone0End") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            restartEsp = true;
          }
          if (p->name() == "zone1Begin") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            restartEsp = true;
          }
          if (p->name() == "zone1End") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            restartEsp = true;
          }
          if (p->name() == "zone2Begin") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            restartEsp = true;
          }
          if (p->name() == "zone2End") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            restartEsp = true;
          }
          if (strcmp(p->name().c_str(), "messageZone0") == 0) {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            zone0MessageManual = p->value().c_str();
            zone0newMessageAvailableManual = true;
          }
          if (strcmp(p->name().c_str(), "messageZone1") == 0) {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            zone1MessageManual = p->value().c_str();
            zone1newMessageAvailableManual = true; 
          }
          if (strcmp(p->name().c_str(), "messageZone2") == 0) {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            zone2MessageManual = p->value().c_str();
            zone2newMessageAvailableManual = true; 
          }
          if (p->name() == "intensity") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            P.setIntensity((p->value()).toInt());
          }
          if (p->name() == "scrollSpeedZone0") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollSpeedZone1") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollSpeedZone2") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollPauseZone0") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollPauseZone1") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollPauseZone2") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollAlignStringZone0") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollAlignStringZone1") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollAlignStringZone2") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollEffectStringZone0In") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollEffectStringZone0Out") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollEffectStringZone1In") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollEffectStringZone1Out") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollEffectStringZone2In") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "scrollEffectStringZone2Out") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "mqttServerAddress") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "mqttServerPort") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "mqttUsername") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "mqttPassword") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "mqttZone0Topic") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "mqttZone1Topic") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "mqttZone2Topic") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "ntpTimeZone") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "clockDisplayUpdateTime") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "clockDisplayFormat") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "owmApiToken") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "owmUnitsFormat") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "owmUpdateInterval") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "owmCity") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "owmWhatToDisplayZone0") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "owmWhatToDisplayZone1") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
          if (p->name() == "owmWhatToDisplayZone2") {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            ConfigFile_Save_Variable(p->name().c_str(), p->value().c_str());
            newConfigAvailable = true;
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"success\"}");
    });

    // reboot system
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "200 OK");
      restartEsp = true;
    });

  ConfigFile_Read_Variable();

  // Initializing display
  P.begin(zoneNumbers);
  // Zone 0 initializing
  P.setZone(0, zone0Begin, zone0End);
  P.setSpeed(0, scrollSpeedZone0);
  P.setPause(0, scrollPauseZone0);
  P.setTextEffect(0, scrollEffectZone0In, scrollEffectZone0Out);
  P.setTextAlignment(0, scrollAlignZone0);
  // Zone 1 initializing
  if(zoneNumbers > 1) {
    P.setZone(1, zone1Begin, zone1End);
    P.setSpeed(1, scrollSpeedZone1);
    P.setPause(1, scrollPauseZone1);
    P.setTextEffect(1, scrollEffectZone1In, scrollEffectZone1Out);
    P.setTextAlignment(1, scrollAlignZone1);
  }
  // Zone 2 initializing
  if(zoneNumbers > 2) {
    P.setZone(2, zone2Begin, zone2End);
    P.setSpeed(2, scrollSpeedZone2);
    P.setPause(2, scrollPauseZone2);
    P.setTextEffect(2, scrollEffectZone2In, scrollEffectZone2Out);
    P.setTextAlignment(2, scrollAlignZone2);
  }
  P.displayClear();
  P.setIntensity(intensity);
  P.setFont(wledFont);

  // Start MQTT client
  mqttClient.setServer(mqttServerAddress.c_str(), mqttServerPort);
  mqttClient.setCallback(MQTTCallback);
  
  // Start webserver
  server.begin();

  // start NTP client
  timeClient.begin();
  timeClient.setTimeOffset(ntpTimeZone * 3600);
  timeClient.update();
  curTime = getCurTime();
}


void loop() {
  if (secondLoop && P.getZoneStatus(0)) {
    P.displayClear();
    newConfigAvailable = true;
    if (workModeZone0 == "") workModeZone0 = "manualInput";
    secondLoop = false;
    //lastTime = -100000;
    //owmLastTime = -1000000;
  }
  if (firstLoop) {
    P.setSpeed(0, 80);
    P.setTextEffect(0, PA_SCROLL_LEFT, PA_NO_EFFECT);
    workModeZone0 = "firstLoop";
    zone0NewMessage("ip: " + WiFi.localIP().toString());
    firstLoop = false;
    secondLoop = true;
  }
  if (restartEsp) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  if (newConfigAvailable) {
    newConfigAvailable = false;
    ConfigFile_Read_Variable();

    P.setSpeed(0, scrollSpeedZone0);
    P.setPause(0, scrollPauseZone0);
    P.setTextAlignment(0, scrollAlignZone0);
    P.setTextEffect(0, scrollEffectZone0In, scrollEffectZone0Out);
    if(zoneNumbers > 1) {
      P.setSpeed(1, scrollSpeedZone1);
      P.setPause(1, scrollPauseZone1);
      P.setTextAlignment(1, scrollAlignZone1);
      P.setTextEffect(1, scrollEffectZone1In, scrollEffectZone1Out);
    }
    if(zoneNumbers > 2) {
      P.setSpeed(2, scrollSpeedZone2);
      P.setPause(2, scrollPauseZone2);
      P.setTextAlignment(2, scrollAlignZone2);
      P.setTextEffect(2, scrollEffectZone2In, scrollEffectZone2Out);
    }
    P.setIntensity(intensity);

    if (workModeZone0 == "manualInput") zone0NewMessage("Manual");
    if (workModeZone1 == "manualInput") zone1NewMessage("Manual");
    if (workModeZone2 == "manualInput") zone1NewMessage("Manual");

    if (workModeZone0 == "wallClock" || workModeZone1 == "wallClock" || workModeZone2 == "wallClock") {
      timeClient.setTimeOffset(ntpTimeZone * 3600);
      lastTime = 0;
      if (workModeZone0 == "wallClock")   zone1NewMessage(getCurTime().c_str());
      if (workModeZone1 == "wallClock")   zone1NewMessage(getCurTime().c_str());
      if (workModeZone2 == "wallClock")   zone1NewMessage(getCurTime().c_str());
    }
    if (workModeZone0 == "mqttClient" || workModeZone1 == "mqttClient" || workModeZone2 == "mqttClient") {
      mqttClient.disconnect();
      if (workModeZone0 == "mqttClient")  zone0NewMessage("MQTT");
      if (workModeZone1 == "mqttClient")  zone1NewMessage("MQTT");
      if (workModeZone2 == "mqttClient")  zone1NewMessage("MQTT");
    }
    
    if (workModeZone0 == "owmWeather") {
      owmLastTime = -1000000;
      if(owmWhatToDisplayZone0 == "owmWeatherIcon") {
        P.setFont(0, wledSymbolFont);
        P.setTextEffect(0, scrollEffectZone0In, PA_NO_EFFECT);
      }
      zone0NewMessage("OWM");
    }
    if (workModeZone1 == "owmWeather") {
      owmLastTime = -1000000;
      if(owmWhatToDisplayZone1 == "owmWeatherIcon") {
        P.setFont(1, wledSymbolFont);
        P.setTextEffect(1, scrollEffectZone1In, PA_NO_EFFECT);
      }
      zone1NewMessage("OWM");
    }
    if (workModeZone2 == "owmWeather") {
      owmLastTime = -1000000;
      if(owmWhatToDisplayZone2 == "owmWeatherIcon") {
        P.setFont(2, wledSymbolFont);
        P.setTextEffect(2, scrollEffectZone2In, PA_NO_EFFECT);
      }
      zone1NewMessage("OWM");
    }
  } else {

    if (workModeZone0 == "mqttClient" || workModeZone1 == "mqttClient" || workModeZone2 == "mqttClient" ){
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

    // MQTT message work mode handler
    if (workModeZone0 == "mqttClient" && zone0newMessageAvailableMQTT) {
      zone0newMessageAvailableMQTT = false;
      zone0NewMessage(zone0MessageMQTT.c_str());
    }
    if (workModeZone1 == "mqttClient" && zone1newMessageAvailableMQTT) {
      zone1newMessageAvailableMQTT = false;
      zone1NewMessage(zone1MessageMQTT.c_str());
    }
    if (workModeZone2 == "mqttClient" && zone2newMessageAvailableMQTT) {
      zone2newMessageAvailableMQTT = false;
      zone2NewMessage(zone2MessageMQTT.c_str());
    }

    // Manual message work mode handler
    if (workModeZone0 == "manualInput" && zone0newMessageAvailableManual) {
      zone0newMessageAvailableManual = false;
      zone0NewMessage(zone0MessageManual.c_str());
    }
    if (workModeZone1 == "manualInput" && zone1newMessageAvailableManual) {
      zone1newMessageAvailableManual = false;
      zone1NewMessage(zone1MessageManual.c_str());
    }
    if (workModeZone2 == "manualInput" && zone2newMessageAvailableManual) {
      zone2newMessageAvailableManual = false;
      zone2NewMessage(zone2MessageManual.c_str());
    }

    // Wall clock work mode handler
    if (workModeZone0 == "wallClock" || workModeZone1 == "wallClock" || workModeZone2 == "wallClock") {
      timeClient.update();
      int lastUpdateIntevar = millis() - lastTime;

      if (workModeZone0 == "wallClock" && P.getZoneStatus(0)) {
        if (!(lastUpdateIntevar % 1000)) {
          curTime = flashClockDots(curTime);
          P.setPause(0, 1);
          P.setTextEffect(0, PA_NO_EFFECT, PA_NO_EFFECT);
          zone0NewMessage(curTime.c_str());
        }

        if (lastUpdateIntevar >= clockDisplayUpdateTime * 1000) {
          if (i == 0) {
            P.setTextEffect(0, PA_NO_EFFECT, scrollEffectZone0Out);
            i++;
          } else {
            curTime = getCurTime();
            P.setTextEffect(0, scrollEffectZone0In, PA_NO_EFFECT);
            i = 0;
          }
          zone0NewMessage(curTime.c_str());
        }
      }

      if (workModeZone1 == "wallClock" && P.getZoneStatus(1)) {
        if (!(lastUpdateIntevar % 1000)) {
          curTime = flashClockDots(curTime);
          P.setPause(1, 1);
          P.setTextEffect(1, PA_NO_EFFECT, PA_NO_EFFECT);
          zone1NewMessage(curTime.c_str());
        }

        if (lastUpdateIntevar >= clockDisplayUpdateTime * 1000) {
          if (i == 0) {
            P.setTextEffect(1, PA_NO_EFFECT, scrollEffectZone1Out);
            i++;
          } else {
            curTime = getCurTime();
            P.setTextEffect(1, scrollEffectZone1In, PA_NO_EFFECT);
            i = 0;
          }
          zone1NewMessage(curTime.c_str());
        }
      }

      if (workModeZone2 == "wallClock" && P.getZoneStatus(2)) {
        if (!(lastUpdateIntevar % 1000)) {
          curTime = flashClockDots(curTime);
          P.setPause(2, 1);
          P.setTextEffect(2, PA_NO_EFFECT, PA_NO_EFFECT);
          zone2NewMessage(curTime.c_str());
        }

        if (lastUpdateIntevar >= clockDisplayUpdateTime * 1000) {
          if (i == 0) {
            P.setTextEffect(2, PA_NO_EFFECT, scrollEffectZone2Out);
            i++;
          } else {
            curTime = getCurTime();
            P.setTextEffect(2, scrollEffectZone2In, PA_NO_EFFECT);
            i = 0;
          }
          zone2NewMessage(curTime.c_str());
        }
      }      
    }


    if (workModeZone0 == "owmWeather" || workModeZone1 == "owmWeather" || workModeZone2 == "owmWeather") {
      if (millis() - owmLastTime >= (unsigned)owmUpdateInterval * 1000) {
        if (openWetherMapGetWeather() == 0) {
          //if (workModeZone0 == "owmWeather" && P.getZoneStatus(0)) {
          if (workModeZone0 == "owmWeather") {
            if (owmWhatToDisplayZone0 == "owmTemperature")  zone0NewMessage(owmTemp);
            if (owmWhatToDisplayZone0 == "owmHumidity")     zone0NewMessage(owmHumidity);
            if (owmWhatToDisplayZone0 == "owmPressure")     zone0NewMessage(owmPressure);
            if (owmWhatToDisplayZone0 == "owmWindSpeed")    zone0NewMessage(owmWindSpeed);
            if (owmWhatToDisplayZone0 == "owmWeatherIcon")  zone0NewMessage(owmWeatherIcon);
          }
          //if (workModeZone1 == "owmWeather" && P.getZoneStatus(1)) {
          if (workModeZone1 == "owmWeather") {
            if (owmWhatToDisplayZone1 == "owmTemperature")  zone1NewMessage(owmTemp);
            if (owmWhatToDisplayZone1 == "owmHumidity")     zone1NewMessage(owmHumidity);
            if (owmWhatToDisplayZone1 == "owmPressure")     zone1NewMessage(owmPressure);
            if (owmWhatToDisplayZone1 == "owmWindSpeed")    zone1NewMessage(owmWindSpeed);
            if (owmWhatToDisplayZone1 == "owmWeatherIcon")  zone1NewMessage(owmWeatherIcon);
          }
          if (workModeZone2 == "owmWeather") {
            if (owmWhatToDisplayZone2 == "owmTemperature")  zone2NewMessage(owmTemp);
            if (owmWhatToDisplayZone2 == "owmHumidity")     zone2NewMessage(owmHumidity);
            if (owmWhatToDisplayZone2 == "owmPressure")     zone2NewMessage(owmPressure);
            if (owmWhatToDisplayZone2 == "owmWindSpeed")    zone2NewMessage(owmWindSpeed);
            if (owmWhatToDisplayZone2 == "owmWeatherIcon")  zone2NewMessage(owmWeatherIcon);
          }          
          owmLastTime = millis();
        }
      }
    }

  }
  displayAnimation();
}
