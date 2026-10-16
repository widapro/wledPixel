#include "sharedScratch.h"

namespace {
StaticJsonDocument<2048> jsonDocument;
char jsonBuffer[1200];
} // namespace

StaticJsonDocument<2048> &sharedJsonDocument() { return jsonDocument; }

char *sharedJsonBuffer() { return jsonBuffer; }

size_t sharedJsonBufferCapacity() { return sizeof(jsonBuffer); }
