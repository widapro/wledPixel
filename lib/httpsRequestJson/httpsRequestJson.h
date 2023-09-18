#ifndef HTTPS_REQUEST_JSON_H
#define HTTPS_REQUEST_JSON_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>

JsonObject httpsRequest(String addr, int port, String path, String token, bool https);

#endif