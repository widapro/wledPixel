#ifndef htmlFiles_h
#define htmlFiles_h

#include <Arduino.h>

const char PAGE_index[] PROGMEM = R"=====(<!doctype html>
<html lang="en">
  <head>
    <!-- Required meta tags -->
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- Bootstrap CSS -->
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
    <title>Wled-pixel</title>
  </head>
  <body>
      <header>
    <div class="px-3 py-2 bg-dark text-white">
      <div class="container">
        <div class="d-flex flex-wrap align-items-center justify-content-center justify-content-lg-start">
          <a href="/" class="d-flex align-items-center my-2 my-lg-0 me-lg-auto text-white text-decoration-none">
            <h1 class="mb-3 fw-bold font-monospace">Wled pixel</h1>
          </a>

          <ul class="nav col-12 col-lg-auto my-2 justify-content-center my-md-0 text-small">
            <li>
                <span class="nav-link text-secondary">
                  Home
                </span>
              </li>
              <li>
                <a href="/settings" class="nav-link text-white">
                  Settings
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
            <h2>Wled pixel dot led Display</h2>
        </div>
        <div class="row g-5">
            <div class="col-md-5 col-lg-4 order-md-last">
              <h4 class="d-flex justify-content-between align-items-center mb-3">
                <span class="text-primary">Status</span>
              </h4>
              <ul class="list-group mb-3">
                <li class="list-group-item d-flex justify-content-between lh-sm">
                  <div>
                    <h6 class="my-0">Wled Pixel</h6>
                    <small class="text-muted">Wled pixel dot led Display</small>
                  </div>
                  <span class="text-muted">v2</span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi SSID:</h6>
                    </div>
                    <span class="text-muted">%wifiSsid%</span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi IP:</h6>
                    </div>
                    <span class="text-muted">%wifiIp%</span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi Gateway:</h6>
                    </div>
                    <span class="text-muted">%wifiGateway%</span>
                </li>
              </ul>
              <ul class="list-group mb-3">
                <li class="list-group-item d-flex justify-content-between lh-sm">
                  <div>
                    <h7 class="my-0">Zone0</h7>
                    <small class="text-muted">Work mode</small>
                  </div>
                  <span class="text-muted">%workModeZone0%</span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h7 class="my-0">Zone1</h7>
                      <small class="text-muted">Work mode</small>
                    </div>
                    <span class="text-muted">%workModeZone1%</span>
                </li>
              </ul>
            </div>
            <div class="col-md-7 col-lg-8">
              <h4 class="mb-3">Manual text sender</h4>
              <span class="text-secondary">If work mode is configured to <mark>"Manual input"</mark> you can send a message here. If you select 
                <strong>"Scroll effect OUT"</strong> to <strong>"NO_EFFECT"</strong> the message will stay on display until a new message will be sent or the device will be rebooted.</span>

              <hr class="my-5">

              
                <div class="row g-3">
                  <div class="col-10">
                    <label for="messageZone0" class="form-label">Zone0 text</label>
                    <input id="messageZone0" class="form-control form-control-lg" type="text" placeholder="Zone0 message">
                  </div>
                  <div class="col-2 align-self-end">
                    <button id="postZone0Text" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Send</button>
                  </div>
                  <div class="col-sm-12"></div>
                  <div class="col-10">
                    <label for="messageZone1" class="form-label">Zone1 text</label>
                    <input id="messageZone1" class="form-control form-control-lg" type="text" placeholder="Zone1 message">
                  </div>
                  <div class="col-2 align-self-end">
                    <button id="postZone1Text" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Send</button>
                  </div>
                  <div class="col-sm-12"></div>
                  <div class="col-10">
                    <label for="messageZone2" class="form-label">Zone2 text</label>
                    <input id="messageZone2" class="form-control form-control-lg" type="text" placeholder="Zone2 message">
                  </div>
                  <div class="col-2 align-self-end">
                    <button id="postZone2Text" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Send</button>
                  </div>
                <div class="col-sm-12"></div>
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


</body>
</html>

<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
<script>
function preparePostRequest(event, key, val) {
    event.preventDefault(); //This will prevent the default click action

    if (key == "postZone0Text") {
        data = {
            messageZone0: document.getElementById("messageZone0").value,
        }
        sendPost(data);
    };

    if (key == "postZone1Text") {
        data = {
            messageZone1: document.getElementById("messageZone1").value,
        }
        sendPost(data);
    };

    if (key == "postZone2Text") {
        data = {
            messageZone2: document.getElementById("messageZone2").value,
        }
        sendPost(data);
    };
};

    function sendPost(dataPost) {
        var toastDanger = new bootstrap.Toast(document.getElementById('liveToastDanger'));
        var toastSuccess = new bootstrap.Toast(document.getElementById('liveToastSuccess'));

        var request = $.ajax({
            url: "/configpost",
            method: "POST",
            cache: false,
            data: dataPost,
            dataType: "json"
        });
        
        request.done(function( msg ) {
            toastSuccess.show();            
        });
            
        request.fail(function( jqXHR, textStatus, responseText ) {
            toastDanger.show();
        });
    }

    const zoneNumbers = %zoneNumbers%;
    const workModeZone0 = "%workModeZone0%"
    const workModeZone1 = "%workModeZone1%"
    const workModeZone2 = "%workModeZone2%"
    if (workModeZone1 != "manualInput") {
        document.getElementById('messageZone1').disabled = true;
        document.getElementById('postZone1Text').disabled = true;
        document.getElementById('messageZone1').placeholder = "Zone1 is not in a Manual input work mode";
    }
    if (workModeZone0 != "manualInput") {
        document.getElementById('messageZone0').disabled = true;
        document.getElementById('postZone0Text').disabled = true;
        document.getElementById('messageZone0').placeholder = "Zone0 is not in a Manual input work mode";
    }
    if (workModeZone2 != "manualInput") {
        document.getElementById('messageZone2').disabled = true;
        document.getElementById('postZone2Text').disabled = true;
        document.getElementById('messageZone2').placeholder = "Zone2 is not in a Manual input work mode";
    }
    if (zoneNumbers == 1) {
        document.getElementById('messageZone1').disabled = true;
        document.getElementById('postZone1Text').disabled = true;
        document.getElementById('messageZone1').placeholder = "Zone1 is not initialized";

        document.getElementById('messageZone2').disabled = true;
        document.getElementById('postZone2Text').disabled = true;
        document.getElementById('messageZone2').placeholder = "Zone2 is not initialized";
    }
   
</script>
</body>
</html>)=====";






const char PAGE_settings[] PROGMEM = R"=====(<!doctype html>
<html lang="en">
  <head>
    <!-- Required meta tags -->
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- Bootstrap CSS -->
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3" crossorigin="anonymous">
    <title>Wled-pixel</title>
  </head>
  <body>
      <header>
    <div class="px-3 py-2 bg-dark text-white">
      <div class="container">
        <div class="d-flex flex-wrap align-items-center justify-content-center justify-content-lg-start">
          <a href="/" class="d-flex align-items-center my-2 my-lg-0 me-lg-auto text-white text-decoration-none">
            <h1 class="mb-3 fw-bold font-monospace">Wled pixel</h1>
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
                    <h6 class="my-0">Wled Pixel</h6>
                    <small class="text-muted">Wled pixel dot led Display</small>
                  </div>
                  <span class="text-muted">v2</span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi SSID:</h6>
                    </div>
                    <span class="text-muted">%wifiSsid%</span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi IP:</h6>
                    </div>
                    <span class="text-muted">%wifiIp%</span>
                </li>
                <li class="list-group-item d-flex justify-content-between lh-sm">
                    <div>
                      <h6 class="my-0">Wifi Gateway:</h6>
                    </div>
                    <span class="text-muted">%wifiGateway%</span>
                </li>
              </ul>
      
            </div>
            <div class="col-md-7 col-lg-8">
              <h4 class="mb-3">Display settings <span class="text-primary fs-6" style="--bs-text-opacity: .5;">[reboot required]</span></h4>
              <form class="needs-validation" novalidate="">
                <div class="row g-3">
                  <div class="col-sm-4">
                    <label for="Numbers of zones" class="form-label">Number of zones</label>
                    <select class="form-select" aria-label="Default select example" id="zoneNumbers">
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                      </select>
                    <div class="invalid-feedback">
                      Valid first name is required.
                    </div>
                  </div>
      
                  <div class="col-sm-4">
                    <label for="zone0Begin" class="form-label">First module <span class="text-muted">[ zone 0 ]</span></label>
                    <input type="text" class="form-control" id="zone0Begin" value="%zone0Begin%" >
                  </div>
                  <div class="col-sm-4">
                    <label for="zone0End" class="form-label">Last module <span class="text-muted">[ zone 0 ]</span></label>
                    <input type="text" class="form-control" id="zone0End" value="%zone0End%" >
                  </div>
                  <div class="col-sm-4"></div>
                  <div class="col-sm-4" id="zone1BeginDiv">
                    <label for="zone1Begin" class="form-label">First module <span class="text-muted">[ zone 1 ]</span></label>
                    <input type="text" class="form-control" id="zone1Begin" value="%zone1Begin%" >
                  </div>
                  <div class="col-sm-4" id="zone1EndDiv">
                    <label for="zone1End" class="form-label">Last module <span class="text-muted">[ zone 1 ]</span></label>
                    <input type="text" class="form-control" id="zone1End" value="%zone1End%" >
                  </div>
                  <div class="col-sm-4"></div>
                  <div class="col-sm-4" id="zone2BeginDiv">
                    <label for="zone2Begin" class="form-label">First module <span class="text-muted">[ zone 2 ]</span></label>
                    <input type="text" class="form-control" id="zone2Begin" value="%zone2Begin%" >
                  </div>
                  <div class="col-sm-4" id="zone2EndDiv">
                    <label for="zone2End" class="form-label">Last module <span class="text-muted">[ zone 2 ]</span></label>
                    <input type="text" class="form-control" id="zone2End" value="%zone2End%" >
                  </div>
                  <button id="applyAndReboot" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply and reboot</button>
                  
                  <hr class="my-4">

                  <div class="col-11">
                    <label for="intensity" class="form-label">Brightness</label>
                    <input type="range" class="form-range" min="0" max="15" step="1" id="intensity" value="%intensity%" onChange="preparePostRequest(event, this.id, this.value);">
                  </div>
                  <div class="col-1 align-self-end">
                    <label id="intensityValue" for="intensity" class="form-label text-primary">%intensity%</label>
                  </div>

                  <hr class="my-5">

                  <h3 class="mb-3">Zone 0</h3>

                  <div class="col-6">
                    <label for="workModeZone0" class="form-label">Working mode</label>
                    <select id="workModeZone0" class="form-select">
                        <option value="mqttClient">MQTT client</option>
                        <option value="manualInput">Manual input</option>
                        <option value="wallClock">Wall clock</option>
                        <option value="owmWeather">Open Weather map</option>
                    </select>
                  </div>
      
                  <div class="col-6">
                    <div class="form-group">
                      <label for="scrollSpeedZone0" class="form-label">Scroll speed</label>
                      <div class="input-group mb-3">
                          <input type="text" class="form-control" id="scrollSpeedZone0" value="%scrollSpeedZone0%" aria-describedby="scrollSpeedZone0Help">
                          <span class="input-group-text">ms</span>
                      </div>
                      <small id="scrollSpeedZone0Help" class="form-text text-muted">Value between 1-255</small>
                    </div>
                  </div>

                  <div class="col-12" id="owmWhatToDisplayZone0div">
                    <label for="owmWhatToDisplayZone0" class="form-label">What to display</label>
                    <select id="owmWhatToDisplayZone0" class="form-select">
                        <option value="owmTemperature">Temperature</option>
                        <option value="owmHumidity">Humidity</option>
                        <option value="owmPressure">Pressure</option>
                        <option value="owmWindSpeed">Wind speed</option>
                        <option value="owmWeatherIcon">Wether icon</option>
                    </select>
                  </div>

                  <div class="col-6">
                    <label for="scrollAlignZone0" class="form-label">Alignment</label>
                    <select id="scrollAlignZone0" class="form-select">
                        <option value="PA_LEFT">Left</option>
                        <option value="PA_CENTER">Center</option>
                        <option value="PA_RIGHT">Right</option>
                      </select>
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollEffectZone0In" class="form-label">Scroll effect <span class="text-primary"><b>IN</b></span></label>
                    <select id="scrollEffectZone0In" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" >SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PA_SPRITE">SPRITE</option>
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
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollPauseZone0" class="form-label">Scroll pause</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollPauseZone0" value="%scrollPauseZone0%">
                        <span class="input-group-text">ms</span>
                    </div>
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollEffectZone0Out" class="form-label">Scroll effect <span class="text-primary"><b>OUT</b></span></label>
                    <select id="scrollEffectZone0Out" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" selected>SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PA_SPRITE">SPRITE</option>
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
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <button id="applyAdditionalSettingsZone0" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>

                  <hr class="my-4">
                  <h3 class="mb-3">Zone 1</h3>

                  <div class="col-6">
                    <label for="workModeZone1" class="form-label">Working mode</label>
                    <select id="workModeZone1" class="form-select">
                        <option value="mqttClient">MQTT client</option>
                        <option value="manualInput">Manual input</option>
                        <option value="wallClock">Wall clock</option>
                        <option value="owmWeather">Open Weather map</option>
                    </select>
                  </div>
      
                  <div class="col-6">
                    <label for="scrollSpeedZone1" class="form-label">Scroll speed</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollSpeedZone1" value="%scrollSpeedZone1%">
                        <span class="input-group-text">ms</span>
                    </div>
                  </div>

                  <div class="col-12" id="owmWhatToDisplayZone1div">
                    <label for="owmWhatToDisplayZone1" class="form-label">What to display</label>
                    <select id="owmWhatToDisplayZone1" class="form-select">
                        <option value="owmTemperature">Temperature</option>
                        <option value="owmHumidity">Humidity</option>
                        <option value="owmPressure">Pressure</option>
                        <option value="owmWindSpeed">Wind speed</option>
                        <option value="owmWeatherIcon">Wether icon</option>
                    </select>
                  </div>

                  <div class="col-6">
                    <label for="scrollAlignZone1" class="form-label">Alignment</label>
                    <select id="scrollAlignZone1" class="form-select">
                        <option value="PA_LEFT">Left</option>
                        <option value="PA_CENTER">Center</option>
                        <option value="PA_RIGHT">Right</option>
                      </select>
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollEffectZone1In" class="form-label">Scroll effect <span class="text-primary"><b>IN</b></span></label>
                    <select id="scrollEffectZone1In" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" >SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PA_SPRITE">SPRITE</option>
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
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollPauseZone1" class="form-label">Scroll pause</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollPauseZone1" value="%scrollPauseZone1%">
                        <span class="input-group-text">ms</span>
                    </div>
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollEffectZone1Out" class="form-label">Scroll effect <span class="text-primary"><b>OUT</b></span></label>
                    <select id="scrollEffectZone1Out" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" selected>SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PA_SPRITE">SPRITE</option>
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
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>
      
                <button id="applyAdditionalSettingsZone1" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>


                <hr class="my-4">
                  <h3 class="mb-3">Zone 2</h3>

                  <div class="col-6">
                    <label for="workModeZone2" class="form-label">Working mode</label>
                    <select id="workModeZone2" class="form-select">
                        <option value="mqttClient">MQTT client</option>
                        <option value="manualInput">Manual input</option>
                        <option value="wallClock">Wall clock</option>
                        <option value="owmWeather">Open Weather map</option>
                    </select>
                  </div>
      
                  <div class="col-6">
                    <label for="scrollSpeedZone2" class="form-label">Scroll speed</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollSpeedZone2" value="%scrollSpeedZone2%">
                        <span class="input-group-text">ms</span>
                    </div>
                  </div>

                  <div class="col-12" id="owmWhatToDisplayZone2div">
                    <label for="owmWhatToDisplayZone2" class="form-label">What to display</label>
                    <select id="owmWhatToDisplayZone2" class="form-select">
                        <option value="owmTemperature">Temperature</option>
                        <option value="owmHumidity">Humidity</option>
                        <option value="owmPressure">Pressure</option>
                        <option value="owmWindSpeed">Wind speed</option>
                        <option value="owmWeatherIcon">Wether icon</option>
                    </select>
                  </div>

                  <div class="col-6">
                    <label for="scrollAlignZone2" class="form-label">Alignment</label>
                    <select id="scrollAlignZone2" class="form-select">
                        <option value="PA_LEFT">Left</option>
                        <option value="PA_CENTER">Center</option>
                        <option value="PA_RIGHT">Right</option>
                      </select>
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollEffectZone2In" class="form-label">Scroll effect <span class="text-primary"><b>IN</b></span></label>
                    <select id="scrollEffectZone2In" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" >SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PA_SPRITE">SPRITE</option>
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
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollPauseZone2" class="form-label">Scroll pause</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="scrollPauseZone2" value="%scrollPauseZone2%">
                        <span class="input-group-text">ms</span>
                    </div>
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="scrollEffectZone2Out" class="form-label">Scroll effect <span class="text-primary"><b>OUT</b></span></label>
                    <select id="scrollEffectZone2Out" class="form-select">
                        <option value="PA_NO_EFFECT">NO_EFFECT</option>
                        <option value="PA_PRINT">PRINT</option>
                        <option value="PA_SCROLL_UP">SCROLL_UP</option>
                        <option value="PA_SCROLL_DOWN">SCROLL_DOWN</option>
                        <option value="PA_SCROLL_LEFT" selected>SCROLL_LEFT</option>
                        <option value="PA_SCROLL_RIGHT">SCROLL_RIGHT</option>
                        <option value="PA_SPRITE">SPRITE</option>
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
                    <div class="invalid-feedback">
                      Please enter a numbers only.
                    </div>
                  </div>
      
                <button id="applyAdditionalSettingsZone2" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>


                <hr class="my-4">
                  <h3 class="mb-3">MQTT settings</h3>

                  <div class="col-6">
                    <label for="mqttServerAddress" class="form-label">Server address</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttServerAddress" value="%mqttServerAddress%">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttServerPort" class="form-label">Server port</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttServerPort" value="%mqttServerPort%">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttUsername" class="form-label">Username</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttUsername" value="%mqttUsername%">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttPassword" class="form-label">Password</label>
                    <div class="input-group mb-3">
                        <input type="password" class="form-control" id="mqttPassword" value="%mqttPassword%">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttZone0Topic" class="form-label">Zone0 MQTT topic</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttZone0Topic" value="%mqttZone0Topic%">
                    </div>
                  </div>
                  
                  <div class="col-6">
                    <label for="mqttZone1Topic" class="form-label">Zone1 MQTT topic</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttZone1Topic" value="%mqttZone1Topic%">
                    </div>
                  </div>

                  <div class="col-6">
                    <label for="mqttZone2Topic" class="form-label">Zone2 MQTT topic</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="mqttZone2Topic" value="%mqttZone2Topic%">
                    </div>
                  </div>

                <button id="applyMqttSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>

                
                <hr class="my-4">
                  <h3 class="mb-3">Wall clock settings</h3>                 
                  <div class="col-3">
                    <label for="timeZone" class="form-label">Time zone</label>
                    <div class="input-group mb-3">
                        <span class="input-group-text">UTC</span>
                        <input type="text" class="form-control" id="ntpTimeZone" value="%ntpTimeZone%">
                    </div>
                  </div>
                  <div class="col-4">
                    <label for="clockDisplayUpdateTime" class="form-label">Display refresh interval</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="clockDisplayUpdateTime" value="%clockDisplayUpdateTime%">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>
                  <div class="col-4">
                    <label for="clockDisplayFormat" class="form-label">Time format</label>
                    <div class="input-group mb-4">
                        <select id="clockDisplayFormat" class="form-select">
                          <option value="HHMM">HH:MM</option>
                          <option value="HHMMSS">HH:MM:SS</option>
                          <option value="HH">HH</option>
                          <option value="MM">MM</option>
                        </select>
                    </div>
                  </div>
                  

                <button id="applyWallClockSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>


                <hr class="my-4">
                  <h3 class="mb-4">Open Weather map settings</h3>                 
                  <div class="col-4">
                    <label for="owmCity" class="form-label">City</label>
                    <div class="input-group mb-4">
                        <input type="text" class="form-control" id="owmCity" value="%owmCity%">
                    </div>
                  </div>
                  <div class="col-8">
                    <label for="owmApiToken" class="form-label">API token</label>
                    <div class="input-group mb-4">
                        <input type="password" class="form-control" id="owmApiToken" value="%owmApiToken%">
                    </div>
                  </div>
                  <div class="col-3">
                    <label for="clockDisplayUpdateTime" class="form-label">Update interval</label>
                    <div class="input-group mb-3">
                        <input type="text" class="form-control" id="owmUpdateInterval" value="%owmUpdateInterval%">
                        <span class="input-group-text">s</span>
                    </div>
                  </div>
                  <div class="col-3">
                    <label for="clockDisplayFormat" class="form-label">Units format</label>
                    <div class="input-group mb-3">
                        <select id="owmUnitsFormat" class="form-select">
                          <option value="metric">metric</option>
                          <option value="imperial">imperial</option>
                        </select>
                    </div>
                  </div>
                  

                <button id="applyOwmSettings" class="w-100 btn btn-primary btn-lg" onClick="preparePostRequest(event, this.id, null);">Apply</button>

                <div class="col-sm-12"></div>
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


</body>
</html>

<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
<script>
    window.onload = document.getElementById("workModeZone0").value = "%workModeZone0%";
    window.onload = document.getElementById("workModeZone1").value = "%workModeZone1%";
    window.onload = document.getElementById("workModeZone2").value = "%workModeZone2%";

    window.onload = document.getElementById("zoneNumbers").value = "%zoneNumbers%";
    window.onload = document.getElementById("zone0Begin").value = "%zone0Begin%";
    window.onload = document.getElementById("zone0End").value = "%zone0End%";
    window.onload = document.getElementById("zone1Begin").value = "%zone1Begin%";
    window.onload = document.getElementById("zone1End").value = "%zone1End%";
    window.onload = document.getElementById("zone2Begin").value = "%zone2Begin%";
    window.onload = document.getElementById("zone2End").value = "%zone2End%";

    window.onload = document.getElementById("intensity").value = "%intensity%";
    window.onload = document.getElementById("intensityValue").value = "%intensity%";

    window.onload = document.getElementById("scrollSpeedZone0").value = "%scrollSpeedZone0%";
    window.onload = document.getElementById("scrollSpeedZone1").value = "%scrollSpeedZone1%";
    window.onload = document.getElementById("scrollSpeedZone2").value = "%scrollSpeedZone2%";
    window.onload = document.getElementById("scrollPauseZone0").value = "%scrollPauseZone0%";
    window.onload = document.getElementById("scrollPauseZone1").value = "%scrollPauseZone1%";
    window.onload = document.getElementById("scrollPauseZone2").value = "%scrollPauseZone2%";
    window.onload = document.getElementById("scrollAlignZone0").value = "%scrollAlignZone0%";
    window.onload = document.getElementById("scrollAlignZone1").value = "%scrollAlignZone1%";
    window.onload = document.getElementById("scrollAlignZone2").value = "%scrollAlignZone2%";
    
    window.onload = document.getElementById("scrollEffectZone0In").value = "%scrollEffectZone0In%";
    window.onload = document.getElementById("scrollEffectZone0Out").value = "%scrollEffectZone0Out%";
    window.onload = document.getElementById("scrollEffectZone1In").value = "%scrollEffectZone1In%";
    window.onload = document.getElementById("scrollEffectZone1Out").value = "%scrollEffectZone1Out%";
    window.onload = document.getElementById("scrollEffectZone2In").value = "%scrollEffectZone2In%";
    window.onload = document.getElementById("scrollEffectZone2Out").value = "%scrollEffectZone2Out%";

    window.onload = document.getElementById("mqttServerAddress").value = "%mqttServerAddress%";
    window.onload = document.getElementById("mqttServerPort").value = "%mqttServerPort%";
    window.onload = document.getElementById("mqttUsername").value = "%mqttUsername%";
    window.onload = document.getElementById("mqttZone0Topic").value = "%mqttZone0Topic%";
    window.onload = document.getElementById("mqttZone1Topic").value = "%mqttZone1Topic%";
    window.onload = document.getElementById("mqttZone2Topic").value = "%mqttZone2Topic%";
    window.onload = document.getElementById("ntpTimeZone").value = "%ntpTimeZone%";
    window.onload = document.getElementById("clockDisplayUpdateTime").value = "%clockDisplayUpdateTime%";
    window.onload = document.getElementById("clockDisplayFormat").value = "%clockDisplayFormat%";

    window.onload = document.getElementById("owmWhatToDisplayZone0").value = "%owmWhatToDisplayZone0%";
    window.onload = document.getElementById("owmWhatToDisplayZone1").value = "%owmWhatToDisplayZone1%";
    window.onload = document.getElementById("owmWhatToDisplayZone2").value = "%owmWhatToDisplayZone2%";
    window.onload = document.getElementById("owmApiToken").value = "%owmApiToken%";
    window.onload = document.getElementById("owmUnitsFormat").value = "%owmUnitsFormat%";
    window.onload = document.getElementById("owmUpdateInterval").value = "%owmUpdateInterval%";
    window.onload = document.getElementById("owmCity").value = "%owmCity%";

    const workModeZone0FirstValue = "%workModeZone0%";
    if (workModeZone0FirstValue == "wallClock") {
      document.getElementById("scrollPauseZone0").disabled = true;
    }
    if (workModeZone0FirstValue != "owmWeather") {
      $(document.getElementById("owmWhatToDisplayZone0div")).hide();
    }

    workModeZone0.addEventListener('change', function (e) {  
      if (e.target.value == "wallClock") {
        document.getElementById("scrollPauseZone0").disabled = true;
      }
      if (e.target.value != "wallClock") {
        document.getElementById("scrollPauseZone0").disabled = false;
      }
      if (e.target.value == "owmWeather") {
        $(document.getElementById("owmWhatToDisplayZone0div")).show();
      }
      if (e.target.value != "owmWeather") {
        $(document.getElementById("owmWhatToDisplayZone0div")).hide();
      }
    });
       
    const workModeZone1FirstValue = "%workModeZone1%";
    if (workModeZone1FirstValue == "wallClock") {
      document.getElementById("scrollPauseZone1").disabled = true;
    }
    if (workModeZone1FirstValue != "owmWeather") {
      $(document.getElementById("owmWhatToDisplayZone1div")).hide();
    }

    workModeZone1.addEventListener('change', function (e) {  
      if (e.target.value == "wallClock") {
        document.getElementById("scrollPauseZone1").disabled = true;
      }
      if (e.target.value != "wallClock") {
        document.getElementById("scrollPauseZone1").disabled = false;
      }
      if (e.target.value == "owmWeather") {
        $(document.getElementById("owmWhatToDisplayZone1div")).show();
      }
      if (e.target.value != "owmWeather") {
        $(document.getElementById("owmWhatToDisplayZone1div")).hide();
      }
    });

    const workModeZone2FirstValue = "%workModeZone2%";
    if (workModeZone2FirstValue == "wallClock") {
      document.getElementById("scrollPauseZone2").disabled = true;
    }
    if (workModeZone2FirstValue != "owmWeather") {
      $(document.getElementById("owmWhatToDisplayZone2div")).hide();
    }

    workModeZone2.addEventListener('change', function (e) {  
      if (e.target.value == "wallClock") {
        document.getElementById("scrollPauseZone2").disabled = true;
      }
      if (e.target.value != "wallClock") {
        document.getElementById("scrollPauseZone2").disabled = false;
      }
      if (e.target.value == "owmWeather") {
        $(document.getElementById("owmWhatToDisplayZone2div")).show();
      }
      if (e.target.value != "owmWeather") {
        $(document.getElementById("owmWhatToDisplayZone2div")).hide();
      }
    });

//      document.getElementById("zoneNumbers").addEventListener('change', function (e) {
//        if (e.target.value == "1") {
//          $(document.getElementById("zone1Begin")).hide();
//          $(document.getElementById("zone1End")).hide();
//          $(document.getElementById("zone2Begin")).hide();
//          $(document.getElementById("zone2End")).hide();
//        }
//        if (e.target.value == "2") {
//          $(document.getElementById("zone1Begin")).show();
//          $(document.getElementById("zone1End")).show();
//          $(document.getElementById("zone2Begin")).hide();
//          $(document.getElementById("zone2End")).hide();
//        }
//        if (e.target.value == "3") {
//          $(document.getElementById("zone1Begin")).show();
//          $(document.getElementById("zone1End")).show();
//          $(document.getElementById("zone2Begin")).show();
//          $(document.getElementById("zone2End")).show();
//        }
//      });
//    //}

    function preparePostRequest(event, key, val) {
        event.preventDefault(); //This will prevent the default click action
        if (key == "intensity") {
            document.getElementById('intensityValue').innerHTML = val;
            data = {intensity: val};
            sendPost(data);
        }

        if (key == "applyAndReboot") {
            data = {
                zoneNumbers:  document.getElementById("zoneNumbers").value,
                zone0Begin:   document.getElementById("zone0Begin").value,
                zone0End:     document.getElementById("zone0End").value,
                zone1Begin:   document.getElementById("zone1Begin").value,
                zone1End:     document.getElementById("zone1End").value,
                zone2Begin:   document.getElementById("zone2Begin").value,
                zone2End:     document.getElementById("zone2End").value
            }
            sendPost(data);
        }
        
        if (key == "applyAdditionalSettingsZone0") {
            data = {
                workModeZone0:              document.getElementById("workModeZone0").value,
                scrollSpeedZone0:           document.getElementById("scrollSpeedZone0").value,
                scrollPauseZone0:           document.getElementById("scrollPauseZone0").value,
                scrollAlignZone0:           document.getElementById("scrollAlignZone0").value,
                scrollEffectZone0In:        document.getElementById("scrollEffectZone0In").value,
                scrollEffectZone0Out:       document.getElementById("scrollEffectZone0Out").value,
                owmWhatToDisplayZone0:      document.getElementById("owmWhatToDisplayZone0").value
            }
            sendPost(data);
        }
        
        if (key == "applyAdditionalSettingsZone1") {
            data = {
                workModeZone1:              document.getElementById("workModeZone1").value,
                scrollSpeedZone1:           document.getElementById("scrollSpeedZone1").value,
                scrollPauseZone1:           document.getElementById("scrollPauseZone1").value,
                scrollAlignZone1:           document.getElementById("scrollAlignZone1").value,
                scrollEffectZone1In:        document.getElementById("scrollEffectZone1In").value,
                scrollEffectZone1Out:       document.getElementById("scrollEffectZone1Out").value,
                owmWhatToDisplayZone1:      document.getElementById("owmWhatToDisplayZone1").value
            }
            sendPost(data);
        }

        if (key == "applyAdditionalSettingsZone2") {
            data = {
                workModeZone2:              document.getElementById("workModeZone2").value,
                scrollSpeedZone2:           document.getElementById("scrollSpeedZone2").value,
                scrollPauseZone2:           document.getElementById("scrollPauseZone2").value,
                scrollAlignZone2:           document.getElementById("scrollAlignZone2").value,
                scrollEffectZone2In:        document.getElementById("scrollEffectZone2In").value,
                scrollEffectZone2Out:       document.getElementById("scrollEffectZone2Out").value,
                owmWhatToDisplayZone2:      document.getElementById("owmWhatToDisplayZone2").value
            }
            sendPost(data);
        }

        if (key == "applyMqttSettings") {
            data = {
                mqttServerAddress:  document.getElementById("mqttServerAddress").value,
                mqttServerPort:     document.getElementById("mqttServerPort").value,
                mqttUsername:       document.getElementById("mqttUsername").value,
                mqttPassword:       document.getElementById("mqttPassword").value,
                mqttZone0Topic:     document.getElementById("mqttZone0Topic").value,
                mqttZone1Topic:     document.getElementById("mqttZone1Topic").value,
                mqttZone2Topic:     document.getElementById("mqttZone2Topic").value
            }
            sendPost(data);
        }

        if (key == "applyWallClockSettings") {
            data = {
                ntpTimeZone:            document.getElementById("ntpTimeZone").value,
                clockDisplayUpdateTime: document.getElementById("clockDisplayUpdateTime").value,
                clockDisplayFormat:     document.getElementById("clockDisplayFormat").value
            }
            sendPost(data);
        }

        if (key == "applyOwmSettings") {
            data = {
                owmApiToken:       document.getElementById("owmApiToken").value,
                owmUnitsFormat:    document.getElementById("owmUnitsFormat").value,
                owmUpdateInterval: document.getElementById("owmUpdateInterval").value,
                owmCity:           document.getElementById("owmCity").value
            }
            sendPost(data);
        }        

    };

    function sendPost(dataPost) {
        var toastDanger =   new bootstrap.Toast(document.getElementById('liveToastDanger'));
        var toastSuccess =  new bootstrap.Toast(document.getElementById('liveToastSuccess'));

        var request = $.ajax({
        url:      "/configpost",
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
</script>
</body>
</html>)=====";


#endif