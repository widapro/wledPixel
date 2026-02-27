#include "httpsRequestJson.h"

#if defined(ESP8266)
// (Unused includes removed to prevent SPI concurrent access issues)
#endif

bool isFetchingHttps = false;
#include <memory>

struct HttpsGuard {
  HttpsGuard() { isFetchingHttps = true; }
  ~HttpsGuard() { isFetchingHttps = false; }
};

// Internal helper function to perform the HTTP request and parse directly
static bool performHttpRequestToDoc(const String &addr, int port,
                                    const String &path, const String &token,
                                    bool https, JsonDocument &doc) {
  HttpsGuard guard;
#if defined(ESP8266)
  if (https) {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlock = ESP.getMaxFreeBlockSize();
    // Require 18KB total free heap and 9KB max block. BearSSL with reduced
    // buffer sizes (setBufferSizes 2048+1024) needs ~14-16KB total, but we
    // keep 18KB headroom to survive heap fragmentation after a prior fetch.
    // If maxBlock falls below 9KB, BearSSL handshake may throw std::bad_alloc.
    if (freeHeap < 18432 || maxBlock < 9216) {
      Serial.printf(
          "\n[HTTPS] Aborting TLS due to low memory. Heap: %u, MaxBlock: %u\n",
          freeHeap, maxBlock);
      return false;
    }
  }
#endif

  bool httpBegin = false;
  bool success = false;
  HTTPClient httpClient;

#if defined(ESP8266)
  std::unique_ptr<BearSSL::WiFiClientSecure> clientSsl;
#elif defined(ESP32)
  std::unique_ptr<WiFiClientSecure> clientSsl;
#endif
  std::unique_ptr<WiFiClient> espClient;

  if (https) {
#if defined(ESP8266)
    // Use std::nothrow to prevent unhandled C++ exceptions (OOM) if memory is
    // too fragmented
    clientSsl.reset(new (std::nothrow) BearSSL::WiFiClientSecure);
    if (!clientSsl) {
      Serial.println(F("\n[HTTPS] Failed to allocate BearSSL client (OOM)."));
      return false;
    }
    clientSsl->setInsecure();
    clientSsl->setBufferSizes(2048, 1024);
    clientSsl->setTimeout(2000);
#elif defined(ESP32)
    clientSsl.reset(new (std::nothrow) WiFiClientSecure);
    if (!clientSsl)
      return false;
    clientSsl->setInsecure();
#endif
    httpBegin = httpClient.begin(*clientSsl, addr, port, path, true);
  } else {
    espClient.reset(new (std::nothrow) WiFiClient);
    if (!espClient)
      return false;
    espClient->setTimeout(2000);
    httpBegin = httpClient.begin(*espClient, addr, port, path, false);
  }

  if (httpBegin) {
    if (token.length() > 0) {
      httpClient.addHeader("Authorization", token);
    }

    int httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_OK) {
      Serial.print(F("\n[HTTPS] Start json parsing from stream"));
      // Parse directly from stream to avoid massive String heap allocation
      DeserializationError error = deserializeJson(doc, httpClient.getStream());
      if (error) {
        Serial.printf("\n[HTTPS] JSON parse error: %s", error.c_str());
        success = false;
      } else {
        success = true;
      }
    } else {
      if (httpCode > 0) {
        Serial.printf("\n[HTTPS] HTTP status error: %d", httpCode);
      } else {
        Serial.printf("\n[HTTPS] api error: %s",
                      httpClient.errorToString(httpCode).c_str());
      }
      success = false;
    }
  } else {
    Serial.print(F("\n[HTTPS] api error start connection"));
    success = false;
  }

  httpClient.end();

  // Force BearSSL to release its 5KB memory block immediately
#if defined(ESP8266) || defined(ESP32)
  if (clientSsl)
    clientSsl->stop();
#endif
  if (espClient)
    espClient->stop();

  return success;
}

// New safe function - caller provides the JsonDocument
bool httpsRequestToDoc(const String &addr, int port, const String &path,
                       const String &token, bool https, JsonDocument &doc) {
  return performHttpRequestToDoc(addr, port, path, token, https, doc);
}
