#ifndef HTTPS_REQUEST_JSON_H
#define HTTPS_REQUEST_JSON_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#if defined(ESP8266)
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
  
#else
  #error "This ain't a ESP8266 or ESP32, dumbo!"
#endif

JsonObject httpsRequest(String addr, int port, String path, String token, bool https);

#endif