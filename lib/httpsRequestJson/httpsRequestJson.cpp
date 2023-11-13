#include "httpsRequestJson.h"

JsonObject httpsRequest(String addr, int port, String path, String token, bool https) {
  String payload;
  bool httpBegin;

  WiFiClient espClient;

  #if defined(ESP8266)
    static BearSSL::WiFiClientSecure httpClientSsl;
    httpClientSsl.setBufferSizes(2048, 1024);
  #elif defined(ESP32)
    WiFiClientSecure httpClientSsl;
  #endif

  HTTPClient *httpClient = nullptr; 
  httpClient = new HTTPClient;
  httpClientSsl.setInsecure();
  
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
    Serial.print(F("api error start connection"));
    payload = "err";
  }
  httpClient->end();
  delete httpClient;
  
  Serial.print(F("\nStart json in httpRequest func"));
  DynamicJsonDocument httpsRespond(2048);
  deserializeJson(httpsRespond, payload);
  JsonObject httpsRespondPostObj = httpsRespond.as<JsonObject>();
  
  return httpsRespondPostObj;
}
