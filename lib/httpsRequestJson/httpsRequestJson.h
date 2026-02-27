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
#error "This ain't a ESP8266 or ESP32!"
#endif

extern bool isFetchingHttps;

// Returns true on success, fills the provided JsonDocument with the response
bool httpsRequestToDoc(const String &addr, int port, const String &path,
                       const String &token, bool https, JsonDocument &doc);

// Legacy function - kept for backward compatibility but marked as deprecated
// WARNING: This function has memory safety issues - use httpsRequestToDoc
// instead

#endif