#include "stockTicker.h"
#include "httpsRequestJson.h"

StockZoneData stockZones[4];

// ─── Parse Symbols ──────────────────────────────────────────────────────────
void stockParseSymbols(uint8_t zone, const String &symbols) {
  if (zone >= 4)
    return;

  StockZoneData &zd = stockZones[zone];
  zd.symbolCount = 0;
  zd.nextFetchIndex = 0;

  // Clear old data
  for (uint8_t i = 0; i < STOCK_MAX_SYMBOLS; i++) {
    zd.quotes[i].symbol = "";
    zd.quotes[i].price = 0;
    zd.quotes[i].change = 0;
    zd.quotes[i].changePct = 0;
    zd.quotes[i].valid = false;
    zd.quotes[i].error = false;
  }

  if (symbols.length() == 0)
    return;

  // Parse comma-separated symbols
  String remaining = symbols;
  remaining.trim();
  remaining.toUpperCase();

  while (remaining.length() > 0 && zd.symbolCount < STOCK_MAX_SYMBOLS) {
    int comma = remaining.indexOf(',');
    String sym;
    if (comma >= 0) {
      sym = remaining.substring(0, comma);
      remaining = remaining.substring(comma + 1);
    } else {
      sym = remaining;
      remaining = "";
    }
    sym.trim();
    if (sym.length() > 0) {
      zd.quotes[zd.symbolCount].symbol = sym;
      zd.symbolCount++;
    }
  }
}

// ─── Fetch Quotes ───────────────────────────────────────────────────────────
void stockUpdateZone(uint8_t zone, const String &apiToken) {
  if (zone >= 4)
    return;
  StockZoneData &zd = stockZones[zone];

  if (apiToken.length() == 0 || zd.symbolCount == 0)
    return;

  for (uint8_t i = 0; i < zd.symbolCount; i++) {
    String path =
        "/api/v1/quote?symbol=" + zd.quotes[i].symbol + "&token=" + apiToken;

    Serial.printf("\nStock: fetching %s ...", zd.quotes[i].symbol.c_str());

    DynamicJsonDocument doc(512);
    bool success = httpsRequestToDoc("finnhub.io", 443, path, "", true, doc);
    JsonObject obj = doc.as<JsonObject>();

    if (success && obj.containsKey("c") && obj["c"].as<float>() > 0) {
      zd.quotes[i].price = obj["c"].as<float>();
      zd.quotes[i].change = obj["d"].as<float>();
      zd.quotes[i].changePct = obj["dp"].as<float>();
      zd.quotes[i].valid = true;
      zd.quotes[i].error = false;
      Serial.printf(" $%.2f (%.2f)", zd.quotes[i].price, zd.quotes[i].change);
    } else {
      Serial.print(" ERROR");
      zd.quotes[i].error = true;
    }
  }
}

// ─── Fetch ONE Symbol (non-blocking per call) ───────────────────────────────
bool stockUpdateOneSymbol(uint8_t zone, const String &apiToken) {
  if (zone >= 4)
    return true;
  StockZoneData &zd = stockZones[zone];

  if (apiToken.length() == 0 || zd.symbolCount == 0)
    return true;

  uint8_t i = zd.nextFetchIndex;
  if (i >= zd.symbolCount)
    i = 0; // safety wrap

#if defined(ESP8266)
  // Pre-check TLS memory BEFORE allocating Strings to avert std::bad_alloc on
  // highly fragmented heaps
  if (ESP.getFreeHeap() < 20480 || ESP.getMaxFreeBlockSize() < 10240) {
    Serial.printf(
        "\n[Stock] Deferring %s due to low memory (Heap: %u, MaxBlock: %u)",
        zd.quotes[i].symbol.c_str(), ESP.getFreeHeap(),
        ESP.getMaxFreeBlockSize());
    // Return false to NOT advance nextFetchIndex, so we retry the exact same
    // symbol on the next tick
    return false;
  }
#endif

  String path =
      "/api/v1/quote?symbol=" + zd.quotes[i].symbol + "&token=" + apiToken;

  Serial.printf("\nStock: fetching %s [%d/%d]...", zd.quotes[i].symbol.c_str(),
                i + 1, zd.symbolCount);

  // Convert to StaticJsonDocument to eliminate heap fragmentation
  static StaticJsonDocument<256> doc;
  doc.clear();
  bool success = httpsRequestToDoc("finnhub.io", 443, path, "", true, doc);
  JsonObject obj = doc.as<JsonObject>();

  if (success && obj.containsKey("c") && obj["c"].as<float>() > 0) {
    zd.quotes[i].price = obj["c"].as<float>();
    zd.quotes[i].change = obj["d"].as<float>();
    zd.quotes[i].changePct = obj["dp"].as<float>();
    zd.quotes[i].valid = true;
    zd.quotes[i].error = false;
    Serial.printf(" %.2f (%.2f)", zd.quotes[i].price, zd.quotes[i].change);
  } else {
    Serial.print(" ERROR");
    zd.quotes[i].error = true;
  }

  // Advance to next symbol, wrap around
  zd.nextFetchIndex = (i + 1) % zd.symbolCount;

  // Return true if we've completed a full round (next is back to 0)
  return (zd.nextFetchIndex == 0);
}

// ─── Async Fetching ─────────────────────────────────────────────────────────
bool stockTaskInProgress = false;

#if defined(ESP32)
// FreeRTOS Task for ESP32
struct StockTaskArgs {
  uint8_t zone;
  String apiToken;
};

void stockFetchTask(void *pvParameters) {
  StockTaskArgs *args = (StockTaskArgs *)pvParameters;
  stockUpdateOneSymbol(args->zone, args->apiToken);
  delete args;
  stockTaskInProgress = false;
  vTaskDelete(NULL);
}
#endif

void stockUpdateOneSymbolAsync(uint8_t zone, const String &apiToken) {
  if (stockTaskInProgress)
    return;
  stockTaskInProgress = true;

#if defined(ESP32)
  StockTaskArgs *args = new StockTaskArgs();
  args->zone = zone;
  args->apiToken = apiToken;
  // Create task on Core 0 (Pro CPU) to not block the main loop on Core 1 (App
  // CPU)
  xTaskCreatePinnedToCore(
      stockFetchTask,   // Task function
      "StockFetchTask", // String with name of task
      6144,         // Stack size in words (WiFiClientSecure needs around 5-6K)
      (void *)args, // Parameter passed as input
      1,            // Priority
      NULL,         // Task handle
      0             // Pin to Core 0
  );
#else
  // For ESP8266, we use the Ticker fix inside httpsRequestJson,
  // so we just call the blocking function directly here.
  stockUpdateOneSymbol(zone, apiToken);
  stockTaskInProgress = false;
#endif
}

// ─── Helper: format float with up to 2 decimals, strip trailing zeros ───────
static String formatPrice(float value) {
  String s = String(value, 2);
  int dotIndex = s.indexOf('.');
  if (dotIndex < 0)
    dotIndex = s.indexOf(',');

  if (dotIndex >= 0) {
    while (s.length() > 0 && s[s.length() - 1] == '0') {
      s.remove(s.length() - 1);
    }
    if (s.length() > 0 &&
        (s[s.length() - 1] == '.' || s[s.length() - 1] == ',')) {
      s.remove(s.length() - 1);
    }
  }
  if (s == "-0")
    return "0";
  return s;
}

// ─── Build Display String ───────────────────────────────────────────────────
String stockGetDisplayString(uint8_t zone, bool showArrows,
                             const String &displayFormat, const String &prefix,
                             const String &postfix) {
  if (zone >= 4)
    return "err";
  StockZoneData &zd = stockZones[zone];

  if (zd.symbolCount == 0)
    return "No symbols";

  String result = "";
  bool anyValid = false;

  for (uint8_t i = 0; i < zd.symbolCount; i++) {
    if (!zd.quotes[i].valid && !zd.quotes[i].error)
      continue;
    anyValid = true;

    if (result.length() > 0)
      result += "  "; // separator between symbols

    if (zd.quotes[i].error) {
      result += "Err";
      continue;
    }

    if (displayFormat == "priceOnly") {
      // Price only: 182.63
      result += formatPrice(zd.quotes[i].price);
    } else if (displayFormat == "changePct") {
      // Symbol + percent change: AAPL +2.34%
      result += zd.quotes[i].symbol;
      result += " ";
      if (zd.quotes[i].changePct >= 0)
        result += "+";
      result += formatPrice(zd.quotes[i].changePct);
      result += "%";
    } else if (displayFormat == "pctOnly") {
      // Percent change only: +2.34%
      if (zd.quotes[i].changePct >= 0)
        result += "+";
      result += formatPrice(zd.quotes[i].changePct);
      result += "%";
    } else {
      // namePrice (default): AAPL 182.63
      result += zd.quotes[i].symbol;
      result += " ";
      result += formatPrice(zd.quotes[i].price);
    }

    // Add arrow if enabled
    if (showArrows) {
      if (zd.quotes[i].change > 0) {
        result += STOCK_ARROW_UP;
      } else if (zd.quotes[i].change < 0) {
        result += STOCK_ARROW_DOWN;
      }
      // If change == 0, no arrow
    }
  }

  if (!anyValid)
    return "Loading...";

  // Apply prefix and postfix
  if (prefix.length() > 0)
    result = prefix + result;
  if (postfix.length() > 0)
    result = result + postfix;

  return result;
}
