#include "httpsRequestJson.h"
#include "NetworkRequestGuard.h"
#include <memory>

#if defined(ESP8266)
#include <umm_malloc/umm_heap_select.h>
#ifndef UMM_HEAP_IRAM
#error "ESP8266 HTTPS requires the secondary IRAM heap configured in platformio.ini"
#endif
#endif

bool isFetchingHttps = false;
volatile bool httpsAbortRequested = false;

void requestHttpsAbortForWeb() { httpsAbortRequested = true; }
bool httpsAbortWasRequested() { return httpsAbortRequested; }

#if defined(ESP8266)
static networkMemory::Heap dramHeap() {
  HeapSelectDram heap;
  return {ESP.getFreeHeap(), ESP.getMaxFreeBlockSize()};
}
static networkMemory::Heap iramHeap() {
  HeapSelectIram heap;
  return {ESP.getFreeHeap(), ESP.getMaxFreeBlockSize()};
}
#endif

networkMemory::Heap secondaryNetworkHeap() {
#if defined(ESP8266)
  return iramHeap();
#else
  return {0, 0};
#endif
}

NetworkRequestResult requestJson(const String &addr, int port,
                                const String &path, const String &token,
                                bool https, JsonDocument &doc,
                                const JsonDocument *filter) {
  HttpsGuard guard(https);
  if (!guard.ready)
    return NetworkRequestResult::Deferred;

#if defined(ESP8266)
  auto dram = dramHeap();
  if (dram.freeBytes < 4096 || dram.largestBlock < 1536) {
    Serial.println(F("\n[NET] Request deferred: insufficient TCP memory"));
    return NetworkRequestResult::Deferred;
  }
  if (https && (dram.freeBytes < 11264 || dram.largestBlock < 7168))
    return NetworkRequestResult::Deferred;

  // A reduced RX buffer is safe only when the peer negotiates MFLN. The
  // probe uses a small plain TCP client, not the fatal BearSSL constructor.
  // Without MFLN use a complete 16KB TLS record instead of guessing its size.
  bool compactRecords = https &&
      BearSSL::WiFiClientSecure::probeMaxFragmentLength(addr, port, 1024);
  if (httpsAbortRequested)
    return NetworkRequestResult::Deferred;
  if (https) {
    dram = dramHeap();
    const auto iram = iramHeap();
    if (!networkMemory::canStartTls(dram, iram, compactRecords)) {
      Serial.printf_P(PSTR("\n[NET] TLS deferred. DRAM=%u/%u IRAM=%u/%u MFLN=%u"),
                      dram.freeBytes, dram.largestBlock, iram.freeBytes,
                      iram.largestBlock, compactRecords);
      return NetworkRequestResult::Deferred;
    }
    Serial.printf_P(PSTR("\n[NET] TLS budget DRAM=%u/%u IRAM=%u/%u RX=%u"),
                    dram.freeBytes, dram.largestBlock, iram.freeBytes,
                    iram.largestBlock, compactRecords ? 1024 : 16384);
  }

  // Keep the scope alive through GET and parsing: BearSSL allocates its SSL
  // and certificate contexts lazily during connect(). Its stack explicitly
  // selects DRAM in the core. The guard restores MQTT only after this heap
  // selection and all HTTP/client objects have been destroyed.
  HeapSelect requestHeap(https && compactRecords ? UMM_HEAP_IRAM : UMM_HEAP_DRAM);
#endif

  // HTTPClient references its client. Declare clients first so HTTPClient is
  // destroyed before their storage, including on every early return.
#if defined(ESP8266)
  std::unique_ptr<BearSSL::WiFiClientSecure> clientSsl;
#else
  std::unique_ptr<WiFiClientSecure> clientSsl;
#endif
  std::unique_ptr<WiFiClient> client;
  HTTPClient http;
  bool began;

  if (https) {
#if defined(ESP8266)
    clientSsl.reset(new (std::nothrow) BearSSL::WiFiClientSecure);
#else
    clientSsl.reset(new (std::nothrow) WiFiClientSecure);
#endif
    if (!clientSsl)
      return NetworkRequestResult::Deferred;
    clientSsl->setInsecure();
#if defined(ESP8266)
    clientSsl->setBufferSizes(compactRecords ? 1024 : 16384, 512);
#endif
    clientSsl->setTimeout(5000);
    began = http.begin(*clientSsl, addr, port, path, true);
  } else {
    client.reset(new (std::nothrow) WiFiClient);
    if (!client)
      return NetworkRequestResult::Deferred;
    client->setTimeout(2000);
    began = http.begin(*client, addr, port, path, false);
  }
  if (!began)
    return NetworkRequestResult::Failed;

  // HTTP/1.0 avoids chunk framing in the raw stream passed to ArduinoJson.
  // No response body is accumulated in a heap-backed String.
  http.useHTTP10(true);
  http.setReuse(false);
  http.setTimeout(2000);
  if (token.length())
    http.addHeader(F("Authorization"), token);
  int code = http.GET();

  if (httpsAbortRequested) {
    Serial.println(F("\n[NET] Request interrupted for web/API priority"));
    return NetworkRequestResult::Deferred;
  }
  if (code == HTTP_CODE_OK) {
    doc.clear();
    DeserializationError error = filter
        ? deserializeJson(doc, http.getStream(), DeserializationOption::Filter(*filter))
        : deserializeJson(doc, http.getStream());
    if (httpsAbortRequested)
      return NetworkRequestResult::Deferred;
    if (!error) {
      Serial.println(F("\n[NET] JSON response OK"));
      return NetworkRequestResult::Success;
    }
    Serial.printf_P(PSTR("\n[NET] JSON parse error: %s"), error.c_str());
  } else if (code > 0) {
    Serial.printf_P(PSTR("\n[NET] HTTP status: %d"), code);
    int size = http.getSize();
    if (code >= 400 && size > 0 && size <= 512) {
      doc.clear();
      // Preserve error details for existing callers without retaining a body
      // String. A malformed/error response never replaces cached readings.
      deserializeJson(doc, http.getStream());
    }
  } else {
    Serial.printf_P(PSTR("\n[NET] Connection error: %s"),
                    http.errorToString(code).c_str());
  }
  return NetworkRequestResult::Failed;
}

bool httpsRequestToDoc(const String &addr, int port, const String &path,
                       const String &token, bool https, JsonDocument &doc) {
  return requestJson(addr, port, path, token, https, doc) ==
         NetworkRequestResult::Success;
}
