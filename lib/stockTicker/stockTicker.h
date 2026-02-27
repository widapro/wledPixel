#ifndef stockTicker_h
#define stockTicker_h

#pragma once
#include <Arduino.h>

// Arrow font slot constants (slots 24/25 in all fonts)
#define STOCK_ARROW_UP '\x18'   // char(24)
#define STOCK_ARROW_DOWN '\x19' // char(25)

// Max symbols per zone
#define STOCK_MAX_SYMBOLS 5

// Per-symbol cached data
struct StockQuote {
  String symbol;
  float price;     // current price (c)
  float change;    // price change (d)
  float changePct; // percentage change (dp)
  bool valid;      // whether we have data
  bool error;      // whether the last fetch failed
};

// Per-zone stock data
struct StockZoneData {
  StockQuote quotes[STOCK_MAX_SYMBOLS];
  uint8_t symbolCount;
  uint8_t nextFetchIndex; // round-robin: which symbol to fetch next
  String displayString;   // pre-built display string
};

extern StockZoneData stockZones[4];

// Parse comma-separated symbols string into stockZones[zone].quotes
void stockParseSymbols(uint8_t zone, const String &symbols);

// Fetch quotes for all symbols in a zone from Finnhub API (blocking)
void stockUpdateZone(uint8_t zone, const String &apiToken);

extern bool stockTaskInProgress;

// Fetch a single symbol asynchronously (non-blocking). Uses FreeRTOS on ESP32,
// and Ticker-assisted blocking fallback on ESP8266.
void stockUpdateOneSymbolAsync(uint8_t zone, const String &apiToken);

// Fetch a single symbol (blocking per call), advances nextFetchIndex
// Returns true if this was the last symbol in the round
bool stockUpdateOneSymbol(uint8_t zone, const String &apiToken);

// Build the display string for a zone
String stockGetDisplayString(uint8_t zone, bool showArrows,
                             const String &displayFormat, const String &prefix,
                             const String &postfix);

#endif
