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
uint8_t MAX_DEVICES = 12;                 // number of device segments
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

typedef struct {
    uint8_t   begin, end;
    uint8_t   scrollSpeed;
    uint16_t  scrollPause;
    String    scrollAlign, scrollEffectIn, scrollEffectOut, workMode;
} ZoneData;

ZoneData zones[] = {
  {0, 3, 35, 3000, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "manualInput"},
  {4, 5, 35, 3000, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "manualInput"},
  {6, 7, 35, 3000, "PA_CENTER", "PA_SCROLL_DOWN", "PA_NO_EFFECT", "manualInput"},
};

uint8_t zoneNumbers = 1;
uint8_t intensity = 7;

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


// Convert scrollAlig in String to textPosition_t type
textPosition_t stringToTextPositionT(String val) {
  if(val == "PA_CENTER") return PA_CENTER;
  if(val == "PA_LEFT") return PA_LEFT;
  if(val == "PA_RIGHT") return PA_RIGHT;
  return PA_LEFT;
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

  if(zones[0].workMode == "mqttClient" && strcmp(topic, (char*) mqttZone0Topic.c_str()) == 0) zone0NewMessage(PayloadString.c_str());
  if(zones[1].workMode == "mqttClient" && strcmp(topic, (char*) mqttZone1Topic.c_str()) == 0) zone1NewMessage(PayloadString.c_str());
  if(zones[2].workMode == "mqttClient" && strcmp(topic, (char*) mqttZone2Topic.c_str()) == 0) zone2NewMessage(PayloadString.c_str());
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
          if (p->name() == "mqttZone0Topic")              writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "mqttZone1Topic")              writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "mqttZone2Topic")              writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "ntpTimeZone")                 writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "clockDisplayUpdateTime")      writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "clockDisplayFormat")          writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmApiToken")                 writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmUnitsFormat")              writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmUpdateInterval")           writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmCity")                     writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmWhatToDisplayZone0")       writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmWhatToDisplayZone1")       writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "owmWhatToDisplayZone2")       writeVarToConfFile(p->name().c_str(), p->value().c_str(), true, false);
          if (p->name() == "intensity") {                 writeVarToConfFile(p->name().c_str(), p->value().c_str(), false, false);
            P.setIntensity((p->value()).toInt());
          }
          if (strcmp(p->name().c_str(), "messageZone0") == 0) {
            zone0MessageManual = p->value().c_str();
            zone0newMessageAvailableManual = true;
          }
          if (strcmp(p->name().c_str(), "messageZone1") == 0) {
            zone1MessageManual = p->value().c_str();
            zone1newMessageAvailableManual = true; 
          }
          if (strcmp(p->name().c_str(), "messageZone2") == 0) {
            zone2MessageManual = p->value().c_str();
            zone2newMessageAvailableManual = true;
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"success\"}");
    });

    // reboot system
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "200 OK");
      restartESP = true;
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
  // Zone 1 initializing
  if(zoneNumbers > 1) {
    P.setZone(1, zones[1].begin, zones[1].end);
    P.setSpeed(1, zones[1].scrollSpeed);
    P.setPause(1, zones[1].scrollPause);
    P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), stringToTextEffectT(zones[2].scrollEffectOut));
    P.setTextAlignment(1, stringToTextPositionT(zones[1].scrollAlign));
  }
  // Zone 2 initializing
  if(zoneNumbers > 2) {
    P.setZone(2, zones[2].begin, zones[2].end);
    P.setSpeed(2, zones[2].scrollSpeed);
    P.setPause(2, zones[2].scrollPause);
    P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), stringToTextEffectT(zones[2].scrollEffectOut));
    P.setTextAlignment(2, stringToTextPositionT(zones[2].scrollAlign));
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
    if (zones[0].workMode == "") zones[0].workMode = "manualInput";
    secondLoop = false;
    //lastTime = -100000;
    //owmLastTime = -1000000;
  }
  if (firstLoop) {
    P.setSpeed(0, 80);
    P.setTextEffect(0, PA_SCROLL_LEFT, PA_NO_EFFECT);
    zones[0].workMode = "firstLoop";
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
    if(zoneNumbers > 1) {
      P.setSpeed(1, zones[1].scrollSpeed);
      P.setPause(1, zones[1].scrollPause);
      P.setTextAlignment(1, stringToTextPositionT(zones[1].scrollAlign));
      P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), stringToTextEffectT(zones[1].scrollEffectOut));
    }
    if(zoneNumbers > 2) {
      P.setSpeed(2, zones[2].scrollSpeed);
      P.setPause(2, zones[2].scrollPause);
      P.setTextAlignment(2, stringToTextPositionT(zones[2].scrollAlign));
      P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), stringToTextEffectT(zones[2].scrollEffectOut));
    }
    P.setIntensity(intensity);

    if (zones[0].workMode == "manualInput") zone0NewMessage("Manual");
    if (zones[1].workMode == "manualInput") zone1NewMessage("Manual");
    if (zones[2].workMode == "manualInput") zone2NewMessage("Manual");

    if (zones[0].workMode == "wallClock" || zones[1].workMode == "wallClock" || zones[2].workMode == "wallClock") {
      timeClient.setTimeOffset(ntpTimeZone * 3600);
      lastTime = 0;
      if (zones[0].workMode == "wallClock")   zone0NewMessage(getCurTime().c_str());
      if (zones[1].workMode == "wallClock")   zone1NewMessage(getCurTime().c_str());
      if (zones[2].workMode == "wallClock")   zone2NewMessage(getCurTime().c_str());
    }
    if (zones[0].workMode == "mqttClient" || zones[1].workMode == "mqttClient" || zones[2].workMode == "mqttClient") {
      mqttClient.disconnect();
      if (zones[0].workMode == "mqttClient")  zone0NewMessage("MQTT");
      if (zones[1].workMode == "mqttClient")  zone1NewMessage("MQTT");
      if (zones[2].workMode == "mqttClient")  zone2NewMessage("MQTT");
    }
    
    if (zones[0].workMode == "owmWeather") {
      owmLastTime = -1000000;
      if(owmWhatToDisplayZone0 == "owmWeatherIcon") {
        P.setFont(0, wledSymbolFont);
        P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), PA_NO_EFFECT);
      }
      zone0NewMessage("OWM");
    }
    if (zones[1].workMode == "owmWeather") {
      owmLastTime = -1000000;
      if(owmWhatToDisplayZone1 == "owmWeatherIcon") {
        P.setFont(1, wledSymbolFont);
        P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), PA_NO_EFFECT);
      }
      zone1NewMessage("OWM");
    }
    if (zones[2].workMode == "owmWeather") {
      owmLastTime = -1000000;
      if(owmWhatToDisplayZone2 == "owmWeatherIcon") {
        P.setFont(2, wledSymbolFont);
        P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), PA_NO_EFFECT);
      }
      zone2NewMessage("OWM");
    }
  } else {

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

    // MQTT message work mode handler
    //if (zones[0].workMode == "mqttClient" && zone0newMessageAvailableMQTT) {
    //  zone0newMessageAvailableMQTT = false;
    //  zone0NewMessage(zone0MessageMQTT.c_str());
    //}
    //if (zones[1].workMode == "mqttClient" && zone1newMessageAvailableMQTT) {
    //  zone1newMessageAvailableMQTT = false;
    //  zone1NewMessage(zone1MessageMQTT.c_str());
    //}
    //if (zones[2].workMode == "mqttClient" && zone2newMessageAvailableMQTT) {
    //  zone2newMessageAvailableMQTT = false;
    //  zone2NewMessage(zone2MessageMQTT.c_str());
    //}

    // Manual message work mode handler
    if (zones[0].workMode == "manualInput" && zone0newMessageAvailableManual) {
      zone0newMessageAvailableManual = false;
      zone0NewMessage(zone0MessageManual.c_str());
    }
    if (zones[1].workMode == "manualInput" && zone1newMessageAvailableManual) {
      zone1newMessageAvailableManual = false;
      zone1NewMessage(zone1MessageManual.c_str());
    }
    if (zones[2].workMode == "manualInput" && zone2newMessageAvailableManual) {
      zone2newMessageAvailableManual = false;
      zone2NewMessage(zone2MessageManual.c_str());
    }

    // Wall clock work mode handler
    if (zones[0].workMode == "wallClock" || zones[1].workMode == "wallClock" || zones[2].workMode == "wallClock") {
      timeClient.update();
      int lastUpdateIntevar = millis() - lastTime;

      if (zones[0].workMode == "wallClock" && P.getZoneStatus(0)) {
        if (!(lastUpdateIntevar % 1000)) {
          curTime = flashClockDots(curTime);
          P.setPause(0, 1);
          P.setTextEffect(0, PA_NO_EFFECT, PA_NO_EFFECT);
          zone0NewMessage(curTime.c_str());
        }

        if (lastUpdateIntevar >= clockDisplayUpdateTime * 1000) {
          if (i == 0) {
            P.setTextEffect(0, PA_NO_EFFECT, stringToTextEffectT(zones[0].scrollEffectOut));
            i++;
          } else {
            curTime = getCurTime();
            P.setTextEffect(0, stringToTextEffectT(zones[0].scrollEffectIn), PA_NO_EFFECT);
            i = 0;
          }
          zone0NewMessage(curTime.c_str());
        }
      }

      if (zones[1].workMode == "wallClock" && P.getZoneStatus(1)) {
        if (!(lastUpdateIntevar % 1000)) {
          curTime = flashClockDots(curTime);
          P.setPause(1, 1);
          P.setTextEffect(1, PA_NO_EFFECT, PA_NO_EFFECT);
          zone1NewMessage(curTime.c_str());
        }

        if (lastUpdateIntevar >= clockDisplayUpdateTime * 1000) {
          if (i == 0) {
            P.setTextEffect(1, PA_NO_EFFECT, stringToTextEffectT(zones[1].scrollEffectOut));
            i++;
          } else {
            curTime = getCurTime();
            P.setTextEffect(1, stringToTextEffectT(zones[1].scrollEffectIn), PA_NO_EFFECT);
            i = 0;
          }
          zone1NewMessage(curTime.c_str());
        }
      }

      if (zones[2].workMode == "wallClock" && P.getZoneStatus(2)) {
        if (!(lastUpdateIntevar % 1000)) {
          curTime = flashClockDots(curTime);
          P.setPause(2, 1);
          P.setTextEffect(2, PA_NO_EFFECT, PA_NO_EFFECT);
          zone2NewMessage(curTime.c_str());
        }

        if (lastUpdateIntevar >= clockDisplayUpdateTime * 1000) {
          if (i == 0) {
            P.setTextEffect(2, PA_NO_EFFECT, stringToTextEffectT(zones[2].scrollEffectOut));
            i++;
          } else {
            curTime = getCurTime();
            P.setTextEffect(2, stringToTextEffectT(zones[2].scrollEffectIn), PA_NO_EFFECT);
            i = 0;
          }
          zone2NewMessage(curTime.c_str());
        }
      }      
    }


    if (zones[0].workMode == "owmWeather" || zones[1].workMode == "owmWeather" || zones[2].workMode == "owmWeather") {
      if (millis() - owmLastTime >= (unsigned)owmUpdateInterval * 1000) {
        if (openWetherMapGetWeather() == 0) {
          if (zones[0].workMode == "owmWeather") {
            if (owmWhatToDisplayZone0 == "owmTemperature")  zone0NewMessage(owmTemp);
            if (owmWhatToDisplayZone0 == "owmHumidity")     zone0NewMessage(owmHumidity);
            if (owmWhatToDisplayZone0 == "owmPressure")     zone0NewMessage(owmPressure);
            if (owmWhatToDisplayZone0 == "owmWindSpeed")    zone0NewMessage(owmWindSpeed);
            if (owmWhatToDisplayZone0 == "owmWeatherIcon")  zone0NewMessage(owmWeatherIcon);
          }
          if (zones[1].workMode == "owmWeather") {
            if (owmWhatToDisplayZone1 == "owmTemperature")  zone1NewMessage(owmTemp);
            if (owmWhatToDisplayZone1 == "owmHumidity")     zone1NewMessage(owmHumidity);
            if (owmWhatToDisplayZone1 == "owmPressure")     zone1NewMessage(owmPressure);
            if (owmWhatToDisplayZone1 == "owmWindSpeed")    zone1NewMessage(owmWindSpeed);
            if (owmWhatToDisplayZone1 == "owmWeatherIcon")  zone1NewMessage(owmWeatherIcon);
          }
          if (zones[2].workMode == "owmWeather") {
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