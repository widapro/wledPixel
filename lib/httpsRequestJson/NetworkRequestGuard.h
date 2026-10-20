#pragma once

#include "../../src/WebPriority.h"

extern bool isFetchingHttps;
extern volatile bool httpsAbortRequested;

struct HttpsGuard {
  explicit HttpsGuard(bool tls) : ready(beginExternalNetworkRequest(tls)) {
    // Releasing MQTT may yield to a browser. Do not start TLS after that
    // browser acquired priority, and never clear an outer request's abort
    // flag when a reentrant caller was refused ownership.
    if (ready && webPriorityIsActive(millis())) {
      endExternalNetworkRequest();
      ready = false;
    }
    if (ready) {
      httpsAbortRequested = false;
      isFetchingHttps = true;
    }
  }
  ~HttpsGuard() {
    if (ready) {
      isFetchingHttps = false;
      endExternalNetworkRequest();
    }
  }
  bool ready;
};
