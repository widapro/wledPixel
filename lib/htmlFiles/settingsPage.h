#ifndef SETTINGS_PAGE_h
#define SETTINGS_PAGE_h

#include <Arduino.h>

const char settingsPagePart1[] PROGMEM = R"=====(<!doctype html>
<html lang="en">
  <head>
    <link rel="icon" href="data:image/svg+xml,%3C%3Fxml version='1.0' encoding='UTF-8'%3F%3E%3C!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN' 'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'%3E%3Csvg height='24' viewBox='0 0 24 24' width='24' xmlns='http://www.w3.org/2000/svg'%3E%3Cpath d='M3,9H7V5H3V9M3,14H7V10H3V14M8,14H12V10H8V14M13,14H17V10H13V14M8,9H12V5H8V9M13,5V9H17V5H13M18,14H22V10H18V14M3,19H7V15H3V19M8,19H12V15H8V19M13,19H17V15H13V19M18,19H22V15H18V19M18,5V9H22V5H18Z'/%3E%3C/svg%3E" type="image/svg+xml" />
    <!-- Required meta tags -->
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- Bootstrap CSS -->
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
    <title>Wled-pixel</title>
  </head>
  <body>
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
  <header>
    <div class="px-3 py-2 bg-dark text-white">
      <div class="container">
        <div class="d-flex flex-wrap align-items-center justify-content-center justify-content-lg-start">
          <a href="/" class="d-flex align-items-center my-2 my-lg-0 me-lg-auto text-white text-decoration-none">
            <h1 class="mb-3 fw-bold font-monospace">wledPixel</h1>
          </a>

          <ul class="nav col-12 col-lg-auto my-2 justify-content-center my-md-0 text-small">
            <li>
                <a href="/" class="nav-link text-white">
                  Home
                </a>
              </li>
              <li>
                <span class="nav-link text-secondary">
                  Settings
                </span>
              </li>
              <li>
                <a href="/backup" class="nav-link text-white">
                  Backup/Restore
                </a>
              </li>
              <li>
                <a href="/update" class="nav-link text-white">
                  Update firmware
                </a>
              </li>
          </ul>
        </div>
      </div>
    </div>
  </header>
  <div class="container">
    <main>
        <div class="py-5 text-center">
            <h2>Settings</h2>
            <p class="lead">The configuration page allows making changes in the settings on the fly. Using this page you can choose different device work modes.</p>
        </div>
        <div class="row g-5">
            <div class="col-md-5 col-lg-4 order-md-last">
              <h4 class="d-flex justify-content-between align-items-center mb-3">
                <span class="text-primary">Info</span>
              </h4>
              <ul class="list-group mb-3">
                <li class="list-group-item d-flex justify-content-between lh-sm">
                  <div>
                    <h6 class="my-0">wledPixel</h6>
                    <small class="text-muted">wledPixel dot led Display</small>
                  </div>
                  <span class="text-muted" id="firmwareVer"></span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Platform:</h6>
                    </div>
                    <span class="text-muted" id="platform"></span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi SSID:</h6>
                    </div>
                    <span class="text-muted" id="wifiSsid"></span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi IP:</h6>
                    </div>
                    <span class="text-muted" id="wifiIp"></span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi Gateway:</h6>
                    </div>
                    <span class="text-muted" id="wifiGateway"></span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi Signal:</h6>
                    </div>
                    <span class="text-muted" id="wifiRssi"></span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">MQTT device prefix:</h6>
                    </div>
                    <span class="text-muted" id="mqttDevicePrefixInfo"></span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">ds18b20 Temperature:</h6>
                    </div>
                    <span class="text-muted" id="ds18b20TempInfo"></span>
                </li>
              </ul>

              <h4 class="d-flex justify-content-between align-items-center mb-3">
                <span class="text-primary">Help</span>
              </h4>
              <div>
                    <h6 class="my-2">Font</h6>
                    <small class="text-muted"><b>Cyrillic</b> - compact neat font<br><b>Wled</b> - fundamental font<br><b>Default</b> - standard display library font</small>
              </div>
              <hr>
              <div>
                    <h6 class="my-2">Character spacing</h6>
                    <small class="text-muted">Distance between characters on the display. The default is exactly 1. It is useful to set 0 if you need to fit a lot of text on the display or you can use the Cyrillic font.</small>
              </div>
              <hr>
              <div>
                    <h6 class="my-2">Wall clock</h6>
                    <small class="text-muted">Clock mode. It has the following display options:
                        <br><b>HH:MM</b> - Hours : Minutes [21:43]
                        <br><b>HH:MM:SS</b> - Hours : Minutes : Seconds [21:43:54]
                        <br><b>HH</b> - Hours [21]
                        <br><b>MM</b> - Minutes [43]
                        <br><b>dd.mm.yyyy</b> - Day.Month.Year [21.06.2022]
                        <br><b>dd.mm</b> - Day.Month [21.06]
                        <br><b>dd.mm aa</b> - Day.Month weekday name (e.g., Sun) [21.06 Tue] <b>*</b><i>in Cyrillic font weekday name will be in Cyrillic</i>
                        <br><br><b>Advice</b><br>In <b>HH:MM</b> and <b>HH:MM:SS</b> mode, two dots flash every second. For correct operation, it is recommended to set the <b>effectIN</b> and <b>OUT</b> to the value <b>NO_EFFECT</b>.
                    </small>
              </div>
              <hr>
              <div>
                    <h6 class="my-2">Open Weather map</h6>
                    <small class="text-muted">First, configure all the settings at the bottom of the page, and then turn on the work mode in the desired zone.
                        <br><br>A free API token allows you to make requests no more than once every <b>60 seconds</b>! Otherwise, your token will be <b>blocked</b> for 24 hours.
                        <br><br>Each zone makes its own request to the API, consider this when calculating the update interval (2 zones - 120 seconds, etc.)</small>
              </div>
              <hr>
              <div>
                    <h6 class="my-2">Home Assistant client</h6>
                    <small class="text-muted">Use a <a href="https://www.home-assistant.io/docs/authentication/">Long-lived access token</a>.
                    <br><br>Only display of sensor value is supported.
                    <br><br>Sensor ID copy from your Home Assistant (e.g. sensor.lumi_weather)</small>
              </div>
              <hr>
              <div>
                <h6 class="my-2">MQTT client</h6>
                <small class="text-muted">The following topics are supported:
                    <ul>
                      <li><b>devicePrefix/zone<i>N</i>/workmode</b> - zone work mode</li>
                        <ul><li>mqttClient</li>
                            <li>manualInput</li>
                            <li>wallClock</li>
                            <li>owmWeather</li>
                            <li>haClient</li>
                            <li>intTempSensor (DS18B20)</li>
                            <li>wopr (WarGames effect)</li>
                        </ul>
                      <li><b>devicePrefix/zone<i>N</i>/text</b> - text to display (for mqttClient mode)</li>
                      <li><b>devicePrefix/zone<i>N</i>/scrolleffectIn</b> - scroll effect <b>IN</b></li>
                      <li><b>devicePrefix/zone<i>N</i>/scrolleffectOut</b> - scroll effect <b>Out</b></li><br>
                      <b>scrolleffectIn</b> and <b>scrolleffectOut</b> support next values:
                        <ul>
                          <li>PA_NO_EFFECT</li>
                          <li>PA_SCROLL_UP</li>
                          <li>PA_SCROLL_DOWN</li>
                          <li>PA_SCROLL_LEFT</li>
                          <li>PA_SCROLL_RIGHT</li>
                          <li>PA_SLICE</li>
                          <li>PA_MESH</li>
                          <li>PA_FADE</li>
                          <li>PA_DISSOLVE</li>
                          <li>PA_BLINDS</li>
                          <li>PA_RANDOM</li>
                          <li>PA_WIPE</li>
                          <li>PA_WIPE_CURSOR</li>
                          <li>PA_SCAN_HORIZ</li>
                          <li>PA_SCAN_VERT</li>
                          <li>PA_OPENING</li>
                          <li>PA_OPENING_CURSOR</li>
                          <li>PA_CLOSING</li>
                          <li>PA_CLOSING_CURSOR</li>
                          <li>PA_SCROLL_UP_LEFT</li>
                          <li>PA_SCROLL_UP_RIGHT</li>
                          <li>PA_SCROLL_DOWN_LEFT</li>
                          <li>PA_SCROLL_DOWN_RIGHT</li>
                          <li>PA_GROW_UP</li>
                          <li>PA_GROW_DOWN</li>
                        </ul><br>
                      <li><b>devicePrefix/zone<i>N</i>/scrollspeed</b> - scroll speed</li>
                      <li><b>devicePrefix/zone<i>N</i>/scrollpause</b> - scroll pause</li>
                      <li><b>devicePrefix/zone<i>N</i>/scrollalign</b> - scroll alignment</li>
                      <li><b>devicePrefix/zone<i>N</i>/charspacing</b> - character spacing</li>
                      <li><b>devicePrefix/intensity</b> - brightness</li>
                      <li><b>devicePrefix/zone<i>N</i>/scrollInfinite</b> - infinite scroll (on/off)</li>
                      <li><b>devicePrefix/zone<i>N</i>/workmode</b> - zone work mode</li>
                      <li><b>devicePrefix/power</b> - display power control, support <b>on</b> / <b>off</b> values</li>
                    </ul>
                      where <b>devicePrefix</b> = <span id="mqttDevicePrefixHelp"></span>
                      <br><b>zoneN</b> = zone number (e.g. Zone0, Zone1, Zone2, Zone3)
                  </small>
              </div>
              <hr>
              <div>
                <h6 class="my-2">HTTP API</h6>
                <small class="text-muted">
                    <b>Manual Message:</b><br>
                    POST to <b>/api/message</b> with parameters:
                    <ul>
                      <li><b>messageZone0</b> ... <b>messageZone3</b> - Text to display</li>
                    </ul>
                    Sending the same message again will restart the animation immediately.
                </small>
              </div>
      
            </div>
            <div class="col-md-7 col-lg-8">
             <form class="needs-validation" novalidate="">
              <div class="row g-3">
 
                <h3 class="mb-3">System settings</h3>
                  <div class="col-6" hidden>
                    <label for="deviceName" class="form-label">Device Name</label>
                    <div class="input-group mb-3">
                      <input type="text" class="form-control" id="deviceName">
                    </div> 
                  </div>

                  <div class="col-6 align-self-center">
                    <div class="form-check">
                      <input class="form-check-input" type="checkbox" value="disableServiceMessages" id="disableServiceMessages">
                      <label class="form-check-label" for="disableServiceMessages">
                        Disable service messages on display
                      </label>
                    </div>
                  </div>
                <div class="row">
                  <div class="col-6">
                    <button id="applySystemSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>
                  </div>
                  <div class="col-6">
                    <button type="button" id="rebootDevice" class="w-100 btn btn-danger btn-lg" onClick="apiRebootDevice();">Reboot Device</button>
                  </div>
                </div>
                <div class="row mt-3">
                  <div class="col-12">
                    <button type="button" id="resetWifi" class="w-100 btn btn-warning btn-lg" onClick="apiResetWifi();">Reset WiFi Settings</button>
                  </div>
                </div>
                <div class="row mt-3">
                  <div class="col-12">
                    <button type="button" id="factoryReset" class="w-100 btn btn-dark btn-lg" onClick="apiFactoryReset();">Factory Reset (Erase All Settings)</button>
                  </div>
                </div>

              <hr class="my-4">

              <h4 class="mb-3">Display settings <span class="text-primary fs-6" style="--bs-text-opacity: .5;">[reboot required]</span></h4>
                <div class="row g-3">
                  <div class="col-sm-4">
                    <label for="Numbers of zones" class="form-label">Number of zones</label>
                    <select class="form-select" aria-label="Default select example" id="zoneNumbers">
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                        <option value="4">4</option>
                      </select>
                    <div class="invalid-feedback">
                      Valid first name is required.
                    </div>
                  </div>
      
                  <div class="col-sm-4">
                    <label for="zone0Begin" class="form-label">First module <span class="text-muted">[ zone 0 ]</span></label>
                    <input type="text" class="form-control" id="zone0Begin" >
                  </div>
                  <div class="col-sm-4">
                    <label for="zone0End" class="form-label">Last module <span class="text-muted">[ zone 0 ]</span></label>
                    <input type="text" class="form-control" id="zone0End" >
                  </div>
                  <div class="col-sm-4"></div>
                  <div class="col-sm-4" id="zone1BeginDiv">
                    <label for="zone1Begin" class="form-label">First module <span class="text-muted">[ zone 1 ]</span></label>
                    <input type="text" class="form-control" id="zone1Begin" >
                  </div>
                  <div class="col-sm-4" id="zone1EndDiv">
                    <label for="zone1End" class="form-label">Last module <span class="text-muted">[ zone 1 ]</span></label>
                    <input type="text" class="form-control" id="zone1End" >
                  </div>
                  <div class="col-sm-4"></div>
                  <div class="col-sm-4" id="zone2BeginDiv">
                    <label for="zone2Begin" class="form-label">First module <span class="text-muted">[ zone 2 ]</span></label>
                    <input type="text" class="form-control" id="zone2Begin" >
                  </div>
                  <div class="col-sm-4" id="zone2EndDiv">
                    <label for="zone2End" class="form-label">Last module <span class="text-muted">[ zone 2 ]</span></label>
                    <input type="text" class="form-control" id="zone2End" >
                  </div>
                  <div class="col-sm-4"></div>
                  <div class="col-sm-4" id="zone3BeginDiv">
                    <label for="zone3Begin" class="form-label">First module <span class="text-muted">[ zone 3 ]</span></label>
                    <input type="text" class="form-control" id="zone3Begin" >
                  </div>
                  <div class="col-sm-4" id="zone3EndDiv">
                    <label for="zone3End" class="form-label">Last module <span class="text-muted">[ zone 3 ]</span></label>
                    <input type="text" class="form-control" id="zone3End" >
                  </div>
                
                  <button id="displaySettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply and reboot</button>
                  
                  <hr class="my-4">

                  <div class="col-11">
                    <label for="intensity" class="form-label">Brightness</label>
                    <input type="range" class="form-range" min="1" max="16" step="1" id="intensity" onChange="preparePostRequest(event, this.id, this.value);">
                  </div>
                  <div class="col-1 align-self-end">
                    <label id="intensityValue" for="intensity" class="form-label text-primary"></label>
                  </div>

                  <hr class="my-5">

                  <h3 class="mb-3">Zone 0</h3>

                  <div class="col-5">
                    <label for="workModeZone0" class="form-label">Working mode</label>
                    <select id="workModeZone0" class="form-select">
                        <option value="mqttClient">MQTT client</option>
                        <option value="manualInput">Manual input</option>
                        <option value="wallClock">Wall clock</option>
                        <option value="owmWeather">Open Weather map</option>
                        <option value="haClient">Home Assistant client</option>
                        <option value="intTempSensor">Internal Temperature sensor (ds18b20)</option>
                        <option value="wopr">WOPR Mode (WarGames)</option>
                        <option value="countdown">Countdown timer</option>
                    </select>
                  </div>
                  
                  <div class="col-7" id="emptyZone0Div"></div>
                  <div class="col-7" id="countdownTargetZone0Div" style="display: none;">
                    <label for="countdownTargetZone0" class="form-label">Target date/time</label>
                    <input type="datetime-local" class="form-control" id="countdownTargetZone0" step="1">
                  </div>
                  <div class="col-5" id="mqttZone0PrefixDiv" style="display: none;">
                    <label for="mqttZone0Prefix" class="form-label">MQTT zone text topic</label>
                        <input type="text" class="form-control" id="mqttTextTopicZone0">
                  </div>
                  <div class="col-2" id="mqttPostfixZone0Div" style="display: none;">
                    <label for="mqttPostfixZone0" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="mqttPostfixZone0">
                  </div>
                  <div class="col-7" id="clockDisplayFormatZone0Div" style="display: none;">
                    <label for="clockDisplayFormatZone0" class="form-label">Time format</label>
                        <select id="clockDisplayFormatZone0" class="form-select">
                          <option value="HHMM">HH:MM</option>
                          <option value="HHMMSS">HH:MM:SS</option>
                          <option value="HH">HH</option>
                          <option value="MM">MM</option>
                          <option value="ddmmyyyy">dd.mm.yyyy</option>
                          <option value="ddmm">dd.mm</option>
                          <option value="ddmmaa">dd.mm aa</option>
                          <option value="aa">aa</option>
                          <option value="ddmmaahhmm">ddmmaahhmm</option>
                        </select>
                  </div>
                  <div class="col-12" id="scrollInfiniteZone0Div" style="display: none;">
                    <div class="form-check form-switch pt-4">
                      <input class="form-check-input" type="checkbox" id="scrollInfiniteZone0" onChange="preparePostRequest(event, this.id, this.checked);">
                      <label class="form-check-label" for="scrollInfiniteZone0">Infinite Scroll</label>
                    </div>
                  </div>


                  <div class="col-7" id="owmWhatToDisplayZone0div" style="display: none;">
                    <label for="owmWhatToDisplayZone0" class="form-label">What to display</label>
                    <select id="owmWhatToDisplayZone0" class="form-select">
                        <option value="owmTemperature">Temperature</option>
                        <option value="owmHumidity">Humidity</option>
                        <option value="owmPressure">Pressure</option>
                        <option value="owmWindSpeed">Wind speed</option>
                        <option value="owmWeatherIcon">Wether icon</option>
                    </select>
                  </div>
                  <div class="col-5" id="haSensorIdZone0Div" style="display: none;">
                    <label for="haSensorIdZone0" class="form-label">Sensor ID</label>
                        <input type="text" class="form-control" id="haSensorIdZone0">
                  </div>
                  <div class="col-2" id="haSensorPostfixZone0Div" style="display: none;">
                    <label for="haSensorPostfixZone0" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="haSensorPostfixZone0">
                  </div>
                  <div class="col-7" id="ds18b20PostfixZone0Div" style="display: none;">
                    <label for="ds18b20PostfixZone0" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="ds18b20PostfixZone0">
                  </div>
                  <!-- Countdown settings Zone 0 -->
                  <div id="countdownSettingsZone0Div" style="display: none;">
                    <div class="row g-2">
                      <div class="col-6">
                        <label for="countdownFormatZone0" class="form-label">Display format</label>
                        <select id="countdownFormatZone0" class="form-select">
                          <option value="auto">Auto</option>
                          <option value="YY">YY</option>
                          <option value="YYMM">YY MM</option>
                          <option value="YYMMDDHHMM">YY MM DD HH MM</option>
                          <option value="YYMMDDHHMMSS">YY MM DD HH MM SS</option>
                          <option value="DD">DD</option>
                          <option value="DDHH">DD HH</option>
                          <option value="DDHHMM">DD HH MM</option>
                          <option value="DDHHMMSS">DD HH MM SS</option>
                          <option value="DDMM">DD MM</option>
                          <option value="HH">HH</option>
                          <option value="HHMM">HH MM</option>
                          <option value="HHMMSS">HH MM SS</option>
                          <option value="MM">MM</option>
                          <option value="MMSS">MM SS</option>
                          <option value="SS">SS</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownFinishZone0" class="form-label">On finish</label>
                        <select id="countdownFinishZone0" class="form-select">
                          <option value="blinkForever">Blink forever</option>
                          <option value="blink3">Blink 3 times</option>
                          <option value="none">Static</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownPrefixZone0" class="form-label">Prefix text</label>
                        <input type="text" class="form-control" id="countdownPrefixZone0" maxlength="10">
                      </div>
                      <div class="col-6">
                        <label for="countdownSuffixZone0" class="form-label">Suffix text</label>
                        <input type="text" class="form-control" id="countdownSuffixZone0" maxlength="10">
                      </div>
                      <div class="col-12">
                        <div class="form-check form-switch pt-2">
                          <input class="form-check-input" type="checkbox" id="countdownShowUnitsZone0">
                          <label class="form-check-label" for="countdownShowUnitsZone0">Show units (d, h, m, s)</label>
                        </div>
                      </div>
                    </div>
                  </div>

                  <div class="col-5" id="fontZone0div">
                    <label for="fontZone0" class="form-label">Font</label>
                    <select id="fontZone0" class="form-select">
                        <option value="default">Default</option>
                        <option value="wledFont">Wled font</option>
                        <option value="wledFont_cyrillic">Cyrillic</option>
                        <option value="wledSymbolFont">Wled Symbol Font</option>
                        <option value="wledFont_CompactSymbols">Compact + Symbols</option>
                    </select>
                  </div>
                  <div class="col-7">
                    <label for="charspacingZone0" class="form-label">Character spacing</label>
                    <select id="charspacingZone0" class="form-select">
                        <option value="0">0</option>
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                        <option value="4">4</option>
                        <option value="5">5</option>
                      </select>
                  </div>

                  <div class="col-5" id="scrollEffectZone0InDiv">
                    <label for="scrollEffectZone0In" class="form-label">Scroll effect <span class="text-primary"><b>IN</b></span></label>
                    <select id="scrollEffectZone0In" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" >SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>

                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                        <option value="PACMAN">Pacman</option>
                        <option value="WAVE">Wave</option>
                        <option value="ROLL">Roll</option>
                        <option value="LINES">Lines</option>
                        <option value="ARROW">Arrow</option>
                        <option value="SAILBOAT">Sailboat</option>
                        <option value="STEAMBOAT">Steamboat</option>
                        <option value="HEART">Heart</option>
                        <option value="INVADER">Invader</option>
                        <option value="ROCKET">Rocket</option>
                        <option value="FBALL">Fireball</option>
                        <option value="CHEVRON">Chevron</option>
                        <option value="WALKER">Walker</option>
                        <option value="MARIO">Mario</option>
                        <option value="GHOST">Ghost</option>
                        <option value="DINO">Dino</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollSpeedZone0Div">
                      <label for="scrollSpeedZone0" class="form-label">Scroll speed</label>
                      <div class="input-group mb-3">
                          <input type="text" class="form-control" id="scrollSpeedZone0" aria-describedby="scrollSpeedZone0Help">
                          <span class="input-group-text">ms</span>
                      </div>
                  </div>

                  <div class="col-5" id="scrollEffectZone0OutDiv">
                    <label for="scrollEffectZone0Out" class="form-label">Scroll effect <span class="text-primary"><b>OUT</b></span></label>
                    <select id="scrollEffectZone0Out" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" selected>SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PACMAN">Pacman</option>
                        <option value="WAVE">Wave</option>
                        <option value="ROLL">Roll</option>
                        <option value="LINES">Lines</option>
                        <option value="ARROW">Arrow</option>
                        <option value="SAILBOAT">Sailboat</option>
                        <option value="STEAMBOAT">Steamboat</option>
                        <option value="HEART">Heart</option>
                        <option value="INVADER">Invader</option>
                        <option value="ROCKET">Rocket</option>
                        <option value="FBALL">Fireball</option>
                        <option value="CHEVRON">Chevron</option>
                        <option value="WALKER">Walker</option>
                        <option value="MARIO">Mario</option>
                        <option value="GHOST">Ghost</option>
                        <option value="DINO">Dino</option>
                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollPauseZone0Div">
                    <label for="scrollPauseZone0" class="form-label">Scroll pause</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollPauseZone0">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>

                  <div class="col-5" id="scrollAlignZone0Div">
                    <label for="scrollAlignZone0" class="form-label">Alignment</label>
                    <select id="scrollAlignZone0" class="form-select">
                        <option value="PA_LEFT">Left</option>
                        <option value="PA_CENTER">Center</option>
                        <option value="PA_RIGHT">Right</option>
                      </select>
                  </div>
                  

                  <button id="zone0Settings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>

                  <div id="zone1Wrapper" class="row g-3">
                  <div class="col-12"><hr class="my-4"></div>
                  <div class="col-12"><h3 class="mb-3">Zone 1</h3></div>

                  <div class="col-5">
                    <label for="workModeZone1" class="form-label">Working mode</label>
                    <select id="workModeZone1" class="form-select">
                        <option value="mqttClient">MQTT client</option>
                        <option value="manualInput">Manual input</option>
                        <option value="wallClock">Wall clock</option>
                        <option value="owmWeather">Open Weather map</option>
                        <option value="haClient">Home Assistant client</option>
                        <option value="intTempSensor">Internal Temperature sensor (ds18b20)</option>
                        <option value="wopr">WOPR Mode (WarGames)</option>
                        <option value="countdown">Countdown timer</option>
                    </select>
                  </div>
                  
                  <div class="col-7" id="emptyZone1Div"></div>
                  <div class="col-7" id="countdownTargetZone1Div" style="display: none;">
                    <label for="countdownTargetZone1" class="form-label">Target date/time</label>
                    <input type="datetime-local" class="form-control" id="countdownTargetZone1" step="1">
                  </div>
                  <div class="col-5" id="mqttZone1PrefixDiv" style="display: none;">
                    <label for="mqttZone1Prefix" class="form-label">MQTT zone text topic</label>
                        <input type="text" class="form-control" id="mqttTextTopicZone1">
                  </div>
                  <div class="col-2" id="mqttPostfixZone1Div" style="display: none;">
                    <label for="mqttPostfixZone1" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="mqttPostfixZone1">
                  </div>
                  <div class="col-7" id="clockDisplayFormatZone1Div" style="display: none;">
                    <label for="clockDisplayFormatZone1" class="form-label">Time format</label>
                        <select id="clockDisplayFormatZone1" class="form-select">
                          <option value="HHMM">HH:MM</option>
                          <option value="HHMMSS">HH:MM:SS</option>
                          <option value="HH">HH</option>
                          <option value="MM">MM</option>
                          <option value="ddmmyyyy">dd.mm.yyyy</option>
                          <option value="ddmm">dd.mm</option>
                          <option value="ddmmaa">dd.mm aa</option>
                          <option value="aa">aa</option>
                          <option value="ddmmaahhmm">ddmmaahhmm</option>
                        </select>
                  </div>
                  <div class="col-12" id="scrollInfiniteZone1Div" style="display: none;">
                    <div class="form-check form-switch pt-4">
                      <input class="form-check-input" type="checkbox" id="scrollInfiniteZone1" onChange="preparePostRequest(event, this.id, this.checked);">
                      <label class="form-check-label" for="scrollInfiniteZone1">Infinite Scroll</label>
                    </div>
                  </div>
                  <div class="col-7" id="owmWhatToDisplayZone1div" style="display: none;">
                    <label for="owmWhatToDisplayZone1" class="form-label">What to display</label>
                    <select id="owmWhatToDisplayZone1" class="form-select">
                        <option value="owmTemperature">Temperature</option>
                        <option value="owmHumidity">Humidity</option>
                        <option value="owmPressure">Pressure</option>
                        <option value="owmWindSpeed">Wind speed</option>
                        <option value="owmWeatherIcon">Wether icon</option>
                    </select>
                  </div>
                  <div class="col-5" id="haSensorIdZone1Div" style="display: none;">
                    <label for="haSensorIdZone1" class="form-label">Sensor ID</label>
                        <input type="text" class="form-control" id="haSensorIdZone1">
                  </div>
                  <div class="col-2" id="haSensorPostfixZone1Div" style="display: none;">
                    <label for="haSensorPostfixZone1" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="haSensorPostfixZone1">
                  </div>
                  <div class="col-7" id="ds18b20PostfixZone1Div" style="display: none;">
                    <label for="ds18b20PostfixZone1" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="ds18b20PostfixZone1">
                  </div>
                  <!-- Countdown settings Zone 1 -->
                  <div id="countdownSettingsZone1Div" style="display: none;">
                    <div class="row g-2">
                      <div class="col-6">
                        <label for="countdownFormatZone1" class="form-label">Display format</label>
                        <select id="countdownFormatZone1" class="form-select">
                          <option value="auto">Auto</option>
                          <option value="YY">YY</option>
                          <option value="YYMM">YY MM</option>
                          <option value="YYMMDDHHMM">YY MM DD HH MM</option>
                          <option value="YYMMDDHHMMSS">YY MM DD HH MM SS</option>
                          <option value="DD">DD</option>
                          <option value="DDHH">DD HH</option>
                          <option value="DDHHMM">DD HH MM</option>
                          <option value="DDHHMMSS">DD HH MM SS</option>
                          <option value="DDMM">DD MM</option>
                          <option value="HH">HH</option>
                          <option value="HHMM">HH MM</option>
                          <option value="HHMMSS">HH MM SS</option>
                          <option value="MM">MM</option>
                          <option value="MMSS">MM SS</option>
                          <option value="SS">SS</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownFinishZone1" class="form-label">On finish</label>
                        <select id="countdownFinishZone1" class="form-select">
                          <option value="blinkForever">Blink forever</option>
                          <option value="blink3">Blink 3 times</option>
                          <option value="none">Static</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownPrefixZone1" class="form-label">Prefix text</label>
                        <input type="text" class="form-control" id="countdownPrefixZone1" maxlength="10">
                      </div>
                      <div class="col-6">
                        <label for="countdownSuffixZone1" class="form-label">Suffix text</label>
                        <input type="text" class="form-control" id="countdownSuffixZone1" maxlength="10">
                      </div>
                      <div class="col-12">
                        <div class="form-check form-switch pt-2">
                          <input class="form-check-input" type="checkbox" id="countdownShowUnitsZone1">
                          <label class="form-check-label" for="countdownShowUnitsZone1">Show units (d, h, m, s)</label>
                        </div>
                      </div>
                    </div>
                  </div>

                  <div class="col-5" id="fontZone1div">
                    <label for="fontZone1" class="form-label">Font</label>
                    <select id="fontZone1" class="form-select">
                        <option value="default">Default</option>
                        <option value="wledFont">Wled font</option>
                        <option value="wledFont_cyrillic">Cyrillic</option>
                        <option value="wledSymbolFont">Wled Symbol Font</option>
                        <option value="wledFont_CompactSymbols">Compact + Symbols</option>
                    </select>
                  </div>
                  <div class="col-7">
                    <label for="charspacingZone1" class="form-label">Character spacing</label>
                    <select id="charspacingZone1" class="form-select">
                        <option value="0">0</option>
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                        <option value="4">4</option>
                        <option value="5">5</option>
                      </select>
                  </div>

                  <div class="col-5" id="scrollEffectZone1InDiv">
                    <label for="scrollEffectZone1In" class="form-label">Scroll effect <span class="text-primary"><b>IN</b></span></label>
                    <select id="scrollEffectZone1In" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" >SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>

                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollSpeedZone1Div">
                      <label for="scrollSpeedZone1" class="form-label">Scroll speed</label>
                      <div class="input-group mb-3">
                          <input type="text" class="form-control" id="scrollSpeedZone1" aria-describedby="scrollSpeedZone1Help">
                          <span class="input-group-text">ms</span>
                      </div>
                  </div>

                  <div class="col-5" id="scrollEffectZone1OutDiv">
                    <label for="scrollEffectZone1Out" class="form-label">Scroll effect <span class="text-primary"><b>OUT</b></span></label>
                    <select id="scrollEffectZone1Out" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" selected>SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PACMAN">Pacman</option>
                        <option value="WAVE">Wave</option>
                        <option value="ROLL">Roll</option>
                        <option value="LINES">Lines</option>
                        <option value="ARROW">Arrow</option>
                        <option value="SAILBOAT">Sailboat</option>
                        <option value="STEAMBOAT">Steamboat</option>
                        <option value="HEART">Heart</option>
                        <option value="INVADER">Invader</option>
                        <option value="ROCKET">Rocket</option>
                        <option value="FBALL">Fireball</option>
                        <option value="CHEVRON">Chevron</option>
                        <option value="WALKER">Walker</option>
                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollPauseZone1Div">
                    <label for="scrollPauseZone1" class="form-label">Scroll pause</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollPauseZone1">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>

                  <div class="col-5" id="scrollAlignZone1Div">
                    <label for="scrollAlignZone1" class="form-label">Alignment</label>
                    <select id="scrollAlignZone1" class="form-select">
                        <option value="PA_LEFT">Left</option>
                        <option value="PA_CENTER">Center</option>
                        <option value="PA_RIGHT">Right</option>
                      </select>
                  </div>
                  

                  <button id="zone1Settings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>
                  </div>

                  <div id="zone2Wrapper" class="row g-3">
                  <div class="col-12"><hr class="my-4"></div>
                  <div class="col-12"><h3 class="mb-3">Zone 2</h3></div>

                  <div class="col-5">
                    <label for="workModeZone2" class="form-label">Working mode</label>
                    <select id="workModeZone2" class="form-select">
                        <option value="mqttClient">MQTT client</option>
                        <option value="manualInput">Manual input</option>
                        <option value="wallClock">Wall clock</option>
                        <option value="owmWeather">Open Weather map</option>
                        <option value="haClient">Home Assistant client</option>
                        <option value="intTempSensor">Internal Temperature sensor (ds18b20)</option>
                        <option value="wopr">WOPR Mode (WarGames)</option>
                        <option value="countdown">Countdown timer</option>
                    </select>
                  </div>
                  
                  <div class="col-7" id="emptyZone2Div"></div>
                  <div class="col-7" id="countdownTargetZone2Div" style="display: none;">
                    <label for="countdownTargetZone2" class="form-label">Target date/time</label>
                    <input type="datetime-local" class="form-control" id="countdownTargetZone2" step="1">
                  </div>
                  <div class="col-5" id="mqttZone2PrefixDiv" style="display: none;">
                    <label for="mqttZone2Prefix" class="form-label">MQTT zone text topic</label>
                        <input type="text" class="form-control" id="mqttTextTopicZone2">
                  </div>
                  <div class="col-2" id="mqttPostfixZone2Div" style="display: none;">
                    <label for="mqttPostfixZone2" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="mqttPostfixZone2">
                  </div>
                  <div class="col-7" id="clockDisplayFormatZone2Div" style="display: none;">
                    <label for="clockDisplayFormatZone2" class="form-label">Time format</label>
                        <select id="clockDisplayFormatZone2" class="form-select">
                          <option value="HHMM">HH:MM</option>
                          <option value="HHMMSS">HH:MM:SS</option>
                          <option value="HH">HH</option>
                          <option value="MM">MM</option>
                          <option value="ddmmyyyy">dd.mm.yyyy</option>
                          <option value="ddmm">dd.mm</option>
                          <option value="ddmmaa">dd.mm aa</option>
                          <option value="aa">aa</option>
                          <option value="ddmmaahhmm">ddmmaahhmm</option>
                        </select>
                  </div>
                  <div class="col-12" id="scrollInfiniteZone2Div" style="display: none;">
                    <div class="form-check form-switch pt-4">
                      <input class="form-check-input" type="checkbox" id="scrollInfiniteZone2" onChange="preparePostRequest(event, this.id, this.checked);">
                      <label class="form-check-label" for="scrollInfiniteZone2">Infinite Scroll</label>
                    </div>
                  </div>
                  <div class="col-7" id="owmWhatToDisplayZone2div" style="display: none;">
                    <label for="owmWhatToDisplayZone2" class="form-label">What to display</label>
                    <select id="owmWhatToDisplayZone2" class="form-select">
                        <option value="owmTemperature">Temperature</option>
                        <option value="owmHumidity">Humidity</option>
                        <option value="owmPressure">Pressure</option>
                        <option value="owmWindSpeed">Wind speed</option>
                        <option value="owmWeatherIcon">Wether icon</option>
                    </select>
                  </div>
                  <div class="col-5" id="haSensorIdZone2Div" style="display: none;">
                    <label for="haSensorIdZone2" class="form-label">Sensor ID</label>
                        <input type="text" class="form-control" id="haSensorIdZone2">
                  </div>
                  <div class="col-2" id="haSensorPostfixZone2Div" style="display: none;">
                    <label for="haSensorPostfixZone2" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="haSensorPostfixZone2">
                  </div>
                  <div class="col-7" id="ds18b20PostfixZone2Div" style="display: none;">
                    <label for="ds18b20PostfixZone2" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="ds18b20PostfixZone2">
                  </div>
                  <!-- Countdown settings Zone 2 -->
                  <div id="countdownSettingsZone2Div" style="display: none;">
                    <div class="row g-2">
                      <div class="col-6">
                        <label for="countdownFormatZone2" class="form-label">Display format</label>
                        <select id="countdownFormatZone2" class="form-select">
                          <option value="auto">Auto</option>
                          <option value="YY">YY</option>
                          <option value="YYMM">YY MM</option>
                          <option value="YYMMDDHHMM">YY MM DD HH MM</option>
                          <option value="YYMMDDHHMMSS">YY MM DD HH MM SS</option>
                          <option value="DD">DD</option>
                          <option value="DDHH">DD HH</option>
                          <option value="DDHHMM">DD HH MM</option>
                          <option value="DDHHMMSS">DD HH MM SS</option>
                          <option value="DDMM">DD MM</option>
                          <option value="HH">HH</option>
                          <option value="HHMM">HH MM</option>
                          <option value="HHMMSS">HH MM SS</option>
                          <option value="MM">MM</option>
                          <option value="MMSS">MM SS</option>
                          <option value="SS">SS</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownFinishZone2" class="form-label">On finish</label>
                        <select id="countdownFinishZone2" class="form-select">
                          <option value="blinkForever">Blink forever</option>
                          <option value="blink3">Blink 3 times</option>
                          <option value="none">Static</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownPrefixZone2" class="form-label">Prefix text</label>
                        <input type="text" class="form-control" id="countdownPrefixZone2" maxlength="10">
                      </div>
                      <div class="col-6">
                        <label for="countdownSuffixZone2" class="form-label">Suffix text</label>
                        <input type="text" class="form-control" id="countdownSuffixZone2" maxlength="10">
                      </div>
                      <div class="col-12">
                        <div class="form-check form-switch pt-2">
                          <input class="form-check-input" type="checkbox" id="countdownShowUnitsZone2">
                          <label class="form-check-label" for="countdownShowUnitsZone2">Show units (d, h, m, s)</label>
                        </div>
                      </div>
                    </div>
                  </div>

                  <div class="col-5" id="fontZone2div">
                    <label for="fontZone2" class="form-label">Font</label>
                    <select id="fontZone2" class="form-select">
                        <option value="default">Default</option>
                        <option value="wledFont">Wled font</option>
                        <option value="wledFont_cyrillic">Cyrillic</option>
                        <option value="wledSymbolFont">Wled Symbol Font</option>
                        <option value="wledFont_CompactSymbols">Compact + Symbols</option>
                    </select>
                  </div>
                  <div class="col-7">
                    <label for="charspacingZone2" class="form-label">Character spacing</label>
                    <select id="charspacingZone2" class="form-select">
                        <option value="0">0</option>
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                        <option value="4">4</option>
                        <option value="5">5</option>
                      </select>
                  </div>

                  <div class="col-5" id="scrollEffectZone2InDiv">
                    <label for="scrollEffectZone2In" class="form-label">Scroll effect <span class="text-primary"><b>IN</b></span></label>
                    <select id="scrollEffectZone2In" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" >SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PACMAN">Pacman</option>
                        <option value="WAVE">Wave</option>
                        <option value="ROLL">Roll</option>
                        <option value="LINES">Lines</option>
                        <option value="ARROW">Arrow</option>
                        <option value="SAILBOAT">Sailboat</option>
                        <option value="STEAMBOAT">Steamboat</option>
                        <option value="HEART">Heart</option>
                        <option value="INVADER">Invader</option>
                        <option value="ROCKET">Rocket</option>
                        <option value="FBALL">Fireball</option>
                        <option value="CHEVRON">Chevron</option>
                        <option value="WALKER">Walker</option>
                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollSpeedZone2Div">
                      <label for="scrollSpeedZone2" class="form-label">Scroll speed</label>
                      <div class="input-group mb-3">
                          <input type="text" class="form-control" id="scrollSpeedZone2" aria-describedby="scrollSpeedZone2Help">
                          <span class="input-group-text">ms</span>
                      </div>
                  </div>

                  <div class="col-5" id="scrollEffectZone2OutDiv">
                    <label for="scrollEffectZone2Out" class="form-label">Scroll effect <span class="text-primary"><b>OUT</b></span></label>
                    <select id="scrollEffectZone2Out" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" selected>SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PACMAN">Pacman</option>
                        <option value="WAVE">Wave</option>
                        <option value="ROLL">Roll</option>
                        <option value="LINES">Lines</option>
                        <option value="ARROW">Arrow</option>
                        <option value="SAILBOAT">Sailboat</option>
                        <option value="STEAMBOAT">Steamboat</option>
                        <option value="HEART">Heart</option>
                        <option value="INVADER">Invader</option>
                        <option value="ROCKET">Rocket</option>
                        <option value="FBALL">Fireball</option>
                        <option value="CHEVRON">Chevron</option>
                        <option value="WALKER">Walker</option>
                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollPauseZone2Div">
                    <label for="scrollPauseZone2" class="form-label">Scroll pause</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollPauseZone2">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>

                  <div class="col-5" id="scrollAlignZone2Div">
                    <label for="scrollAlignZone2" class="form-label">Alignment</label>
                    <select id="scrollAlignZone2" class="form-select">
                        <option value="PA_LEFT">Left</option>
                        <option value="PA_CENTER">Center</option>
                        <option value="PA_RIGHT">Right</option>
                      </select>
                  </div>
      
                  <button id="zone2Settings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>
                  </div>

                  <div id="zone3WrapperPart1" class="row g-3">
                  <div class="col-12"><hr class="my-4"></div>
                  <div class="col-12"><h3 class="mb-3">Zone 3</h3></div>

                  <div class="col-5">
                    <label for="workModeZone3" class="form-label">Working mode</label>
                    <select id="workModeZone3" class="form-select">
                        <option value="mqttClient">MQTT client</option>
                        <option value="manualInput">Manual input</option>
                        <option value="wallClock">Wall clock</option>
                        <option value="owmWeather">Open Weather map</option>
                        <option value="haClient">Home Assistant client</option>
                        <option value="intTempSensor">Internal Temperature sensor (ds18b20)</option>
                        <option value="wopr">WOPR Mode (WarGames)</option>
                        <option value="countdown">Countdown timer</option>
                    </select>
                  </div>
                  
                  <div class="col-7" id="emptyZone3Div"></div>
                  <div class="col-7" id="countdownTargetZone3Div" style="display: none;">
                    <label for="countdownTargetZone3" class="form-label">Target date/time</label>
                    <input type="datetime-local" class="form-control" id="countdownTargetZone3" step="1">
                  </div>
                  <div class="col-5" id="mqttZone3PrefixDiv" style="display: none;">
                    <label for="mqttZone3Prefix" class="form-label">MQTT zone text topic</label>
                        <input type="text" class="form-control" id="mqttTextTopicZone3">
                  </div>
                  <div class="col-2" id="mqttPostfixZone3Div" style="display: none;">
                    <label for="mqttPostfixZone3" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="mqttPostfixZone3">
                  </div>
                  <div class="col-7" id="clockDisplayFormatZone3Div" style="display: none;">
                    <label for="clockDisplayFormatZone3" class="form-label">Time format</label>
                        <select id="clockDisplayFormatZone3" class="form-select">
                          <option value="HHMM">HH:MM</option>
                          <option value="HHMMSS">HH:MM:SS</option>
                          <option value="HH">HH</option>
                          <option value="MM">MM</option>
                          <option value="ddmmyyyy">dd.mm.yyyy</option>
                          <option value="ddmm">dd.mm</option>
                          <option value="ddmmaa">dd.mm aa</option>
                          <option value="aa">aa</option>
                          <option value="ddmmaahhmm">ddmmaahhmm</option>
                        </select>
                  </div>
                  <div class="col-12" id="scrollInfiniteZone3Div" style="display: none;">
                    <div class="form-check form-switch pt-4">
                      <input class="form-check-input" type="checkbox" id="scrollInfiniteZone3" onChange="preparePostRequest(event, this.id, this.checked);">
                      <label class="form-check-label" for="scrollInfiniteZone3">Infinite Scroll</label>
                    </div>
                  </div>
                  <div class="col-7" id="owmWhatToDisplayZone3div" style="display: none;">
                    <label for="owmWhatToDisplayZone3" class="form-label">What to display</label>
                    <select id="owmWhatToDisplayZone3" class="form-select">
                        <option value="owmTemperature">Temperature</option>
                        <option value="owmHumidity">Humidity</option>
                        <option value="owmPressure">Pressure</option>
                        <option value="owmWindSpeed">Wind speed</option>
                        <option value="owmWeatherIcon">Wether icon</option>
                    </select>
                  </div>
                  <div class="col-5" id="haSensorIdZone3Div" style="display: none;">
                    <label for="haSensorIdZone3" class="form-label">Sensor ID</label>
                        <input type="text" class="form-control" id="haSensorIdZone3">
                  </div>
                  <div class="col-2" id="haSensorPostfixZone3Div" style="display: none;">
                    <label for="haSensorPostfixZone3" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="haSensorPostfixZone3">
                  </div>
                  <div class="col-7" id="ds18b20PostfixZone3Div" style="display: none;">
                    <label for="ds18b20PostfixZone3" class="form-label">Postfix</label>
                        <input type="text" class="form-control" id="ds18b20PostfixZone3">
                  </div>
                  <!-- Countdown settings Zone 3 -->
                  <div id="countdownSettingsZone3Div" style="display: none;">
                    <div class="row g-2">
                      <div class="col-6">
                        <label for="countdownFormatZone3" class="form-label">Display format</label>
                        <select id="countdownFormatZone3" class="form-select">
                          <option value="auto">Auto</option>
                          <option value="YY">YY</option>
                          <option value="YYMM">YY MM</option>
                          <option value="YYMMDDHHMM">YY MM DD HH MM</option>
                          <option value="YYMMDDHHMMSS">YY MM DD HH MM SS</option>
                          <option value="DD">DD</option>
                          <option value="DDHH">DD HH</option>
                          <option value="DDHHMM">DD HH MM</option>
                          <option value="DDHHMMSS">DD HH MM SS</option>
                          <option value="DDMM">DD MM</option>
                          <option value="HH">HH</option>
                          <option value="HHMM">HH MM</option>
                          <option value="HHMMSS">HH MM SS</option>
                          <option value="MM">MM</option>
                          <option value="MMSS">MM SS</option>
                          <option value="SS">SS</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownFinishZone3" class="form-label">On finish</label>
                        <select id="countdownFinishZone3" class="form-select">
                          <option value="blinkForever">Blink forever</option>
                          <option value="blink3">Blink 3 times</option>
                          <option value="none">Static</option>
                        </select>
                      </div>
                      <div class="col-6">
                        <label for="countdownPrefixZone3" class="form-label">Prefix text</label>
                        <input type="text" class="form-control" id="countdownPrefixZone3" maxlength="10">
                      </div>
                      <div class="col-6">
                        <label for="countdownSuffixZone3" class="form-label">Suffix text</label>
                        <input type="text" class="form-control" id="countdownSuffixZone3" maxlength="10">
                      </div>
                      <div class="col-12">
                        <div class="form-check form-switch pt-2">
                          <input class="form-check-input" type="checkbox" id="countdownShowUnitsZone3">
                          <label class="form-check-label" for="countdownShowUnitsZone3">Show units (d, h, m, s)</label>
                        </div>
                      </div>
                    </div>
                  </div>

                  <div class="col-5" id="fontZone3div">
                    <label for="fontZone3" class="form-label">Font</label>
                    <select id="fontZone3" class="form-select">
                        <option value="default">Default</option>
                        <option value="wledFont">Wled font</option>
                        <option value="wledFont_cyrillic">Cyrillic</option>
                        <option value="wledSymbolFont">Wled Symbol Font</option>
                        <option value="wledFont_CompactSymbols">Compact + Symbols</option>
                    </select>
                  </div>
                  <div class="col-7">
                    <label for="charspacingZone3" class="form-label">Character spacing</label>
                    <select id="charspacingZone3" class="form-select">
                        <option value="0">0</option>
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                        <option value="4">4</option>
                        <option value="5">5</option>
                      </select>
                  </div>

                  <div class="col-5" id="scrollEffectZone3InDiv">
                    <label for="scrollEffectZone3In" class="form-label">Scroll effect <span class="text-primary"><b>IN</b></span></label>
                    <select id="scrollEffectZone3In" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" >SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PACMAN">Pacman</option>
                        <option value="WAVE">Wave</option>
                        <option value="ROLL">Roll</option>
                        <option value="LINES">Lines</option>
                        <option value="ARROW">Arrow</option>
                        <option value="SAILBOAT">Sailboat</option>
                        <option value="STEAMBOAT">Steamboat</option>
                        <option value="HEART">Heart</option>
                        <option value="INVADER">Invader</option>
                        <option value="ROCKET">Rocket</option>
                        <option value="FBALL">Fireball</option>
                        <option value="CHEVRON">Chevron</option>
                        <option value="WALKER">Walker</option>
                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollSpeedZone3Div">
                      <label for="scrollSpeedZone3" class="form-label">Scroll speed</label>
                      <div class="input-group mb-3">
                          <input type="text" class="form-control" id="scrollSpeedZone3" aria-describedby="scrollSpeedZone3Help">
                          <span class="input-group-text">ms</span>
                      </div>
                  </div>

                  <div class="col-5" id="scrollEffectZone3OutDiv">
                    <label for="scrollEffectZone3Out" class="form-label">Scroll effect <span class="text-primary"><b>OUT</b></span></label>
                    <select id="scrollEffectZone3Out" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" selected>SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PACMAN">Pacman</option>
                        <option value="WAVE">Wave</option>
                        <option value="ROLL">Roll</option>
                        <option value="LINES">Lines</option>
                        <option value="ARROW">Arrow</option>
                        <option value="SAILBOAT">Sailboat</option>
                        <option value="STEAMBOAT">Steamboat</option>
                        <option value="HEART">Heart</option>
                        <option value="INVADER">Invader</option>
                        <option value="ROCKET">Rocket</option>
                        <option value="FBALL">Fireball</option>
                        <option value="CHEVRON">Chevron</option>
                        <option value="WALKER">Walker</option>
                        <option value="PA_SLICE">SLICE</option>
                        <option value="PA_MESH">MESH</option>
                        <option value="PA_FADE">FADE</option>
                        <option value="PA_DISSOLVE">DISSOLVE</option>
                        <option value="PA_BLINDS">BLINDS</option>
                        <option value="PA_RANDOM">RANDOM</option>
                        <option value="PA_WIPE">WIPE</option>
                        <option value="PA_WIPE_CURSOR">WIPE_CURSOR</option>
                        <option value="PA_SCAN_HORIZ">SCAN_HORIZ</option>
                        <option value="PA_SCAN_VERT">SCAN_VERT</option>
                        <option value="PA_OPENING">OPENING</option>
                        <option value="PA_OPENING_CURSOR">OPENING_CURSOR</option>
                        <option value="PA_CLOSING">CLOSING</option>
                        <option value="PA_CLOSING_CURSOR">CLOSING_CURSOR</option>
                        <option value="PA_SCROLL_UP_LEFT">SCROLL_UP_LEFT</option>
                        <option value="PA_SCROLL_UP_RIGHT">SCROLL_UP_RIGHT</option>
                        <option value="PA_SCROLL_DOWN_LEFT">SCROLL_DOWN_LEFT</option>
                        <option value="PA_SCROLL_DOWN_RIGHT">SCROLL_DOWN_RIGHT</option>
                        <option value="PA_GROW_UP">GROW_UP</option>
                        <option value="PA_GROW_DOWN">GROW_DOWN</option>
                      </select>
                  </div>
                  <div class="col-7" id="scrollPauseZone3Div">
                    <label for="scrollPauseZone3" class="form-label">Scroll pause</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollPauseZone3">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>

                  <div class="col-5" id="scrollAlignZone3Div">
                    <label for="scrollAlignZone3" class="form-label">Alignment</label>
                    <select id="scrollAlignZone3" class="form-select">
                        <option value="PA_LEFT">Left</option>
                        <option value="PA_CENTER">Center</option>
                        <option value="PA_RIGHT">Right</option>
                      </select>
                  </div>
                  
                  </div>
                  <!-- SPLIT POINT -->
                  
                  </div>)=====";

const char settingsPagePart2[] PROGMEM =
    R"=====(<div class="row g-3" id="zone3ApplyRow">
                <div class="col-12">
                <button id="zone3Settings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>
                </div>
                 </div>


                <hr class="my-4">
                  <h3 class="mb-3">MQTT settings</h3>

                  <div class="col-12 align-self-center">
                    <div class="form-check">
                      <input class="form-check-input" type="checkbox" value="mqttEnable" id="mqttEnable">
                      <label class="form-check-label" for="mqttEnable">
                        Enable MQTT
                      </label>
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="mqttServerAddress" class="form-label">Server address</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttServerAddress">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttServerPort" class="form-label">Server port</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttServerPort">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttUsername" class="form-label">Username</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttUsername">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttPassword" class="form-label">Password</label>
                    <div class="input-group mb-3">
                        <input type="password" class="form-control" id="mqttPassword">
                    </div>
                  </div>

                <button id="applyMqttSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>

                
                <hr class="my-4">
                  <h3 class="mb-3">Wall clock settings</h3>
                  <div class="col-6">
                    <label class="form-label">NTP Time server</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="ntpServer">
                    </div>
                  </div>
                  <div class="col-3">
                    <label for="ntpTimeZone" class="form-label">Time zone</label>
                    <div class="input-group mb-3">
                        <span class="input-group-text">UTC</span>
                        <input type="number" step="0.25" class="form-control" id="ntpTimeZone">
                    </div>
                  </div>
                  <div class="col-3">
                    <label for="timeZone" class="form-label">Update interval</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="ntpUpdateInterval">
                        <span class="input-group-text">Hours</span>
                    </div>
                  </div>
                  <div class="col-12 align-self-center">
                    <div class="form-check">
                      <input class="form-check-input" type="checkbox" value="disableDotsblink" id="disableDotsBlink">
                      <label class="form-check-label" for="disableDotsBlink">
                        Disable clock dots blink
                      </label>
                    </div>
                  </div>
                  

                <button id="applyWallClockSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>


                <hr class="my-4">
                  <h3 class="mb-4">Open Weather map settings</h3>                 
                  <div class="col-6">
                    <label for="owmCity" class="form-label">City <small>Name</small></label>
                    <div class="input-group mb-4">
                        <input type="text" class="form-control" id="owmCity">
                    </div>
                  </div>
                  <div class="col-6">
                    <label for="owmApiToken" class="form-label">API token</label>
                    <div class="input-group mb-4">
                        <input type="password" class="form-control" id="owmApiToken">
                    </div>
                  </div>

                  <div class="col-6">
                    <label class="form-label">Update interval</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="owmUpdateInterval">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>
                  <div class="col-6">
                    <label for="clockDisplayFormat" class="form-label">Units format</label>
                    <div class="input-group mb-3">
                        <select id="owmUnitsFormat" class="form-select">
                          <option value="metric">metric</option>
                          <option value="imperial">imperial</option>
                        </select>
                    </div>
                  </div>
                  

                <button id="applyOwmSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>


                <hr class="my-4">
                  <h3 class="mb-4">Home Assistant client settings</h3>
                  <div class="col-3">
                    <label for="forhaApiHttpType" class="form-label">HTTP or HTTPS</label>
                    <div class="input-group mb-3">
                        <select id="haApiHttpType" class="form-select">
                          <option value="http" selected="selected">HTTP</option>
                          <option value="https">HTTPS</option>
                        </select>
                    </div>
                  </div>
                  <div class="col-6">
                    <label for="haAddr" class="form-label">HA address</label><small class="form-text text-muted"> [without HTTPS or HTTP or / ]</small>
                    <div class="input-group mb-4">
                        <input type="text" class="form-control" id="haAddr">
                    </div>
                  </div>
                  <div class="col-3">
                    <label for="haApiPort" class="form-label">API port</label>
                    <div class="input-group mb-4">
                        <input type="text" class="form-control" id="haApiPort">
                    </div>
                  </div>                  
 
                  <div class="col-9">
                    <label for="haApiToken" class="form-label">API token</label>
                    <div class="input-group mb-4">
                        <input type="password" class="form-control" id="haApiToken">
                    </div>
                  </div>
                  <div class="col-3">
                    <label for="haUpdateInterval" class="form-label">Update interval</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="haUpdateInterval">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>
                                    

                <button id="applyHaSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>


                <hr class="my-4">
                  <h3 class="mb-4">Internal Temperature sensor (ds18b20) settings</h3>
                  <div class="col-6 align-self-center">
                    <div class="form-check">
                      <input class="form-check-input" type="checkbox" value="ds18b20Enable" id="ds18b20Enable">
                      <label class="form-check-label" for="ds18b20Enable">
                        Enable Temperature sensor (ds18b20)
                      </label>
                    </div>
                  </div>
                  <div class="col-6 align-self-center">
                        <label for="haApiToken" class="form-label">Measured temperature: <span id="ds18b20TempVal">Loading...</span></label>
                  </div>
                  <div class="col-6">
                    <label class="form-label">Update interval</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="ds18b20UpdateInterval">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>
                  <div class="col-6">
                    <label for="ds18b20UnitsFormat" class="form-label">Units format</label>
                    <div class="input-group mb-3">
                        <select id="ds18b20UnitsFormat" class="form-select">
                          <option value="Celsius">Celsius</option>
                          <option value="Fahrenheit">Fahrenheit</option>
                        </select>
                    </div>
                  </div>
                <button id="applyDs18b20Settings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>



                <div class="col-sm-12"></div>
                <footer class="my-5 pt-5 text-muted text-center text-small border-top">
                  <p class="mb-1">&copy; 2026 wledPixel <span id="footerFwVer"></span></p>
                  <p class="mb-1">Developed by <a href="https://github.com/widapro" class="text-reset text-decoration-none">widapro</a></p>
                  <ul class="list-inline">
                    <li class="list-inline-item"><a href="https://github.com/widapro/wledPixel" target="_blank" class="text-decoration-none">GitHub</a></li>
                    <li class="list-inline-item">|</li>
                    <li class="list-inline-item"><a href="https://github.com/widapro/wledPixel/issues" target="_blank" class="text-decoration-none">Report Bug</a></li>
                  </ul>
                </footer>
              </form>
            </div>
          </div>
    </main>
  </div>


    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p" crossorigin="anonymous"></script>



    <div class="position-fixed top-0 end-0 p-3" style="z-index: 11">
    
        <div id="liveToastDanger" class="toast align-items-center text-white bg-danger border-0" role="alert" aria-live="assertive" aria-atomic="true" data-bs-autohide="true">
            <div class="d-flex">
                <div class="toast-body">
                Error!
                </div>
                <button type="button" class="btn-close btn-close-white me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button>
            </div>
        </div>
        <div id="liveToastSuccess" class="toast align-items-center text-white bg-success border-0" role="alert" aria-live="assertive" aria-atomic="true">
            <div class="d-flex">
                <div class="toast-body">
                Success.
                </div>
                <button type="button" class="btn-close btn-close-white me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button>
            </div>
        </div>
    </div>



<script>
    window.onload = function(e){
      // Fetch settings from API
      fetch('/api/settings')
        .then(response => response.json())
        .then(data => {
            console.log('Settings loaded:', data);
            
            // Populate fields
            document.getElementById("workModeZone0").value = data.workModeZone0;
            document.getElementById("workModeZone1").value = data.workModeZone1;
            document.getElementById("workModeZone2").value = data.workModeZone2;
            document.getElementById("workModeZone3").value = data.workModeZone3;
            document.getElementById("zoneNumbers").value = data.zoneNumbers;
            document.getElementById("zone0Begin").value = data.zone0Begin;
            document.getElementById("zone0End").value = data.zone0End;
            document.getElementById("zone1Begin").value = data.zone1Begin;
            document.getElementById("zone1End").value = data.zone1End;
            document.getElementById("zone2Begin").value = data.zone2Begin;
            document.getElementById("zone2End").value = data.zone2End;
            document.getElementById("zone3Begin").value = data.zone3Begin;
            document.getElementById("zone3End").value = data.zone3End;
            document.getElementById("intensity").value = data.intensity;
            document.getElementById("intensityValue").innerHTML = data.intensity;
            document.getElementById("scrollSpeedZone0").value = data.scrollSpeedZone0;
            document.getElementById("scrollSpeedZone1").value = data.scrollSpeedZone1;
            document.getElementById("scrollSpeedZone2").value = data.scrollSpeedZone2;
            document.getElementById("scrollSpeedZone3").value = data.scrollSpeedZone3;
            document.getElementById("scrollPauseZone0").value = data.scrollPauseZone0;
            document.getElementById("scrollPauseZone1").value = data.scrollPauseZone1;
            document.getElementById("scrollPauseZone2").value = data.scrollPauseZone2;
            document.getElementById("scrollPauseZone3").value = data.scrollPauseZone3;
            document.getElementById("scrollAlignZone0").value = data.scrollAlignZone0;
            document.getElementById("scrollAlignZone1").value = data.scrollAlignZone1;
            document.getElementById("scrollAlignZone2").value = data.scrollAlignZone2;
            document.getElementById("scrollAlignZone3").value = data.scrollAlignZone3;
            document.getElementById("scrollEffectZone0In").value = data.scrollEffectZone0In;
            document.getElementById("scrollEffectZone0Out").value = data.scrollEffectZone0Out;
            document.getElementById("scrollEffectZone1In").value = data.scrollEffectZone1In;
            document.getElementById("scrollEffectZone1Out").value = data.scrollEffectZone1Out;
            document.getElementById("scrollEffectZone2In").value = data.scrollEffectZone2In;
            document.getElementById("scrollEffectZone2Out").value = data.scrollEffectZone2Out;
            document.getElementById("scrollEffectZone3In").value = data.scrollEffectZone3In;
            document.getElementById("scrollEffectZone3Out").value = data.scrollEffectZone3Out;
            
            document.getElementById("mqttEnable").checked = data.mqttEnable;
            
            document.getElementById("mqttServerAddress").value = data.mqttServerAddress;
            document.getElementById("mqttServerPort").value = data.mqttServerPort;
            document.getElementById("mqttUsername").value = data.mqttUsername;
            // Password is not sent back for security, or handle if needed
            document.getElementById("mqttPassword").value = data.mqttPassword; 

            document.getElementById("mqttTextTopicZone0").value = data.mqttTextTopicZone0;
            document.getElementById("mqttTextTopicZone1").value = data.mqttTextTopicZone1;
            document.getElementById("mqttTextTopicZone2").value = data.mqttTextTopicZone2;
            document.getElementById("mqttTextTopicZone3").value = data.mqttTextTopicZone3;
            document.getElementById("ntpTimeZone").value = data.ntpTimeZone;
            document.getElementById("ntpUpdateInterval").value = data.ntpUpdateInterval;
            document.getElementById("ntpServer").value = data.ntpServer;
            document.getElementById("clockDisplayFormatZone0").value = data.clockDisplayFormatZone0;
            document.getElementById("clockDisplayFormatZone1").value = data.clockDisplayFormatZone1;
            document.getElementById("clockDisplayFormatZone2").value = data.clockDisplayFormatZone2;
            document.getElementById("clockDisplayFormatZone3").value = data.clockDisplayFormatZone3;
            document.getElementById("owmWhatToDisplayZone0").value = data.owmWhatToDisplayZone0;
            document.getElementById("owmWhatToDisplayZone1").value = data.owmWhatToDisplayZone1;
            document.getElementById("owmWhatToDisplayZone2").value = data.owmWhatToDisplayZone2;
            document.getElementById("owmWhatToDisplayZone3").value = data.owmWhatToDisplayZone3;
            document.getElementById("owmApiToken").value = data.owmApiToken;
            document.getElementById("owmUnitsFormat").value = data.owmUnitsFormat;
            document.getElementById("owmUpdateInterval").value = data.owmUpdateInterval;
            document.getElementById("owmCity").value = data.owmCity;
            document.getElementById("fontZone0").value = data.fontZone0;
            document.getElementById("fontZone1").value = data.fontZone1;
            document.getElementById("fontZone2").value = data.fontZone2;
            document.getElementById("fontZone3").value = data.fontZone3;
            document.getElementById("haAddr").value = data.haAddr;
            document.getElementById("haApiHttpType").value = data.haApiHttpType;
            document.getElementById("haApiToken").value = data.haApiToken;
            document.getElementById("haApiPort").value = data.haApiPort;
            document.getElementById("haUpdateInterval").value = data.haUpdateInterval;
            document.getElementById("haSensorIdZone0").value = data.haSensorIdZone0;
            document.getElementById("haSensorIdZone1").value = data.haSensorIdZone1;
            document.getElementById("haSensorIdZone2").value = data.haSensorIdZone2;
            document.getElementById("haSensorIdZone3").value = data.haSensorIdZone3;
            document.getElementById("haSensorPostfixZone0").value = data.haSensorPostfixZone0;
            document.getElementById("haSensorPostfixZone1").value = data.haSensorPostfixZone1;
            document.getElementById("haSensorPostfixZone2").value = data.haSensorPostfixZone2;
            document.getElementById("haSensorPostfixZone3").value = data.haSensorPostfixZone3;
            document.getElementById("mqttPostfixZone0").value = data.mqttPostfixZone0;
            document.getElementById("mqttPostfixZone1").value = data.mqttPostfixZone1;
            document.getElementById("mqttPostfixZone2").value = data.mqttPostfixZone2;
            document.getElementById("mqttPostfixZone3").value = data.mqttPostfixZone3;
            document.getElementById("scrollInfiniteZone0").checked = data.scrollInfiniteZone0;
            document.getElementById("scrollInfiniteZone1").checked = data.scrollInfiniteZone1;
            document.getElementById("scrollInfiniteZone2").checked = data.scrollInfiniteZone2;
            document.getElementById("scrollInfiniteZone3").checked = data.scrollInfiniteZone3;
            document.getElementById("ds18b20PostfixZone0").value = data.ds18b20PostfixZone0;
            document.getElementById("ds18b20PostfixZone1").value = data.ds18b20PostfixZone1;
            document.getElementById("ds18b20PostfixZone2").value = data.ds18b20PostfixZone2;
            document.getElementById("ds18b20PostfixZone3").value = data.ds18b20PostfixZone3;
            // Countdown settings
            if(data.countdownFormatZone0) document.getElementById("countdownFormatZone0").value = data.countdownFormatZone0;
            if(data.countdownFormatZone1) document.getElementById("countdownFormatZone1").value = data.countdownFormatZone1;
            if(data.countdownFormatZone2) document.getElementById("countdownFormatZone2").value = data.countdownFormatZone2;
            if(data.countdownFormatZone3) document.getElementById("countdownFormatZone3").value = data.countdownFormatZone3;
            if(data.countdownFinishZone0) document.getElementById("countdownFinishZone0").value = data.countdownFinishZone0;
            if(data.countdownFinishZone1) document.getElementById("countdownFinishZone1").value = data.countdownFinishZone1;
            if(data.countdownFinishZone2) document.getElementById("countdownFinishZone2").value = data.countdownFinishZone2;
            if(data.countdownFinishZone3) document.getElementById("countdownFinishZone3").value = data.countdownFinishZone3;
            if(data.countdownTargetZone0) document.getElementById("countdownTargetZone0").value = data.countdownTargetZone0;
            if(data.countdownTargetZone1) document.getElementById("countdownTargetZone1").value = data.countdownTargetZone1;
            if(data.countdownTargetZone2) document.getElementById("countdownTargetZone2").value = data.countdownTargetZone2;
            if(data.countdownTargetZone3) document.getElementById("countdownTargetZone3").value = data.countdownTargetZone3;
            if(data.countdownPrefixZone0) document.getElementById("countdownPrefixZone0").value = data.countdownPrefixZone0;
            if(data.countdownPrefixZone1) document.getElementById("countdownPrefixZone1").value = data.countdownPrefixZone1;
            if(data.countdownPrefixZone2) document.getElementById("countdownPrefixZone2").value = data.countdownPrefixZone2;
            if(data.countdownPrefixZone3) document.getElementById("countdownPrefixZone3").value = data.countdownPrefixZone3;
            if(data.countdownSuffixZone0) document.getElementById("countdownSuffixZone0").value = data.countdownSuffixZone0;
            if(data.countdownSuffixZone1) document.getElementById("countdownSuffixZone1").value = data.countdownSuffixZone1;
            if(data.countdownSuffixZone2) document.getElementById("countdownSuffixZone2").value = data.countdownSuffixZone2;
            if(data.countdownSuffixZone3) document.getElementById("countdownSuffixZone3").value = data.countdownSuffixZone3;
            document.getElementById("countdownShowUnitsZone0").checked = !!data.countdownShowUnitsZone0;
            document.getElementById("countdownShowUnitsZone1").checked = !!data.countdownShowUnitsZone1;
            document.getElementById("countdownShowUnitsZone2").checked = !!data.countdownShowUnitsZone2;
            document.getElementById("countdownShowUnitsZone3").checked = !!data.countdownShowUnitsZone3;
            document.getElementById("charspacingZone0").value = data.charspacingZone0;
            document.getElementById("charspacingZone1").value = data.charspacingZone1;
            document.getElementById("charspacingZone2").value = data.charspacingZone2;
            document.getElementById("charspacingZone3").value = data.charspacingZone3;
            document.getElementById("deviceName").value = data.deviceName;
            document.getElementById("ds18b20UpdateInterval").value = data.ds18b20UpdateInterval;
            document.getElementById("ds18b20UnitsFormat").value = data.ds18b20UnitsFormat;
            
            document.getElementById("disableServiceMessages").checked = data.disableServiceMessages;
            document.getElementById("disableDotsBlink").checked = data.disableDotsBlink;
            document.getElementById("ds18b20Enable").checked = data.ds18b20Enable;
            document.getElementById("ds18b20TempVal").innerText = data.ds18b20Temp;

            // Populate Info section
            document.getElementById("firmwareVer").innerText = data.firmwareVer;
            document.getElementById("platform").innerText = data.platform;
            if(document.getElementById("footerFwVer")) document.getElementById("footerFwVer").innerText = data.firmwareVer;
            document.getElementById("wifiSsid").innerText = data.wifiSsid;
            document.getElementById("wifiIp").innerText = data.wifiIp;
            document.getElementById("wifiGateway").innerText = data.wifiGateway;
            document.getElementById("wifiRssi").innerText = data.wifiRssi + " dBm";
            document.getElementById("mqttDevicePrefixInfo").innerText = data.mqttDevicePrefix;
            document.getElementById("mqttDevicePrefixHelp").innerText = data.mqttDevicePrefix;
            document.getElementById("ds18b20TempInfo").innerText = data.ds18b20Temp;
            
            // Trigger UI updates based on loaded values (Hiding/Showing fields)
            // Need to manually trigger the logic that was previously in the if-blocks
            updateUI(data); 

        })
        .catch(err => {
            console.error('Error loading settings:', err);
            // Optionally show error toast
        });
    }

    function updateUI(data) {
     const zoneNumbers = document.getElementById("zoneNumbers").value;
          if (zoneNumbers == "1") {
        $(document.getElementById("zone1BeginDiv")).hide();
        $(document.getElementById("zone1EndDiv")).hide();
        $(document.getElementById("zone2BeginDiv")).hide();
        $(document.getElementById("zone2EndDiv")).hide();
        $(document.getElementById("zone3BeginDiv")).hide();
        $(document.getElementById("zone3EndDiv")).hide();

        $('#zone1Wrapper').addClass('d-none');
        $('#zone2Wrapper').addClass('d-none');
        $('#zone3WrapperPart1').addClass('d-none');
        $('#zone3ApplyRow').addClass('d-none');
      }
      if (zoneNumbers == "2") {
        $(document.getElementById("zone1BeginDiv")).show();
        $(document.getElementById("zone1EndDiv")).show();
        $(document.getElementById("zone2BeginDiv")).hide();
        $(document.getElementById("zone2EndDiv")).hide();
        $(document.getElementById("zone3BeginDiv")).hide();
        $(document.getElementById("zone3EndDiv")).hide();

        $('#zone1Wrapper').removeClass('d-none');
        $('#zone2Wrapper').addClass('d-none');
        $('#zone3WrapperPart1').addClass('d-none');
        $('#zone3ApplyRow').addClass('d-none');
      }
      if (zoneNumbers == "3") {
        $(document.getElementById("zone1BeginDiv")).show();
        $(document.getElementById("zone1EndDiv")).show();
        $(document.getElementById("zone2BeginDiv")).show();
        $(document.getElementById("zone2EndDiv")).show();
        $(document.getElementById("zone3BeginDiv")).hide();
        $(document.getElementById("zone3EndDiv")).hide();

        $('#zone1Wrapper').removeClass('d-none');
        $('#zone2Wrapper').removeClass('d-none');
        $('#zone3WrapperPart1').addClass('d-none');
        $('#zone3ApplyRow').addClass('d-none');
      }
      if (zoneNumbers == "4") {
        $(document.getElementById("zone1BeginDiv")).show();
        $(document.getElementById("zone1EndDiv")).show();
        $(document.getElementById("zone2BeginDiv")).show();
        $(document.getElementById("zone2EndDiv")).show();
        $(document.getElementById("zone3BeginDiv")).show();
        $(document.getElementById("zone3EndDiv")).show();

        $('#zone1Wrapper').removeClass('d-none');
        $('#zone2Wrapper').removeClass('d-none');
        $('#zone3WrapperPart1').removeClass('d-none');
        $('#zone3ApplyRow').removeClass('d-none');
      }

      function updateZoneFields(zoneIndex) {
          const workMode = document.getElementById("workModeZone" + zoneIndex).value;
          const zoneDivSuffix = "Zone" + zoneIndex + "Div"; // e.g. Zone0Div
          
          if (workMode == "mqttClient") {
            $(document.getElementById("emptyZone" + zoneIndex + "Div")).hide();
            $(document.getElementById("mqttZone" + zoneIndex + "PrefixDiv")).show();
            $(document.getElementById("mqttPostfix" + zoneDivSuffix)).show();
            $(document.getElementById("mqttDevicePrefix" + zoneDivSuffix)).show();
            $(document.getElementById("scrollInfinite" + zoneDivSuffix)).show();
          }
          if (workMode == "owmWeather") {
            $(document.getElementById("emptyZone" + zoneIndex + "Div")).hide();
            $(document.getElementById("owmWhatToDisplayZone" + zoneIndex + "div")).show();
          }
          if (workMode == "wallClock") {
            $(document.getElementById("emptyZone" + zoneIndex + "Div")).hide();
            $(document.getElementById("clockDisplayFormat" + zoneDivSuffix)).show();
            $(document.getElementById("scrollInfinite" + zoneDivSuffix)).show();
            document.getElementById("scrollEffectZone" + zoneIndex + "Out").disabled = true;
          }
          if (workMode == "haClient") {
            $(document.getElementById("emptyZone" + zoneIndex + "Div")).hide();
            $(document.getElementById("haSensorId" + zoneDivSuffix)).show();
            $(document.getElementById("haSensorPostfix" + zoneDivSuffix)).show();
          }
          if (workMode == "intTempSensor") {
            $(document.getElementById("emptyZone" + zoneIndex + "Div")).hide();
            $(document.getElementById("ds18b20Postfix" + zoneDivSuffix)).show();
          }
          if (workMode == "manualInput") {
            $(document.getElementById("emptyZone" + zoneIndex + "Div")).hide();
            $(document.getElementById("scrollInfinite" + zoneDivSuffix)).show();
          }
          if (workMode == "countdown") {
            $(document.getElementById("emptyZone" + zoneIndex + "Div")).hide();
            $(document.getElementById("countdownTargetZone" + zoneIndex + "Div")).show();
            $(document.getElementById("countdownSettingsZone" + zoneIndex + "Div")).show();
            $(document.getElementById("scrollInfinite" + zoneDivSuffix)).show();
            $(document.getElementById("scrollSpeedZone" + zoneIndex + "Div")).show();
            $(document.getElementById("scrollAlignZone" + zoneIndex + "Div")).show();
            document.getElementById("scrollEffectZone" + zoneIndex + "Out").disabled = true;
          }
      }

      updateZoneFields(0);
      updateZoneFields(1);
      updateZoneFields(2);
      updateZoneFields(3);
    }

   
</script>




<script>
workModeZone0.addEventListener('change', function (e) {
      $(document.getElementById("emptyZone0Div")).show();
      $(document.getElementById("mqttZone0PrefixDiv")).hide();
      $(document.getElementById("mqttPostfixZone0Div")).hide();
      $(document.getElementById("scrollInfiniteZone0Div")).hide();
      $(document.getElementById("mqttDevicePrefixZone0Div")).hide();
      $(document.getElementById("scrollPauseZone0Div")).show();
      $(document.getElementById("scrollEffectZone0InDiv")).show();
      $(document.getElementById("scrollEffectZone0OutDiv")).show();
      $(document.getElementById("scrollSpeedZone0Div")).show();
      $(document.getElementById("scrollAlignZone0Div")).show();
      $(document.getElementById("owmWhatToDisplayZone0div")).hide();
      $(document.getElementById("clockDisplayFormatZone0Div")).hide();
      $(document.getElementById("haSensorIdZone0Div")).hide();
      $(document.getElementById("haSensorPostfixZone0Div")).hide();
      $(document.getElementById("ds18b20PostfixZone0Div")).hide();
      $(document.getElementById("countdownTargetZone0Div")).hide();
      $(document.getElementById("countdownSettingsZone0Div")).hide();
      
      if (e.target.value == "mqttClient") {
        $(document.getElementById("emptyZone0Div")).hide();
        $(document.getElementById("mqttZone0PrefixDiv")).show();
        $(document.getElementById("mqttPostfixZone0Div")).show();
        $(document.getElementById("scrollInfiniteZone0Div")).show();
        $(document.getElementById("mqttDevicePrefixZone0Div")).show();
      }
      
      if (e.target.value == "wallClock") {
        $(document.getElementById("emptyZone0Div")).hide();
        $(document.getElementById("clockDisplayFormatZone0Div")).show();
        $(document.getElementById("scrollInfiniteZone0Div")).show();
        document.getElementById("scrollEffectZone0Out").disabled = true;
      }

      if (e.target.value == "owmWeather") {
        $(document.getElementById("emptyZone0Div")).hide();
        $(document.getElementById("owmWhatToDisplayZone0div")).show();
      }

      if (e.target.value == "haClient") {
        $(document.getElementById("emptyZone0Div")).hide();
        $(document.getElementById("haSensorIdZone0Div")).show();
        $(document.getElementById("haSensorPostfixZone0Div")).show();
      }
      if (e.target.value == "intTempSensor") {
        $(document.getElementById("emptyZone0Div")).hide();
        $(document.getElementById("ds18b20PostfixZone0Div")).show();
      }
      if (e.target.value == "manualInput") {
        $(document.getElementById("emptyZone0Div")).hide();
        $(document.getElementById("scrollInfiniteZone0Div")).show();
      }
      if (e.target.value == "countdown") {
        $(document.getElementById("emptyZone0Div")).hide();
        $(document.getElementById("countdownTargetZone0Div")).show();
        $(document.getElementById("countdownSettingsZone0Div")).show();
        $(document.getElementById("scrollInfiniteZone0Div")).show();
        $(document.getElementById("scrollSpeedZone0Div")).show();
        $(document.getElementById("scrollAlignZone0Div")).show();
        document.getElementById("scrollEffectZone0Out").disabled = true;
      }
    });

    workModeZone1.addEventListener('change', function (e) {
      $(document.getElementById("emptyZone1Div")).show();
      $(document.getElementById("mqttZone1PrefixDiv")).hide();
      $(document.getElementById("mqttPostfixZone1Div")).hide();
      $(document.getElementById("scrollInfiniteZone1Div")).hide();
      $(document.getElementById("mqttDevicePrefixZone1Div")).hide();
      $(document.getElementById("scrollPauseZone1Div")).show();
      $(document.getElementById("scrollEffectZone1InDiv")).show();
      $(document.getElementById("scrollEffectZone1OutDiv")).show();
      $(document.getElementById("scrollSpeedZone1Div")).show();
      $(document.getElementById("scrollAlignZone1Div")).show();
      $(document.getElementById("owmWhatToDisplayZone1div")).hide();
      $(document.getElementById("clockDisplayFormatZone1Div")).hide();
      $(document.getElementById("haSensorIdZone1Div")).hide();
      $(document.getElementById("haSensorPostfixZone1Div")).hide();
      $(document.getElementById("ds18b20PostfixZone1Div")).hide();
      $(document.getElementById("countdownTargetZone1Div")).hide();
      $(document.getElementById("countdownSettingsZone1Div")).hide();

      if (e.target.value == "mqttClient") {
        $(document.getElementById("emptyZone1Div")).hide();
        $(document.getElementById("mqttZone1PrefixDiv")).show();
        $(document.getElementById("mqttPostfixZone1Div")).show();
        $(document.getElementById("scrollInfiniteZone1Div")).show();
        $(document.getElementById("mqttDevicePrefixZone1Div")).show();
      }
      
      if (e.target.value == "wallClock") {
        $(document.getElementById("emptyZone1Div")).hide();
        $(document.getElementById("clockDisplayFormatZone1Div")).show();
        $(document.getElementById("scrollInfiniteZone1Div")).show();
        document.getElementById("scrollEffectZone1Out").disabled = true;
      }

      if (e.target.value == "owmWeather") {
        $(document.getElementById("emptyZone1Div")).hide();
        $(document.getElementById("owmWhatToDisplayZone1div")).show();
      }

      if (e.target.value == "haClient") {
        $(document.getElementById("emptyZone1Div")).hide();
        $(document.getElementById("haSensorIdZone1Div")).show();
        $(document.getElementById("haSensorPostfixZone1Div")).show();
      }
      if (e.target.value == "intTempSensor") {
        $(document.getElementById("emptyZone1Div")).hide();
        $(document.getElementById("ds18b20PostfixZone1Div")).show();
      }
      if (e.target.value == "manualInput") {
        $(document.getElementById("emptyZone1Div")).hide();
        $(document.getElementById("scrollInfiniteZone1Div")).show();
      }
      if (e.target.value == "countdown") {
        $(document.getElementById("emptyZone1Div")).hide();
        $(document.getElementById("countdownTargetZone1Div")).show();
        $(document.getElementById("countdownSettingsZone1Div")).show();
        $(document.getElementById("scrollInfiniteZone1Div")).show();
        $(document.getElementById("scrollSpeedZone1Div")).show();
        $(document.getElementById("scrollAlignZone1Div")).show();
        document.getElementById("scrollEffectZone1Out").disabled = true;
      }
    });

    workModeZone2.addEventListener('change', function (e) {
      $(document.getElementById("emptyZone2Div")).show();
      $(document.getElementById("mqttZone2PrefixDiv")).hide();
      $(document.getElementById("mqttPostfixZone2Div")).hide();
      $(document.getElementById("scrollInfiniteZone2Div")).hide();
      $(document.getElementById("mqttDevicePrefixZone2Div")).hide();
      $(document.getElementById("scrollPauseZone2Div")).show();
      $(document.getElementById("scrollEffectZone2InDiv")).show();
      $(document.getElementById("scrollEffectZone2OutDiv")).show();
      $(document.getElementById("scrollSpeedZone2Div")).show();
      $(document.getElementById("scrollAlignZone2Div")).show();
      $(document.getElementById("owmWhatToDisplayZone2div")).hide();
      $(document.getElementById("clockDisplayFormatZone2Div")).hide();
      $(document.getElementById("haSensorIdZone2Div")).hide();
      $(document.getElementById("haSensorPostfixZone2Div")).hide();
      $(document.getElementById("ds18b20PostfixZone2Div")).hide();
      $(document.getElementById("countdownTargetZone2Div")).hide();
      $(document.getElementById("countdownSettingsZone2Div")).hide();
      
      if (e.target.value == "mqttClient") {
        $(document.getElementById("emptyZone2Div")).hide();
        $(document.getElementById("mqttZone2PrefixDiv")).show();
        $(document.getElementById("mqttPostfixZone2Div")).show();
        $(document.getElementById("scrollInfiniteZone2Div")).show();
        $(document.getElementById("mqttDevicePrefixZone2Div")).show();
      }
      
      if (e.target.value == "wallClock") {
        $(document.getElementById("emptyZone2Div")).hide();
        $(document.getElementById("clockDisplayFormatZone2Div")).show();
        $(document.getElementById("scrollInfiniteZone2Div")).show();
        document.getElementById("scrollEffectZone2Out").disabled = true;
      }

      if (e.target.value == "owmWeather") {
        $(document.getElementById("emptyZone2Div")).hide();
        $(document.getElementById("owmWhatToDisplayZone2div")).show();
      }

      if (e.target.value == "haClient") {
        $(document.getElementById("emptyZone2Div")).hide();
        $(document.getElementById("haSensorIdZone2Div")).show();
        $(document.getElementById("haSensorPostfixZone2Div")).show();
      }
      if (e.target.value == "intTempSensor") {
        $(document.getElementById("emptyZone2Div")).hide();
        $(document.getElementById("ds18b20PostfixZone2Div")).show();
      }
      if (e.target.value == "manualInput") {
        $(document.getElementById("emptyZone2Div")).hide();
        $(document.getElementById("scrollInfiniteZone2Div")).show();
      }
      if (e.target.value == "countdown") {
        $(document.getElementById("emptyZone2Div")).hide();
        $(document.getElementById("countdownTargetZone2Div")).show();
        $(document.getElementById("countdownSettingsZone2Div")).show();
        $(document.getElementById("scrollInfiniteZone2Div")).show();
        $(document.getElementById("scrollSpeedZone2Div")).show();
        $(document.getElementById("scrollAlignZone2Div")).show();
        document.getElementById("scrollEffectZone2Out").disabled = true;
      }
    });

    workModeZone3.addEventListener('change', function (e) {
      $(document.getElementById("emptyZone3Div")).show();
      $(document.getElementById("mqttZone3PrefixDiv")).hide();
      $(document.getElementById("mqttPostfixZone3Div")).hide();
      $(document.getElementById("scrollInfiniteZone3Div")).hide();
      $(document.getElementById("mqttDevicePrefixZone3Div")).hide();
      $(document.getElementById("scrollPauseZone3Div")).show();
      $(document.getElementById("scrollEffectZone3InDiv")).show();
      $(document.getElementById("scrollEffectZone3OutDiv")).show();
      $(document.getElementById("scrollSpeedZone3Div")).show();
      $(document.getElementById("scrollAlignZone3Div")).show();
      $(document.getElementById("owmWhatToDisplayZone3div")).hide();
      $(document.getElementById("clockDisplayFormatZone3Div")).hide();
      $(document.getElementById("haSensorIdZone3Div")).hide();
      $(document.getElementById("haSensorPostfixZone3Div")).hide();
      $(document.getElementById("ds18b20PostfixZone3Div")).hide();
      $(document.getElementById("countdownTargetZone3Div")).hide();
      $(document.getElementById("countdownSettingsZone3Div")).hide();

      if (e.target.value == "mqttClient") {
        $(document.getElementById("emptyZone3Div")).hide();
        $(document.getElementById("mqttZone3PrefixDiv")).show();
        $(document.getElementById("mqttPostfixZone3Div")).show();
        $(document.getElementById("scrollInfiniteZone3Div")).show();
        $(document.getElementById("mqttDevicePrefixZone3Div")).show();
      }

      if (e.target.value == "wallClock") {
        $(document.getElementById("emptyZone3Div")).hide();
        $(document.getElementById("clockDisplayFormatZone3Div")).show();
        $(document.getElementById("scrollInfiniteZone3Div")).show();
        document.getElementById("scrollEffectZone3Out").disabled = true;
      }

      if (e.target.value == "owmWeather") {
        $(document.getElementById("emptyZone3Div")).hide();
        $(document.getElementById("owmWhatToDisplayZone3div")).show();
      }

      if (e.target.value == "haClient") {
        $(document.getElementById("emptyZone3Div")).hide();
        $(document.getElementById("haSensorIdZone3Div")).show();
        $(document.getElementById("haSensorPostfixZone3Div")).show();
      }
      if (e.target.value == "intTempSensor") {
        $(document.getElementById("emptyZone3Div")).hide();
        $(document.getElementById("ds18b20PostfixZone3Div")).show();
      }
      if (e.target.value == "manualInput") {
        $(document.getElementById("emptyZone3Div")).hide();
        $(document.getElementById("scrollInfiniteZone3Div")).show();
      }
      if (e.target.value == "countdown") {
        $(document.getElementById("emptyZone3Div")).hide();
        $(document.getElementById("countdownTargetZone3Div")).show();
        $(document.getElementById("countdownSettingsZone3Div")).show();
        $(document.getElementById("scrollInfiniteZone3Div")).show();
        $(document.getElementById("scrollSpeedZone3Div")).show();
        $(document.getElementById("scrollAlignZone3Div")).show();
        document.getElementById("scrollEffectZone3Out").disabled = true;
      }
    });

    document.getElementById("zoneNumbers").addEventListener('change', function (e) {
      if (e.target.value == "1") {
        $(document.getElementById("zone1BeginDiv")).hide();
        $(document.getElementById("zone1EndDiv")).hide();
        $(document.getElementById("zone2BeginDiv")).hide();
        $(document.getElementById("zone2EndDiv")).hide();
        $(document.getElementById("zone3BeginDiv")).hide();
        $(document.getElementById("zone3EndDiv")).hide();

        $('#zone1Wrapper').addClass('d-none');
        $('#zone2Wrapper').addClass('d-none');
        $('#zone3WrapperPart1').addClass('d-none');
        $('#zone3ApplyRow').addClass('d-none');
      }
      if (e.target.value == "2") {
        $(document.getElementById("zone1BeginDiv")).show();
        $(document.getElementById("zone1EndDiv")).show();
        $(document.getElementById("zone2BeginDiv")).hide();
        $(document.getElementById("zone2EndDiv")).hide();
        $(document.getElementById("zone3BeginDiv")).hide();
        $(document.getElementById("zone3EndDiv")).hide();

        $('#zone1Wrapper').removeClass('d-none');
        $('#zone2Wrapper').addClass('d-none');
        $('#zone3WrapperPart1').addClass('d-none');
        $('#zone3ApplyRow').addClass('d-none');
      }
      if (e.target.value == "3") {
        $(document.getElementById("zone1BeginDiv")).show();
        $(document.getElementById("zone1EndDiv")).show();
        $(document.getElementById("zone2BeginDiv")).show();
        $(document.getElementById("zone2EndDiv")).show();
        $(document.getElementById("zone3BeginDiv")).hide();
        $(document.getElementById("zone3EndDiv")).hide();

        $('#zone1Wrapper').removeClass('d-none');
        $('#zone2Wrapper').removeClass('d-none');
        $('#zone3WrapperPart1').addClass('d-none');
        $('#zone3ApplyRow').addClass('d-none');
      }
      if (e.target.value == "4") {
        $(document.getElementById("zone1BeginDiv")).show();
        $(document.getElementById("zone1EndDiv")).show();
        $(document.getElementById("zone2BeginDiv")).show();
        $(document.getElementById("zone2EndDiv")).show();
        $(document.getElementById("zone3BeginDiv")).show();
        $(document.getElementById("zone3EndDiv")).show();

        $('#zone1Wrapper').removeClass('d-none');
        $('#zone2Wrapper').removeClass('d-none');
        $('#zone3WrapperPart1').removeClass('d-none');
        $('#zone3ApplyRow').removeClass('d-none');
      }
    });

    // Toggle scroll speed based on infinite scroll setting in countdown mode
    function checkScrollSpeed(zone) {
      if (document.getElementById("workModeZone" + zone).value == "countdown") {
        document.getElementById("scrollSpeedZone" + zone).disabled = !document.getElementById("scrollInfiniteZone" + zone).checked;
      }
    }
    document.getElementById("scrollInfiniteZone0").addEventListener('change', function() { checkScrollSpeed(0); });
    document.getElementById("scrollInfiniteZone1").addEventListener('change', function() { checkScrollSpeed(1); });
    document.getElementById("scrollInfiniteZone2").addEventListener('change', function() { checkScrollSpeed(2); });
    document.getElementById("scrollInfiniteZone3").addEventListener('change', function() { checkScrollSpeed(3); });

    function preparePostRequest(event, key, val) {
        event.preventDefault(); //This will prevent the default click action
        if (key == "intensity") {
            document.getElementById('intensityValue').innerHTML = val;
            data = {
                key:       "intensity",
                intensity: val
            }
            sendPost(data);
        }

        if (key == "displaySettings") {
            data = {
                key:          "displaySettings",
                zoneNumbers:  document.getElementById("zoneNumbers").value,
                zone0Begin:   document.getElementById("zone0Begin").value,
                zone0End:     document.getElementById("zone0End").value,
                zone1Begin:   document.getElementById("zone1Begin").value,
                zone1End:     document.getElementById("zone1End").value,
                zone2Begin:   document.getElementById("zone2Begin").value,
                zone2End:     document.getElementById("zone2End").value,
                zone3Begin:   document.getElementById("zone3Begin").value,
                zone3End:     document.getElementById("zone3End").value
            }
            sendPost(data);
        }
        
        if (key == "zone0Settings") {
            data = {
                key:                   "zoneSettings",
                zone:                  0,
                workMode:              document.getElementById("workModeZone0").value,
                font:                  document.getElementById("fontZone0").value,
                owmWhatToDisplay:      document.getElementById("owmWhatToDisplayZone0").value,
                haSensorId:            document.getElementById("haSensorIdZone0").value,
                haSensorPostfix:       document.getElementById("haSensorPostfixZone0").value,
                clockDisplayFormat:    document.getElementById("clockDisplayFormatZone0").value,
                scrollSpeed:           document.getElementById("scrollSpeedZone0").value,
                scrollPause:           document.getElementById("scrollPauseZone0").value,
                scrollAlign:           document.getElementById("scrollAlignZone0").value,
                scrollEffectIn:        document.getElementById("scrollEffectZone0In").value,
                scrollEffectOut:       document.getElementById("scrollEffectZone0Out").value,
                charspacing:           document.getElementById("charspacingZone0").value,
                mqttTextTopic:         document.getElementById("mqttTextTopicZone0").value,
                mqttPostfix:           document.getElementById("mqttPostfixZone0").value,
                scrollInfinite:        document.getElementById("scrollInfiniteZone0").checked,
                ds18b20Postfix:        document.getElementById("ds18b20PostfixZone0").value,
                countdownTarget:       document.getElementById("countdownTargetZone0").value,
                countdownFormat:       document.getElementById("countdownFormatZone0").value,
                countdownFinish:       document.getElementById("countdownFinishZone0").value,
                countdownShowUnits:    document.getElementById("countdownShowUnitsZone0").checked,
                countdownPrefix:       document.getElementById("countdownPrefixZone0").value,
                countdownSuffix:       document.getElementById("countdownSuffixZone0").value
                
            }
            sendPost(data);
        }
        
        if (key == "zone1Settings") {
            data = {
                key:                   "zoneSettings",
                zone:                  1,
                workMode:              document.getElementById("workModeZone1").value,
                font:                  document.getElementById("fontZone1").value,                
                owmWhatToDisplay:      document.getElementById("owmWhatToDisplayZone1").value,
                haSensorId:            document.getElementById("haSensorIdZone1").value,
                haSensorPostfix:       document.getElementById("haSensorPostfixZone1").value,
                clockDisplayFormat:    document.getElementById("clockDisplayFormatZone1").value,
                scrollSpeed:           document.getElementById("scrollSpeedZone1").value,
                scrollPause:           document.getElementById("scrollPauseZone1").value,
                scrollAlign:           document.getElementById("scrollAlignZone1").value,
                scrollEffectIn:        document.getElementById("scrollEffectZone1In").value,
                scrollEffectOut:       document.getElementById("scrollEffectZone1Out").value,
                charspacing:           document.getElementById("charspacingZone1").value,
                mqttTextTopic:         document.getElementById("mqttTextTopicZone1").value,
                mqttPostfix:           document.getElementById("mqttPostfixZone1").value,
                scrollInfinite:        document.getElementById("scrollInfiniteZone1").checked,
                ds18b20Postfix:        document.getElementById("ds18b20PostfixZone1").value,
                countdownTarget:       document.getElementById("countdownTargetZone1").value,
                countdownFormat:       document.getElementById("countdownFormatZone1").value,
                countdownFinish:       document.getElementById("countdownFinishZone1").value,
                countdownShowUnits:    document.getElementById("countdownShowUnitsZone1").checked,
                countdownPrefix:       document.getElementById("countdownPrefixZone1").value,
                countdownSuffix:       document.getElementById("countdownSuffixZone1").value
            }
            sendPost(data);
        }

        if (key == "zone2Settings") {
            data = {
                key:                   "zoneSettings",
                zone:                  2,
                workMode:              document.getElementById("workModeZone2").value,
                font:                  document.getElementById("fontZone2").value,
                owmWhatToDisplay:      document.getElementById("owmWhatToDisplayZone2").value,
                haSensorId:            document.getElementById("haSensorIdZone2").value,
                haSensorPostfix:       document.getElementById("haSensorPostfixZone2").value,
                clockDisplayFormat:    document.getElementById("clockDisplayFormatZone2").value,
                scrollSpeed:           document.getElementById("scrollSpeedZone2").value,
                scrollPause:           document.getElementById("scrollPauseZone2").value,
                scrollAlign:           document.getElementById("scrollAlignZone2").value,
                scrollEffectIn:        document.getElementById("scrollEffectZone2In").value,
                scrollEffectOut:       document.getElementById("scrollEffectZone2Out").value,
                charspacing:           document.getElementById("charspacingZone2").value,
                mqttTextTopic:         document.getElementById("mqttTextTopicZone2").value,
                mqttPostfix:           document.getElementById("mqttPostfixZone2").value,
                scrollInfinite:        document.getElementById("scrollInfiniteZone2").checked,
                ds18b20Postfix:        document.getElementById("ds18b20PostfixZone2").value,
                countdownTarget:       document.getElementById("countdownTargetZone2").value,
                countdownFormat:       document.getElementById("countdownFormatZone2").value,
                countdownFinish:       document.getElementById("countdownFinishZone2").value,
                countdownShowUnits:    document.getElementById("countdownShowUnitsZone2").checked,
                countdownPrefix:       document.getElementById("countdownPrefixZone2").value,
                countdownSuffix:       document.getElementById("countdownSuffixZone2").value
            }
            sendPost(data);
        }

        if (key == "zone3Settings") {
            data = {
                key:                   "zoneSettings",
                zone:                  3,
                workMode:              document.getElementById("workModeZone3").value,
                font:                  document.getElementById("fontZone3").value,
                owmWhatToDisplay:      document.getElementById("owmWhatToDisplayZone3").value,
                haSensorId:            document.getElementById("haSensorIdZone3").value,
                haSensorPostfix:       document.getElementById("haSensorPostfixZone3").value,
                clockDisplayFormat:    document.getElementById("clockDisplayFormatZone3").value,
                scrollSpeed:           document.getElementById("scrollSpeedZone3").value,
                scrollPause:           document.getElementById("scrollPauseZone3").value,
                scrollAlign:           document.getElementById("scrollAlignZone3").value,
                scrollEffectIn:        document.getElementById("scrollEffectZone3In").value,
                scrollEffectOut:       document.getElementById("scrollEffectZone3Out").value,
                charspacing:           document.getElementById("charspacingZone3").value,
                mqttTextTopic:         document.getElementById("mqttTextTopicZone3").value,
                mqttPostfix:           document.getElementById("mqttPostfixZone3").value,
                scrollInfinite:        document.getElementById("scrollInfiniteZone3").checked,
                ds18b20Postfix:        document.getElementById("ds18b20PostfixZone3").value,
                countdownTarget:       document.getElementById("countdownTargetZone3").value,
                countdownFormat:       document.getElementById("countdownFormatZone3").value,
                countdownFinish:       document.getElementById("countdownFinishZone3").value,
                countdownShowUnits:    document.getElementById("countdownShowUnitsZone3").checked,
                countdownPrefix:       document.getElementById("countdownPrefixZone3").value,
                countdownSuffix:       document.getElementById("countdownSuffixZone3").value
            }
            sendPost(data);
        }


        if (key == "applyMqttSettings") {
            data = {
                key:                "mqttSettings",
                mqttEnable:         document.querySelector("#mqttEnable").checked,
                mqttServerAddress:  document.getElementById("mqttServerAddress").value,
                mqttServerPort:     document.getElementById("mqttServerPort").value,
                mqttUsername:       document.getElementById("mqttUsername").value,
                mqttPassword:       document.getElementById("mqttPassword").value
            }
            sendPost(data);
        }

        if (key == "applyWallClockSettings") {
            data = {
                key:                    "wallClockSett",
                ntpTimeZone:            document.getElementById("ntpTimeZone").value,
                disableDotsBlink:       document.querySelector('#disableDotsBlink').checked,
                ntpUpdateInterval:      document.getElementById("ntpUpdateInterval").value,
                ntpServer:              document.getElementById("ntpServer").value
            }
            sendPost(data);
        }

        if (key == "applyOwmSettings") {
            data = {
                key:               "owmSettings",
                owmApiToken:       document.getElementById("owmApiToken").value,
                owmUnitsFormat:    document.getElementById("owmUnitsFormat").value,
                owmUpdateInterval: document.getElementById("owmUpdateInterval").value,
                owmCity:           document.getElementById("owmCity").value
            }
            sendPost(data);
        }

        if (key == "applyHaSettings") {
            data = {
                key:                "haSettings",
                haAddr:             document.getElementById("haAddr").value,
                haApiToken:         document.getElementById("haApiToken").value,
                haApiHttpType:      document.getElementById("haApiHttpType").value,
                haApiPort:          document.getElementById("haApiPort").value,
                haUpdateInterval:   document.getElementById("haUpdateInterval").value
            }
            sendPost(data);
        }

        if (key == "applySystemSettings") {
            data = {
                key:                    "systemSettings",
                deviceName:             document.getElementById("deviceName").value,
                disableServiceMessages: document.querySelector('#disableServiceMessages').checked
            }
            sendPost(data);
        }  
        if (key == "applyDs18b20Settings") {
            data = {
                key:                    "ds18b20Settings",
                ds18b20Enable:          document.querySelector('#ds18b20Enable').checked,
                ds18b20UpdateInterval:  document.querySelector('#ds18b20UpdateInterval').value,
                ds18b20UnitsFormat:     document.querySelector('#ds18b20UnitsFormat').value
            }
            sendPost(data);
        }
        

    };

    function sendPost(dataPost) {
        var toastDanger =   new bootstrap.Toast(document.getElementById('liveToastDanger'));
        var toastSuccess =  new bootstrap.Toast(document.getElementById('liveToastSuccess'));

        var request = $.ajax({
        url:      "/api/config",
        method:   "POST",
        cache:    false,
        data:     dataPost,
        dataType: "json"
        });
        
        request.done(function( msg ) {
            toastSuccess.show();            
        });
            
        request.fail(function( jqXHR, textStatus, responseText ) {
            alert( "Request failed: " + responseText );
            toastDanger.show();
        });
    }
    function apiResetWifi() {
      if (confirm("This will reset WiFi settings and reboot. You will need to connect to the AP to reconfigure. Are you sure?")) {
        var request = $.ajax({
          url: "/api/resetWifi",
          method: "POST",
          cache: false
        });
        request.done(function( msg ) {
          // alert("Device is resetting WiFi...");
        });
        request.fail(function( jqXHR, textStatus ) {
          alert( "Request failed: " + textStatus );
        });
      }
    }
    function apiRebootDevice() {
      if (confirm("Are you sure you want to reboot the device?")) {
        var request = $.ajax({
          url: "/api/reboot",
          method: "POST",
          cache: false
        });
        request.done(function( msg ) {
          // alert("Device is rebooting...");
        });
        request.fail(function( jqXHR, textStatus ) {
          alert( "Request failed: " + textStatus );
        });
      }
    }
    function apiFactoryReset() {
      if (confirm("WARNING: This will erase ALL settings (WiFi, MQTT, zones, etc.) and reboot. Are you absolutely sure?")) {
        var request = $.ajax({
          url: "/api/factory-reset",
          method: "POST",
          cache: false
        });
        request.done(function( msg ) {
          alert("Factory reset complete. Device is rebooting...");
        });
        request.fail(function( jqXHR, textStatus ) {
          alert( "Request failed: " + textStatus );
        });
      }
    }
</script>

</body>
</html>)=====";

#endif