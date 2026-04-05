#include "ConfigStore.h"
#include <cstdio>

namespace {
constexpr uint8_t kZoneCapacity = 4;

void buildZoneKey(char *out, size_t outSize, uint8_t zone, const char *suffix) {
  snprintf(out, outSize, "zone%u%s", zone, suffix);
}

uint8_t normalizedZoneCount(uint8_t raw) {
  if (raw < 1)
    return 1;
  if (raw > kZoneCapacity)
    return kZoneCapacity;
  return raw;
}
} // namespace

// ─── Template Processor
// ────────────────────────────────────────────────────────
String processor(const String &var) {
  char buffer[10];
  if (var == F("workModeZone0"))
    return zones[0].workMode;
  if (var == F("workModeZone1"))
    return zones[1].workMode;
  if (var == F("workModeZone2"))
    return zones[2].workMode;
  if (var == F("workModeZone3"))
    return zones[3].workMode;
  if (var == F("zoneNumbers"))
    return itoa(zoneNumbers, buffer, 10);
  if (var == F("zone0Begin"))
    return itoa(zones[0].begin, buffer, 10);
  if (var == F("zone0End"))
    return itoa(zones[0].end, buffer, 10);
  if (var == F("zone1Begin"))
    return itoa(zones[1].begin, buffer, 10);
  if (var == F("zone1End"))
    return itoa(zones[1].end, buffer, 10);
  if (var == F("zone2Begin"))
    return itoa(zones[2].begin, buffer, 10);
  if (var == F("zone2End"))
    return itoa(zones[2].end, buffer, 10);
  if (var == F("zone3Begin"))
    return itoa(zones[3].begin, buffer, 10);
  if (var == F("zone3End"))
    return itoa(zones[3].end, buffer, 10);
  if (var == F("wifiSsid"))
    return WiFi.SSID();
  if (var == F("wifiIp"))
    return WiFi.localIP().toString();
  if (var == F("wifiGateway"))
    return WiFi.gatewayIP().toString();
  if (var == F("intensity"))
    return itoa(intensity + 1, buffer, 10);
  if (var == F("scrollSpeedZone0"))
    return itoa(zones[0].scrollSpeed, buffer, 10);
  if (var == F("scrollSpeedZone1"))
    return itoa(zones[1].scrollSpeed, buffer, 10);
  if (var == F("scrollSpeedZone2"))
    return itoa(zones[2].scrollSpeed, buffer, 10);
  if (var == F("scrollSpeedZone3"))
    return itoa(zones[3].scrollSpeed, buffer, 10);
  if (var == F("scrollPauseZone0"))
    return itoa(zones[0].scrollPause, buffer, 10);
  if (var == F("scrollPauseZone1"))
    return itoa(zones[1].scrollPause, buffer, 10);
  if (var == F("scrollPauseZone2"))
    return itoa(zones[2].scrollPause, buffer, 10);
  if (var == F("scrollPauseZone3"))
    return itoa(zones[3].scrollPause, buffer, 10);
  if (var == F("scrollAlignZone0"))
    return zones[0].scrollAlign;
  if (var == F("scrollAlignZone1"))
    return zones[1].scrollAlign;
  if (var == F("scrollAlignZone2"))
    return zones[2].scrollAlign;
  if (var == F("scrollAlignZone3"))
    return zones[3].scrollAlign;
  if (var == F("scrollEffectZone0In"))
    return zones[0].scrollEffectIn;
  if (var == F("scrollEffectZone1In"))
    return zones[1].scrollEffectIn;
  if (var == F("scrollEffectZone2In"))
    return zones[2].scrollEffectIn;
  if (var == F("scrollEffectZone3In"))
    return zones[3].scrollEffectIn;
  if (var == F("scrollEffectZone0Out"))
    return zones[0].scrollEffectOut;
  if (var == F("scrollEffectZone1Out"))
    return zones[1].scrollEffectOut;
  if (var == F("scrollEffectZone2Out"))
    return zones[2].scrollEffectOut;
  if (var == F("scrollEffectZone3Out"))
    return zones[3].scrollEffectOut;
  if (var == F("mqttEnable")) {
    if (mqttEnable)
      return "true";
    if (!mqttEnable)
      return "false";
  }
  if (var == F("mqttServerAddress"))
    return mqttServerAddress;
  if (var == F("mqttServerPort"))
    return itoa(mqttServerPort, buffer, 10);
  if (var == F("mqttUsername"))
    return mqttUsername;
  if (var == F("mqttPassword"))
    return mqttPassword;
  if (var == F("mqttDevicePrefix"))
    return MQTTGlobalPrefix;
  if (var == F("mqttTextTopicZone0"))
    return MQTTZones[0].message;
  if (var == F("mqttTextTopicZone1"))
    return MQTTZones[1].message;
  if (var == F("mqttTextTopicZone2"))
    return MQTTZones[2].message;
  if (var == F("mqttTextTopicZone3"))
    return MQTTZones[3].message;
  if (var == F("mqttPostfixZone0"))
    return zones[0].mqttPostfix;
  if (var == F("mqttPostfixZone1"))
    return zones[1].mqttPostfix;
  if (var == F("mqttPostfixZone2"))
    return zones[2].mqttPostfix;
  if (var == F("mqttPostfixZone3"))
    return zones[3].mqttPostfix;
  if (var == F("ntpTimeZone"))
    return String(ntpTimeZone);
  if (var == F("ntpUpdateInterval"))
    return itoa(ntpUpdateInterval, buffer, 10);
  if (var == F("ntpServer"))
    return ntpServer;
  if (var == F("clockDisplayFormatZone0"))
    return zones[0].clockDisplayFormat;
  if (var == F("clockDisplayFormatZone1"))
    return zones[1].clockDisplayFormat;
  if (var == F("clockDisplayFormatZone2"))
    return zones[2].clockDisplayFormat;
  if (var == F("clockDisplayFormatZone3"))
    return zones[3].clockDisplayFormat;
  if (var == F("owmApiToken"))
    return owmApiToken;
  if (var == F("owmUnitsFormat"))
    return owmUnitsFormat;
  if (var == F("owmUpdateInterval"))
    return itoa(owmUpdateInterval, buffer, 10);
  if (var == F("owmWhatToDisplayZone0"))
    return zones[0].owmWhatToDisplay;
  if (var == F("owmWhatToDisplayZone1"))
    return zones[1].owmWhatToDisplay;
  if (var == F("owmWhatToDisplayZone2"))
    return zones[2].owmWhatToDisplay;
  if (var == F("owmWhatToDisplayZone3"))
    return zones[3].owmWhatToDisplay;
  if (var == F("owmCity"))
    return owmCity;
  if (var == F("fontZone0"))
    return zones[0].font;
  if (var == F("fontZone1"))
    return zones[1].font;
  if (var == F("fontZone2"))
    return zones[2].font;
  if (var == F("fontZone3"))
    return zones[3].font;
  if (var == F("haAddr"))
    return haAddr;
  if (var == F("haApiToken"))
    return haApiToken;
  if (var == F("haApiHttpType"))
    return haApiHttpType;
  if (var == F("haApiPort"))
    return itoa(haApiPort, buffer, 10);
  if (var == F("haSensorIdZone0"))
    return zones[0].haSensorId;
  if (var == F("haSensorIdZone1"))
    return zones[1].haSensorId;
  if (var == F("haSensorIdZone2"))
    return zones[2].haSensorId;
  if (var == F("haSensorIdZone3"))
    return zones[3].haSensorId;
  if (var == F("haSensorPostfixZone0"))
    return zones[0].haSensorPostfix;
  if (var == F("haSensorPostfixZone1"))
    return zones[1].haSensorPostfix;
  if (var == F("haSensorPostfixZone2"))
    return zones[2].haSensorPostfix;
  if (var == F("haSensorPostfixZone3"))
    return zones[3].haSensorPostfix;
  if (var == F("haUpdateInterval"))
    return itoa(haUpdateInterval, buffer, 10);
  if (var == F("charspacingZone0"))
    return itoa(zones[0].charspacing, buffer, 10);
  if (var == F("charspacingZone1"))
    return itoa(zones[1].charspacing, buffer, 10);
  if (var == F("charspacingZone2"))
    return itoa(zones[2].charspacing, buffer, 10);
  if (var == F("charspacingZone3"))
    return itoa(zones[3].charspacing, buffer, 10);
  if (var == F("firmwareVer"))
    return firmwareVer;
  if (var == F("deviceName"))
    return shortMACaddr;
  if (var == F("disableServiceMessages"))
    return String(disableServiceMessages);
  if (var == F("disableDotsBlink"))
    return String(disableDotsBlink);
  if (var == F("ds18b20Enable"))
    return String(ds18b20Enable);
  if (var == F("ds18b20Temp"))
    return (dsTemp == "-127") ? "Not connected" : dsTemp;
  if (var == F("ds18b20UpdateInterval"))
    return itoa(ds18b20UpdateInterval, buffer, 10);
  if (var == F("ds18b20UnitsFormat"))
    return ds18b20UnitsFormat;
  if (var == F("ds18b20PostfixZone0"))
    return zones[0].ds18b20Postfix;
  if (var == F("ds18b20PostfixZone1"))
    return zones[1].ds18b20Postfix;
  if (var == F("ds18b20PostfixZone2"))
    return zones[2].ds18b20Postfix;
  if (var == F("ds18b20PostfixZone3"))
    return zones[3].ds18b20Postfix;
  return String();
}

// ─── Save Config
// ───────────────────────────────────────────────────────────────
void saveVarsToConfFile(String groupName, uint8_t n) {
  preferences.begin(groupName.c_str(), false);

  if (groupName == "systemSettings") {
    preferences.putString("deviceName", shortMACaddr);
    preferences.putBool("disablServMess", disableServiceMessages);
  }

  if (groupName == "displaySettings") {
    preferences.putUChar("zoneNumbers", normalizedZoneCount(zoneNumbers));
    preferences.putUChar("zones0Begin", zones[0].begin);
    preferences.putUChar("zones0End", zones[0].end);
    preferences.putUChar("zones1Begin", zones[1].begin);
    preferences.putUChar("zones1End", zones[1].end);
    preferences.putUChar("zones2Begin", zones[2].begin);
    preferences.putUChar("zones2End", zones[2].end);
    preferences.putUChar("zones3Begin", zones[3].begin);
    preferences.putUChar("zones3End", zones[3].end);
  }

  if (groupName == "zoneSettings") {
    if (n >= kZoneCapacity) {
      preferences.end();
      return;
    }

    char key[24];
    buildZoneKey(key, sizeof(key), n, "WorkMode");
    preferences.putString(key, zones[n].workMode);
    buildZoneKey(key, sizeof(key), n, "ScrolSpeed");
    preferences.putUChar(key, zones[n].scrollSpeed);
    buildZoneKey(key, sizeof(key), n, "ScrolPause");
    preferences.putUChar(key, zones[n].scrollPause);
    buildZoneKey(key, sizeof(key), n, "ScrolAlign");
    preferences.putString(key, zones[n].scrollAlign);
    buildZoneKey(key, sizeof(key), n, "ScrolEfIn");
    preferences.putString(key, zones[n].scrollEffectIn);
    buildZoneKey(key, sizeof(key), n, "ScrolEfOut");
    preferences.putString(key, zones[n].scrollEffectOut);
    buildZoneKey(key, sizeof(key), n, "Font");
    preferences.putString(key, zones[n].font);
    buildZoneKey(key, sizeof(key), n, "Charspac");
    preferences.putUChar(key, zones[n].charspacing);
    buildZoneKey(key, sizeof(key), n, "mqttTexTop");
    preferences.putString(key, MQTTZones[n].message);
    buildZoneKey(key, sizeof(key), n, "mqttPosfix");
    preferences.putString(key, zones[n].mqttPostfix);
    buildZoneKey(key, sizeof(key), n, "ClDispForm");
    preferences.putString(key, zones[n].clockDisplayFormat);
    buildZoneKey(key, sizeof(key), n, "OwmWhDisp");
    preferences.putString(key, zones[n].owmWhatToDisplay);
    buildZoneKey(key, sizeof(key), n, "HaSensorId");
    preferences.putString(key, zones[n].haSensorId);
    buildZoneKey(key, sizeof(key), n, "HaSensorPf");
    preferences.putString(key, zones[n].haSensorPostfix);
    buildZoneKey(key, sizeof(key), n, "Ds18b20Pf");
    preferences.putString(key, zones[n].ds18b20Postfix);
    buildZoneKey(key, sizeof(key), n, "WoprUpdInt");
    preferences.putUShort(key, woprZones[n].updateInterval);
    buildZoneKey(key, sizeof(key), n, "ScrolInf");
    preferences.putBool(key, zones[n].scrollInfinite);
    // Countdown settings
    buildZoneKey(key, sizeof(key), n, "CdFormat");
    preferences.putString(key, zones[n].countdownFormat);
    buildZoneKey(key, sizeof(key), n, "CdFinish");
    preferences.putString(key, zones[n].countdownFinish);
    buildZoneKey(key, sizeof(key), n, "CdUnits");
    preferences.putBool(key, zones[n].countdownShowUnits);
    buildZoneKey(key, sizeof(key), n, "CdPrefix");
    preferences.putString(key, zones[n].countdownPrefix);
    buildZoneKey(key, sizeof(key), n, "CdSuffix");
    preferences.putString(key, zones[n].countdownSuffix);
    buildZoneKey(key, sizeof(key), n, "CdTarget");
    preferences.putString(key, countdownState[n].targetStr);
    // Stock ticker settings
    buildZoneKey(key, sizeof(key), n, "StockSym");
    preferences.putString(key, zones[n].stockSymbols);
    buildZoneKey(key, sizeof(key), n, "StockFmt");
    preferences.putString(key, zones[n].stockDisplayFormat);
    buildZoneKey(key, sizeof(key), n, "StockPre");
    preferences.putString(key, zones[n].stockPrefix);
    buildZoneKey(key, sizeof(key), n, "StockPst");
    preferences.putString(key, zones[n].stockPostfix);
    buildZoneKey(key, sizeof(key), n, "StockArr");
    preferences.putBool(key, zones[n].stockShowArrows);
  }

  if (groupName == "mqttSettings") {
    preferences.putBool("mqttEnable", mqttEnable);
    preferences.putString("mqttServerAddr", mqttServerAddress);
    preferences.putUShort("mqttServerPort", mqttServerPort);
    preferences.putString("mqttUsername", mqttUsername);
    preferences.putString("mqttPassword", mqttPassword);
  }

  if (groupName == "wallClockSett") {
    preferences.putFloat("ntpTz", ntpTimeZone);
    preferences.putBool("disableDotBlink", disableDotsBlink);
    preferences.putUShort("ntpUpdateInt", ntpUpdateInterval);
    preferences.putString("ntpServer", ntpServer);
  }

  if (groupName == "owmSettings") {
    preferences.putString("owmApiToken", owmApiToken);
    preferences.putString("owmUnitsFormat", owmUnitsFormat);
    preferences.putUShort("owmUpdateInt", owmUpdateInterval);
    preferences.putString("owmCity", owmCity);
  }

  if (groupName == "haSettings") {
    preferences.putString("haAddr", haAddr);
    preferences.putString("haApiToken", haApiToken);
    preferences.putString("haApiHttpType", haApiHttpType);
    preferences.putUShort("haApiPort", haApiPort);
    preferences.putUShort("haUpdateInt", haUpdateInterval);
  }

  if (groupName == "ds18b20Settings") {
    preferences.putBool("ds18b20Enable", ds18b20Enable);
    preferences.putUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    preferences.putString("ds18b20UnitFr", ds18b20UnitsFormat);
  }

  if (groupName == "stockSettings") {
    preferences.putString("stApiToken", stockApiToken);
    preferences.putUShort("stUpdateInt", stockUpdateInterval);
  }

  if (groupName == "pbSettings") {
    for (uint8_t i = 0; i < kZoneCapacity; i++) {
      char key[24];
      snprintf(key, sizeof(key), "pb%uEnable", i);
      preferences.putBool(key, progressBars[i].enabled);
      snprintf(key, sizeof(key), "pb%uSrcType", i);
      preferences.putString(key, progressBars[i].dataSourceType);
      snprintf(key, sizeof(key), "pb%uSrcId", i);
      preferences.putString(key, progressBars[i].dataSourceId);
      snprintf(key, sizeof(key), "pb%uMin", i);
      preferences.putFloat(key, progressBars[i].minValue);
      snprintf(key, sizeof(key), "pb%uMax", i);
      preferences.putFloat(key, progressBars[i].maxValue);
      snprintf(key, sizeof(key), "pb%uCondEn", i);
      preferences.putBool(key, progressBars[i].conditionEnabled);
      snprintf(key, sizeof(key), "pb%uCdSrcT", i);
      preferences.putString(key, progressBars[i].conditionSourceType);
      snprintf(key, sizeof(key), "pb%uCdSrcI", i);
      preferences.putString(key, progressBars[i].conditionSourceId);
      snprintf(key, sizeof(key), "pb%uCdVal", i);
      preferences.putString(key, progressBars[i].conditionValue);
    }
  }

  if (groupName == "intensity") {
    preferences.putUChar("intensity", intensity);
  }

  preferences.end();
}

// ─── Read Config
// ───────────────────────────────────────────────────────────────
void readConfig(String groupName, uint8_t n) {
  preferences.begin(groupName.c_str(), true);

  if (groupName == "systemSettings") {
    shortMACaddr = preferences.getString("deviceName", shortMACaddr);
    disableServiceMessages =
        preferences.getBool("disablServMess", disableServiceMessages);
  }

  if (groupName == "displaySettings") {
    zoneNumbers =
        normalizedZoneCount(preferences.getUChar("zoneNumbers", zoneNumbers));
    zones[0].begin = preferences.getUChar("zones0Begin", zones[0].begin);
    zones[0].end = preferences.getUChar("zones0End", zones[0].end);
    zones[1].begin = preferences.getUChar("zones1Begin", zones[1].begin);
    zones[1].end = preferences.getUChar("zones1End", zones[1].end);
    zones[2].begin = preferences.getUChar("zones2Begin", zones[2].begin);
    zones[2].end = preferences.getUChar("zones2End", zones[2].end);
    zones[3].begin = preferences.getUChar("zones3Begin", zones[3].begin);
    zones[3].end = preferences.getUChar("zones3End", zones[3].end);
  }

  if (groupName == "zoneSettings") {
    if (n >= kZoneCapacity) {
      preferences.end();
      return;
    }

    char key[24];
    buildZoneKey(key, sizeof(key), n, "WorkMode");
    zones[n].workMode = preferences.getString(key, zones[n].workMode);
    buildZoneKey(key, sizeof(key), n, "ScrolSpeed");
    zones[n].scrollSpeed = preferences.getUChar(key, zones[n].scrollSpeed);
    buildZoneKey(key, sizeof(key), n, "ScrolPause");
    zones[n].scrollPause = preferences.getUChar(key, zones[n].scrollPause);
    buildZoneKey(key, sizeof(key), n, "ScrolAlign");
    zones[n].scrollAlign = preferences.getString(key, zones[n].scrollAlign);
    buildZoneKey(key, sizeof(key), n, "ScrolEfIn");
    zones[n].scrollEffectIn =
        preferences.getString(key, zones[n].scrollEffectIn);
    buildZoneKey(key, sizeof(key), n, "ScrolEfOut");
    zones[n].scrollEffectOut =
        preferences.getString(key, zones[n].scrollEffectOut);
    buildZoneKey(key, sizeof(key), n, "Font");
    zones[n].font = preferences.getString(key, zones[n].font);
    buildZoneKey(key, sizeof(key), n, "Charspac");
    zones[n].charspacing = preferences.getUChar(key, zones[n].charspacing);
    buildZoneKey(key, sizeof(key), n, "mqttTexTop");
    MQTTZones[n].message = preferences.getString(key, MQTTZones[n].message);
    buildZoneKey(key, sizeof(key), n, "mqttPosfix");
    zones[n].mqttPostfix = preferences.getString(key, zones[n].mqttPostfix);
    buildZoneKey(key, sizeof(key), n, "ClDispForm");
    zones[n].clockDisplayFormat =
        preferences.getString(key, zones[n].clockDisplayFormat);
    buildZoneKey(key, sizeof(key), n, "OwmWhDisp");
    zones[n].owmWhatToDisplay =
        preferences.getString(key, zones[n].owmWhatToDisplay);
    buildZoneKey(key, sizeof(key), n, "ScrolInf");
    zones[n].scrollInfinite = preferences.getBool(key, zones[n].scrollInfinite);
    buildZoneKey(key, sizeof(key), n, "HaSensorId");
    zones[n].haSensorId = preferences.getString(key, zones[n].haSensorId);
    buildZoneKey(key, sizeof(key), n, "HaSensorPf");
    zones[n].haSensorPostfix =
        preferences.getString(key, zones[n].haSensorPostfix);
    buildZoneKey(key, sizeof(key), n, "Ds18b20Pf");
    zones[n].ds18b20Postfix =
        preferences.getString(key, zones[n].ds18b20Postfix);
    buildZoneKey(key, sizeof(key), n, "WoprUpdInt");
    woprZones[n].updateInterval =
        preferences.getUShort(key, woprZones[n].updateInterval);
    // Countdown settings
    buildZoneKey(key, sizeof(key), n, "CdFormat");
    zones[n].countdownFormat = preferences.getString(key, zones[n].countdownFormat);
    buildZoneKey(key, sizeof(key), n, "CdFinish");
    zones[n].countdownFinish = preferences.getString(key, zones[n].countdownFinish);
    buildZoneKey(key, sizeof(key), n, "CdUnits");
    zones[n].countdownShowUnits = preferences.getBool(key, zones[n].countdownShowUnits);
    buildZoneKey(key, sizeof(key), n, "CdPrefix");
    zones[n].countdownPrefix = preferences.getString(key, zones[n].countdownPrefix);
    buildZoneKey(key, sizeof(key), n, "CdSuffix");
    zones[n].countdownSuffix = preferences.getString(key, zones[n].countdownSuffix);
    buildZoneKey(key, sizeof(key), n, "CdTarget");
    String savedTarget = preferences.getString(key, "");
    if (savedTarget.length() > 0) {
      parseCountdownTarget(savedTarget, n, timeClient.getEpochTime(),
                           ntpTimeZone);
    }
    // Stock ticker settings
    buildZoneKey(key, sizeof(key), n, "StockSym");
    zones[n].stockSymbols = preferences.getString(key, zones[n].stockSymbols);
    buildZoneKey(key, sizeof(key), n, "StockFmt");
    zones[n].stockDisplayFormat =
        preferences.getString(key, zones[n].stockDisplayFormat);
    buildZoneKey(key, sizeof(key), n, "StockPre");
    zones[n].stockPrefix = preferences.getString(key, zones[n].stockPrefix);
    buildZoneKey(key, sizeof(key), n, "StockPst");
    zones[n].stockPostfix = preferences.getString(key, zones[n].stockPostfix);
    buildZoneKey(key, sizeof(key), n, "StockArr");
    zones[n].stockShowArrows =
        preferences.getBool(key, zones[n].stockShowArrows);
  }

  if (groupName == "mqttSettings") {
    mqttEnable = preferences.getBool("mqttEnable", mqttEnable);
    mqttServerAddress =
        preferences.getString("mqttServerAddr", mqttServerAddress);
    mqttServerPort = preferences.getUShort("mqttServerPort", mqttServerPort);
    mqttUsername = preferences.getString("mqttUsername", mqttUsername);
    mqttPassword = preferences.getString("mqttPassword", mqttPassword);
    Serial.print("readConfig: Loaded mqttPassword: ");
    Serial.println(mqttPassword.length() > 0 ? "EXISTS" : "EMPTY");
  }

  if (groupName == "wallClockSett") {
    ntpTimeZone = preferences.getFloat("ntpTz", ntpTimeZone);
    disableDotsBlink = preferences.getBool("disableDotBlink", disableDotsBlink);
    ntpUpdateInterval =
        preferences.getUShort("ntpUpdateInt", ntpUpdateInterval);
    ntpServer = preferences.getString("ntpServer", ntpServer);
  }

  if (groupName == "owmSettings") {
    owmApiToken = preferences.getString("owmApiToken", owmApiToken);
    owmUnitsFormat = preferences.getString("owmUnitsFormat", owmUnitsFormat);
    owmUpdateInterval =
        preferences.getUShort("owmUpdateInt", owmUpdateInterval);
    owmCity = preferences.getString("owmCity", owmCity);
  }

  if (groupName == "haSettings") {
    haAddr = preferences.getString("haAddr", haAddr);
    haApiToken = preferences.getString("haApiToken", haApiToken);
    haApiHttpType = preferences.getString("haApiHttpType", haApiHttpType);
    haApiPort = preferences.getUShort("haApiPort", haApiPort);
    haUpdateInterval = preferences.getUShort("haUpdateInt", haUpdateInterval);
  }

  if (groupName == "ds18b20Settings") {
    ds18b20Enable = preferences.getBool("ds18b20Enable", ds18b20Enable);
    ds18b20UpdateInterval =
        preferences.getUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    ds18b20UnitsFormat =
        preferences.getString("ds18b20UnitFr", ds18b20UnitsFormat);
  }

  if (groupName == "intensity") {
    intensity = preferences.getUChar("intensity", intensity);
  }

  if (groupName == "stockSettings") {
    stockApiToken = preferences.getString("stApiToken", stockApiToken);
    stockUpdateInterval =
        preferences.getUShort("stUpdateInt", stockUpdateInterval);
  }

  if (groupName == "pbSettings") {
    for (uint8_t i = 0; i < kZoneCapacity; i++) {
      char key[24];
      snprintf(key, sizeof(key), "pb%uEnable", i);
      progressBars[i].enabled = preferences.getBool(key, progressBars[i].enabled);
      snprintf(key, sizeof(key), "pb%uSrcType", i);
      progressBars[i].dataSourceType =
          preferences.getString(key, progressBars[i].dataSourceType);
      snprintf(key, sizeof(key), "pb%uSrcId", i);
      progressBars[i].dataSourceId =
          preferences.getString(key, progressBars[i].dataSourceId);
      snprintf(key, sizeof(key), "pb%uMin", i);
      progressBars[i].minValue =
          preferences.getFloat(key, progressBars[i].minValue);
      snprintf(key, sizeof(key), "pb%uMax", i);
      progressBars[i].maxValue =
          preferences.getFloat(key, progressBars[i].maxValue);
      snprintf(key, sizeof(key), "pb%uCondEn", i);
      progressBars[i].conditionEnabled =
          preferences.getBool(key, progressBars[i].conditionEnabled);
      snprintf(key, sizeof(key), "pb%uCdSrcT", i);
      progressBars[i].conditionSourceType =
          preferences.getString(key, progressBars[i].conditionSourceType);
      snprintf(key, sizeof(key), "pb%uCdSrcI", i);
      progressBars[i].conditionSourceId =
          preferences.getString(key, progressBars[i].conditionSourceId);
      snprintf(key, sizeof(key), "pb%uCdVal", i);
      progressBars[i].conditionValue =
          preferences.getString(key, progressBars[i].conditionValue);
      // Reset runtime state
      progressBars[i].currentValue = 0.0;
      progressBars[i].conditionMet = !progressBars[i].conditionEnabled;
      progressBars[i].barActive = false;
      progressBars[i].lastDataUpdate = 0;
      progressBars[i].lastCondUpdate = 0;
      progressBars[i].lastCondReceivedValue = "";
    }
  }

  preferences.end();
}

// ─── Read All Config
// ───────────────────────────────────────────────────────────
void readAllConfig() {
  readConfig("systemSettings", 99);
  readConfig("displaySettings", 99);
  for (uint8_t n = 0; n < normalizedZoneCount(zoneNumbers); n++) {
    readConfig("zoneSettings", n);
  }
  readConfig("mqttSettings", 99);
  readConfig("wallClockSett", 99);
  readConfig("owmSettings", 99);
  readConfig("haSettings", 99);
  readConfig("ds18b20Settings", 99);
  readConfig("stockSettings", 99);
  readConfig("pbSettings", 99);
  readConfig("intensity", 99);
}
