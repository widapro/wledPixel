#include "WebPriority.h"
#include "Globals.h"

namespace {
volatile uint8_t activeWebRequests = 0;
volatile uint8_t activeApiRequests = 0;
#if defined(ESP8266)
constexpr unsigned long kWebPriorityCooldownMs = 1500UL;
constexpr uint16_t kMqttNormalBufferSize = 1024;
constexpr uint16_t kMqttWebPriorityBufferSize = 128;

bool webPriorityMode = false;
bool externalNetworkActive = false;
bool externalMqttReleased = false;
bool externalMqttWasConnected = false;
uint16_t externalMqttBufferSize = kMqttNormalBufferSize;
unsigned long webPriorityStartedMillis = 0;


void enterWebPriorityMode(unsigned long now) {
  if (webPriorityMode)
    return;

  webPriorityMode = true;
  webPriorityStartedMillis = now;

  if (mqttEnable && mqttServerAddress.length() > 0) {
    if (mqttClient.connected())
      mqttClient.disconnect();
    mqttEspClient.stop();
    lastReconnectAttempt = now;
    shouldMqttDisconnect = false;

    if (mqttClient.getBufferSize() > kMqttWebPriorityBufferSize) {
      mqttClient.setBufferSize(kMqttWebPriorityBufferSize);
    }
  }

  Serial.printf_P(PSTR("\n[WEB] Priority ON. Heap=%u MaxBlock=%u Frag=%u%%"),
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                ESP.getHeapFragmentation());
}

void exitWebPriorityMode(unsigned long now) {
  bool mqttBufferReady = true;

  if (mqttEnable && mqttServerAddress.length() > 0 &&
      mqttClient.getBufferSize() < kMqttNormalBufferSize) {
    mqttBufferReady = mqttClient.setBufferSize(kMqttNormalBufferSize);
  }

  webPriorityMode = false;

  if (mqttEnable && mqttServerAddress.length() > 0) {
    lastReconnectAttempt = mqttBufferReady ? 0 : now;
    if (mqttBufferReady)
      shouldMqttPublish = true;
  }

  Serial.printf_P(PSTR("\n[WEB] Priority OFF after %lu ms. Heap=%u MaxBlock=%u"),
                now - webPriorityStartedMillis, ESP.getFreeHeap(),
                ESP.getMaxFreeBlockSize());
  if (!mqttBufferReady) {
    Serial.print(F(" [MQTT buffer restore deferred]"));
  }
}
#endif
} // namespace

bool beginExternalNetworkRequest(bool tls) {
#if defined(ESP8266)
  unsigned long now = millis();
  if (externalNetworkActive || webPriorityIsActive(now))
    return false;

  externalNetworkActive = true;
  externalMqttReleased = false;
  externalMqttWasConnected = false;

  // TLS always needs the largest contiguous block available. Plain HTTP can
  // coexist with MQTT while there is enough headroom, avoiding reconnect churn
  // for normal Home Assistant polling.
  bool releaseMqtt =
      tls || ESP.getFreeHeap() < 7000 || ESP.getMaxFreeBlockSize() < 3000;
  if (releaseMqtt && mqttEnable && mqttServerAddress.length() > 0) {
    externalMqttWasConnected = mqttClient.connected();
    externalMqttBufferSize = mqttClient.getBufferSize();
    if (externalMqttWasConnected)
      mqttClient.disconnect();
    mqttEspClient.stop();
    shouldMqttDisconnect = false;
    if (mqttClient.getBufferSize() > kMqttWebPriorityBufferSize)
      mqttClient.setBufferSize(kMqttWebPriorityBufferSize);
    externalMqttReleased = true;
    yield();
  }

  Serial.printf_P(PSTR("\n[NET] External %s request ON. Heap=%u MaxBlock=%u Frag=%u%%"
                " MQTT=%s"),
                tls ? "TLS" : "HTTP", ESP.getFreeHeap(),
                ESP.getMaxFreeBlockSize(), ESP.getHeapFragmentation(),
                externalMqttReleased ? "released" : "kept");
#else
  (void)tls;
#endif
  return true;
}

void endExternalNetworkRequest() {
#if defined(ESP8266)
  if (!externalNetworkActive)
    return;

  externalNetworkActive = false;
  unsigned long now = millis();

  // An incoming browser request may have switched on web priority while the
  // outbound request was running. In that case its exit path owns restoration.
  if (externalMqttReleased && !webPriorityIsActive(now)) {
    uint16_t restoreSize =
        externalMqttBufferSize > 0 ? externalMqttBufferSize
                                   : kMqttNormalBufferSize;
    bool restored = mqttClient.getBufferSize() >= restoreSize ||
                    mqttClient.setBufferSize(restoreSize);
    lastReconnectAttempt = restored ? 0 : now;
    if (restored && externalMqttWasConnected)
      shouldMqttPublish = true;
    if (!restored)
      Serial.print(F("\n[NET] MQTT buffer restore deferred"));
  }

  Serial.printf_P(PSTR("\n[NET] External request OFF. Heap=%u MaxBlock=%u"),
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  externalMqttReleased = false;
  externalMqttWasConnected = false;
#endif
}

void noteWebActivity() {
  unsigned long now = millis();
  webActivityMillis = now;
  // Async callbacks can run inside a yielding MQTT socket write. They only
  // claim priority here: disconnecting or resizing MQTT at that point would
  // reenter the same client. serviceWebPriorityMode() releases its resources
  // from loop(), after the interrupted foreground operation has returned.
}

bool beginTrackedWebRequest(bool apiRequest) {
  noteWebActivity();
  // Large API responses share a single fixed workspace. Serialize them so a
  // second browser cannot overwrite a profile while it is being streamed.
  if (apiRequest && activeApiRequests > 0)
    return false;
  if (activeWebRequests < 255)
    activeWebRequests++;
  if (apiRequest && activeApiRequests < 255)
    activeApiRequests++;
  return true;
}

void endTrackedWebRequest(bool apiRequest) {
  if (activeWebRequests > 0)
    activeWebRequests--;
  if (apiRequest && activeApiRequests > 0)
    activeApiRequests--;
  webActivityMillis = millis();
}

bool webPriorityIsActive(unsigned long now) {
#if defined(ESP8266)
  return activeWebRequests > 0 ||
         (webActivityMillis > 0 &&
          (now - webActivityMillis) < kWebPriorityCooldownMs);
#else
  (void)now;
  return false;
#endif
}

bool webPriorityHasActiveRequests() {
  return activeWebRequests > 0;
}

uint8_t webPriorityActiveRequestCount() {
  return activeWebRequests;
}

uint8_t webPriorityActiveApiCount() {
  return activeApiRequests;
}

unsigned long webPriorityLastActivity() { return webActivityMillis; }

bool serviceWebPriorityMode(unsigned long now) {
#if defined(ESP8266)
  if (webPriorityIsActive(now)) {
    enterWebPriorityMode(now);
    return true;
  }

  if (webPriorityMode)
    exitWebPriorityMode(now);
#else
  (void)now;
#endif

  return false;
}
