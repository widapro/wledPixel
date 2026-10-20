#include "sharedScratch.h"
#if defined(ESP8266)
#include <memory>
#include <umm_malloc/umm_heap_select.h>
#endif

namespace {
StaticJsonDocument<2048> jsonDocument;
constexpr size_t jsonBufferSize = 1200;
#if defined(ESP8266)
std::unique_ptr<char[]> jsonBuffer;
#else
char jsonBuffer[jsonBufferSize];
#endif
} // namespace

StaticJsonDocument<2048> &sharedJsonDocument() { return jsonDocument; }

char *sharedJsonBuffer() {
#if defined(ESP8266)
  // Publishing is serialized with foreground networking. Keep this persistent
  // byte buffer out of DRAM, where BearSSL must allocate its 6200-byte stack.
  if (!jsonBuffer) {
    HeapSelectIram heap;
    jsonBuffer.reset(new (std::nothrow) char[jsonBufferSize]);
  }
  return jsonBuffer.get();
#else
  return jsonBuffer;
#endif
}

size_t sharedJsonBufferCapacity() { return jsonBufferSize; }
