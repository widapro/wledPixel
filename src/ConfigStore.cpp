#include "ConfigStore.h"

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
    preferences.putUChar("zoneNumbers", zoneNumbers);
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
    preferences.putString((String("zone") + n + "WorkMode").c_str(),
                          zones[n].workMode);
    preferences.putUChar((String("zone") + n + "ScrolSpeed").c_str(),
                         zones[n].scrollSpeed);
    preferences.putUChar((String("zone") + n + "ScrolPause").c_str(),
                         zones[n].scrollPause);
    preferences.putString((String("zone") + n + "ScrolAlign").c_str(),
                          zones[n].scrollAlign);
    preferences.putString((String("zone") + n + "ScrolEfIn").c_str(),
                          zones[n].scrollEffectIn);
    preferences.putString((String("zone") + n + "ScrolEfOut").c_str(),
                          zones[n].scrollEffectOut);
    preferences.putString((String("zone") + n + "Font").c_str(), zones[n].font);
    preferences.putUChar((String("zone") + n + "Charspac").c_str(),
                         zones[n].charspacing);
    preferences.putString((String("zone") + n + "mqttTexTop").c_str(),
                          MQTTZones[n].message);
    preferences.putString((String("zone") + n + "mqttPosfix").c_str(),
                          zones[n].mqttPostfix);
    preferences.putString((String("zone") + n + "ClDispForm").c_str(),
                          zones[n].clockDisplayFormat);
    preferences.putString((String("zone") + n + "OwmWhDisp").c_str(),
                          zones[n].owmWhatToDisplay);
    preferences.putString((String("zone") + n + "HaSensorId").c_str(),
                          zones[n].haSensorId);
    preferences.putString((String("zone") + n + "HaSensorPf").c_str(),
                          zones[n].haSensorPostfix);
    preferences.putString((String("zone") + n + "Ds18b20Pf").c_str(),
                          zones[n].ds18b20Postfix);
    preferences.putUShort((String("zone") + n + "WoprUpdInt").c_str(),
                          woprZones[n].updateInterval);
    preferences.putBool((String("zone") + n + "ScrolInf").c_str(),
                        zones[n].scrollInfinite);
    // Countdown settings
    preferences.putString((String("zone") + n + "CdFormat").c_str(),
                          zones[n].countdownFormat);
    preferences.putString((String("zone") + n + "CdFinish").c_str(),
                          zones[n].countdownFinish);
    preferences.putBool((String("zone") + n + "CdUnits").c_str(),
                        zones[n].countdownShowUnits);
    preferences.putString((String("zone") + n + "CdPrefix").c_str(),
                          zones[n].countdownPrefix);
    preferences.putString((String("zone") + n + "CdSuffix").c_str(),
                          zones[n].countdownSuffix);
    preferences.putString((String("zone") + n + "CdTarget").c_str(),
                          countdownState[n].targetStr);
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
    zoneNumbers = preferences.getUChar("zoneNumbers", zoneNumbers);
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
    zones[n].workMode = preferences.getString(
        (String("zone") + n + "WorkMode").c_str(), zones[n].workMode);
    zones[n].scrollSpeed = preferences.getUChar(
        (String("zone") + n + "ScrolSpeed").c_str(), zones[n].scrollSpeed);
    zones[n].scrollPause = preferences.getUChar(
        (String("zone") + n + "ScrolPause").c_str(), zones[n].scrollPause);
    zones[n].scrollAlign = preferences.getString(
        (String("zone") + n + "ScrolAlign").c_str(), zones[n].scrollAlign);
    zones[n].scrollEffectIn = preferences.getString(
        (String("zone") + n + "ScrolEfIn").c_str(), zones[n].scrollEffectIn);
    zones[n].scrollEffectOut = preferences.getString(
        (String("zone") + n + "ScrolEfOut").c_str(), zones[n].scrollEffectOut);
    zones[n].font = preferences.getString((String("zone") + n + "Font").c_str(),
                                          zones[n].font);
    zones[n].charspacing = preferences.getUChar(
        (String("zone") + n + "Charspac").c_str(), zones[n].charspacing);
    MQTTZones[n].message = preferences.getString(
        (String("zone") + n + "mqttTexTop").c_str(), MQTTZones[n].message);
    zones[n].mqttPostfix = preferences.getString(
        (String("zone") + n + "mqttPosfix").c_str(), zones[n].mqttPostfix);
    zones[n].clockDisplayFormat =
        preferences.getString((String("zone") + n + "ClDispForm").c_str(),
                              zones[n].clockDisplayFormat);
    zones[n].owmWhatToDisplay = preferences.getString(
        (String("zone") + n + "OwmWhDisp").c_str(), zones[n].owmWhatToDisplay);
    zones[n].scrollInfinite = preferences.getBool(
        (String("zone") + n + "ScrolInf").c_str(), zones[n].scrollInfinite);
    zones[n].haSensorId = preferences.getString(
        (String("zone") + n + "HaSensorId").c_str(), zones[n].haSensorId);
    zones[n].haSensorPostfix = preferences.getString(
        (String("zone") + n + "HaSensorPf").c_str(), zones[n].haSensorPostfix);
    zones[n].ds18b20Postfix = preferences.getString(
        (String("zone") + n + "Ds18b20Pf").c_str(), zones[n].ds18b20Postfix);
    woprZones[n].updateInterval =
        preferences.getUShort((String("zone") + n + "WoprUpdInt").c_str(),
                              woprZones[n].updateInterval);
    // Countdown settings
    zones[n].countdownFormat = preferences.getString(
        (String("zone") + n + "CdFormat").c_str(), zones[n].countdownFormat);
    zones[n].countdownFinish = preferences.getString(
        (String("zone") + n + "CdFinish").c_str(), zones[n].countdownFinish);
    zones[n].countdownShowUnits = preferences.getBool(
        (String("zone") + n + "CdUnits").c_str(), zones[n].countdownShowUnits);
    zones[n].countdownPrefix = preferences.getString(
        (String("zone") + n + "CdPrefix").c_str(), zones[n].countdownPrefix);
    zones[n].countdownSuffix = preferences.getString(
        (String("zone") + n + "CdSuffix").c_str(), zones[n].countdownSuffix);
    String savedTarget =
        preferences.getString((String("zone") + n + "CdTarget").c_str(), "");
    if (savedTarget.length() > 0) {
      parseCountdownTarget(savedTarget, n, timeClient.getEpochTime(),
                           ntpTimeZone);
    }
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

  preferences.end();
}

// ─── Read All Config
// ───────────────────────────────────────────────────────────
void readAllConfig() {
  readConfig("systemSettings", 99);
  readConfig("displaySettings", 99);
  for (uint8_t n = 0; n < zoneNumbers; n++) {
    readConfig("zoneSettings", n);
  }
  readConfig("mqttSettings", 99);
  readConfig("wallClockSett", 99);
  readConfig("owmSettings", 99);
  readConfig("haSettings", 99);
  readConfig("ds18b20Settings", 99);
  readConfig("intensity", 99);
  readConfig("intensity", 99);
}
