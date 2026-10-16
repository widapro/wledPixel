#include "httpsRequestJson.h"
#include "../../src/WebPriority.h"

#if defined(ESP8266)
// (Unused includes removed to prevent SPI concurrent access issues)
#endif

bool isFetchingHttps = false;
volatile bool httpsAbortRequested = false;
#include <memory>

struct HttpsGuard {
  explicit HttpsGuard(bool tls) : ready(false) {
    httpsAbortRequested = false;
    ready = beginExternalNetworkRequest(tls);
    isFetchingHttps = ready;
  }
  ~HttpsGuard() {
    isFetchingHttps = false;
    if (ready)
      endExternalNetworkRequest();
  }
  bool ready;
};

void requestHttpsAbortForWeb() { httpsAbortRequested = true; }

bool httpsAbortWasRequested() { return httpsAbortRequested; }

// Internal helper function to perform the HTTP request and parse directly
static bool performHttpRequestToDoc(const String &addr, int port,
                                    const String &path, const String &token,
                                    bool https, JsonDocument &doc) {
  HttpsGuard guard(https);
  if (!guard.ready) {
    Serial.print(F("\n[NET] External request deferred for web/API priority"));
    return false;
  }
#if defined(ESP8266)
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t maxBlock = ESP.getMaxFreeBlockSize();
  const uint32_t minHeap = https ? 6500 : 4000;
  const uint32_t minBlock = https ? 3200 : 1500;
  if (freeHeap < minHeap || maxBlock < minBlock) {
    Serial.printf("\n[NET] Request deferred after memory release. "
                  "TLS=%s Heap=%u/%u MaxBlock=%u/%u",
                  https ? "yes" : "no", freeHeap, minHeap, maxBlock,
                  minBlock);
    return false;
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
    // ESP8266 core clamps these to its documented 512-byte data minimum and
    // adds TLS record overhead internally (837 RX + 597 TX allocated).
    clientSsl->setBufferSizes(512, 512);
    clientSsl->setTimeout(1000);
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
    espClient->setTimeout(1000);
    httpBegin = httpClient.begin(*espClient, addr, port, path, false);
  }

  if (httpBegin) {
    if (token.length() > 0) {
      httpClient.addHeader("Authorization", token);
    }

    int httpCode = httpClient.GET();

    if (httpsAbortRequested) {
      Serial.print(F("\n[HTTPS] Aborted for incoming web request"));
      success = false;
      httpClient.end();
      if (clientSsl)
        clientSsl->stop();
      if (espClient)
        espClient->stop();
      return false;
    }

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
        // HA and most other JSON APIs return a short explanation for 4xx
        // responses. Parse it into the caller-provided document instead of
        // allocating a heap-backed response String.
        int errorSize = httpClient.getSize();
        if (httpCode >= 400 && errorSize > 0 && errorSize <= 512) {
          doc.clear();
          DeserializationError error =
              deserializeJson(doc, httpClient.getStream());
          if (!error) {
            const char *message = doc[F("message")].as<const char *>();
            if (message && message[0] != '\0')
              Serial.printf(" message='%s'", message);
          }
        }
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
