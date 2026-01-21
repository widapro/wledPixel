#include "httpsRequestJson.h"

// Internal helper function to perform the HTTP request
static String performHttpRequest(String addr, int port, String path,
                                 String token, bool https) {
  String payload;
  bool httpBegin;

  WiFiClient espClient;

#if defined(ESP8266)
  static BearSSL::WiFiClientSecure httpClientSsl;
  httpClientSsl.setBufferSizes(2048, 1024);
#elif defined(ESP32)
  WiFiClientSecure httpClientSsl;
#endif

  // Use stack-allocated HTTPClient instead of heap
  HTTPClient httpClient;
  httpClientSsl.setInsecure();
  httpClientSsl.setTimeout(2000);

  if (https)
    httpBegin = httpClient.begin(httpClientSsl, addr, port, path, true);
  else
    httpBegin = httpClient.begin(espClient, addr, port, path, false);

  if (httpBegin) {
    if (token.length() > 0) {
      httpClient.addHeader("Authorization", token);
    }
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) {
      payload = httpClient.getString();
    } else {
      Serial.printf("api error: %s",
                    httpClient.errorToString(httpCode).c_str());
      payload = "err" + String(httpCode);
    }
  } else {
    Serial.print(F("api error start connection"));
    payload = "err";
  }
  httpClient.end();

  return payload;
}

// New safe function - caller provides the JsonDocument
bool httpsRequestToDoc(String addr, int port, String path, String token,
                       bool https, JsonDocument &doc) {
  String payload = performHttpRequest(addr, port, path, token, https);

  Serial.print(F("\nStart json parsing in httpRequest func"));
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.printf("\nJSON parse error: %s", error.c_str());
    return false;
  }
  return true;
}

// Legacy function - kept for backward compatibility
// Uses a static buffer to avoid returning reference to local variable
// WARNING: Not thread-safe, buffer is overwritten on each call
JsonObject httpsRequest(String addr, int port, String path, String token,
                        bool https) {
  // Static document persists between calls - prevents memory leak but is NOT
  // thread-safe
  static StaticJsonDocument<2048> staticDoc;
  staticDoc.clear();

  String payload = performHttpRequest(addr, port, path, token, https);

  Serial.print(F("\nStart json in httpRequest func"));
  deserializeJson(staticDoc, payload);

  return staticDoc.as<JsonObject>();
}
