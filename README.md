# wledPixel
![img](https://raw.githubusercontent.com/widapro/wled-pixel-v2/master/img/wledPixel_logoPicS.jpeg)

MAX7219 LED dot matrix display driven by an ESP32 or ESP8266 MCU.

<a href="https://github.com/widapro/wled-pixel-v2/releases" target="_blank">The latest firmware version can be found in Releases</a>


## Supported key features:
1. **Home Assistant client** [display sensor values]
2. **OpenWeatherMap client** [display: temperature (C/F), humidity, pressure, wind speed, weather icon]
3. **MQTT client** [each display zone support own topic]
4. Display controll through MQTT
5. **Wall NTP clock** [NTP sync clock]
6. **Manual input**
7. Full controll through **web UI**
8. Home Assistant MQTT discovery [When MQTT settings specified, device will be automatically send discovery message to HA]
9. Control display as light in Home Assistant [MQTT setup required]
10. Initial setup through wifi AP and web UI
11. Support 4 independent display zones
12. Support up to 12 display segments at the same time [configured in web UI]
13. Included 3 different fonts
14. Plus symbols font

##### Ingredients:
1. Dot matrix display MAX7219, something like this: <a href="https://aliexpress.com/item/32618155357.html" target="_blank">https://aliexpress.com/item/32618155357.html</a>
2. ESP32 or WeMos D1 Mini (ESP8266), something like this: <a href="https://aliexpress.com/item/32651747570.html" target="_blank">https://aliexpress.com/item/32651747570.html</a>

### 3D Printed Case
- **3D Printed Case**: Download from <a href="https://www.printables.com/model/1565724-wledpixel-smart-wifi-led-matrix-clock-info-display" target="_blank">Printables</a>


## Web UI - Settings
![img](https://raw.githubusercontent.com/widapro/wled-pixel-v2/master/img/wledPixel_settingsS.jpeg)


## Hardware GPIO for external devices, hardcoded:

### ESP32
```
// Display pinout
#define DATA_PIN  23         // ESP32 GPIO23
#define CS_PIN    5          // ESP32 GPIO5
#define CLK_PIN   18         // ESP32 GPIO18

// Ds18b20 pinout
const int oneWireBus = 4;    // ESP32 GPIO04
```

### ESP8266
```
// Display pinout
#define DATA_PIN  D7         // WeMos D1 mini or ESP8266 -> GPIO13
#define CS_PIN    D6         // WeMos D1 mini or ESP8266 -> GPIO12
#define CLK_PIN   D5         // WeMos D1 mini or ESP8266 -> GPIO14

// Ds18b20 pinout
const int oneWireBus = D4;   // WeMos D1 mini or ESP8266 -> GPIO02
```


## Wiring
![Wiring diagram](img/wledPixel_Wiring.png)

## First boot:
> - Device will boot in wifi access point mode and show "AP Mode" on the display.
> - AP wifi name: **wledPixel-[short MAC address]**
> - AP wifi password: **12345678**
> - Connect to this AP and configure your wifi settings (connect to your wifi network)
> - When the device connects to the WIFI network, the device's IP address will be displayed on the zone0
> - Open the browser and go to the device IP address

## Building from Source

### Prerequisites
- <a href="https://platformio.org/install" target="_blank">PlatformIO</a> (VSCode extension or CLI)
- Git

### Build Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/widapro/wledPixel.git
   cd wledPixel
   ```

2. **Build for ESP32:**
   ```bash
   pio run -e esp32
   ```

3. **Build for ESP8266 (D1 Mini):**
   ```bash
   pio run -e d1_mini
   ```

4. **Upload firmware:**
   ```bash
   # For ESP32
   pio run -e esp32 -t upload

   # For ESP8266
   pio run -e d1_mini -t upload
   ```

5. **Monitor serial output:**
   ```bash
   pio device monitor -b 115200
   ```

### Troubleshooting

If the device gets stuck at boot, erase the flash memory:
```bash
pio run -e esp32 -t erase
pio run -e esp32 -t upload
```


## API
```
/api/temperature      - get measured temperature from connected ds18b20 sensor
```

## Icons in Wled symbol font:
<img width="898" alt="Icons in Wled symbol font" src="https://user-images.githubusercontent.com/6948905/179143312-908f9cda-a766-4928-9fb2-5f4c08b55dbc.png">

```
1 - "X"
2 - "stop"
3 - "rain"
4 - "ip:"
8 - "full light segment"
A - "calendar"
B - "windows"
C - "clouds"
D - "door"
E - "female"
F - "snowflake"
G - "key"
H - "male"
I - "alarm"
J - "clock"
K - "garbage"
L - "info"
M - "moon"
N - "message"
O - "reminder"
P - "wifi"
R - "huge rain"
S - "sun"
T - "thunderstorm"
c - "cloud"
f - "fog"
m - "cloud and moon"
r - "rain"
s - "sun and cloud"
° - "degree symbol"
```


## MQTT topics:
* `devicePrefix/zoneN/text`             - zone text (when workmode=mqttClient)
* `devicePrefix/zoneN/scrolleffectIn`   - scroll effect IN
* `devicePrefix/zoneN/scrolleffectOut`  - scroll effect Out
* `devicePrefix/zoneN/scrollspeed`      - scroll speed
* `devicePrefix/zoneN/scrollpause`      - scroll pause
* `devicePrefix/zoneN/scrollalign`      - scroll alignment
* `devicePrefix/zoneN/charspacing`      - character spacing
* `devicePrefix/intensity`              - brightness
* `devicePrefix/zoneN/workmode`         - zone work mode
* `devicePrefix/zoneN/scrollInfinite`   - infinite scroll (on/off, true/false, 1/0)
* `devicePrefix/power`                  - display power control, support on / off values

where **devicePrefix** = wledPixel-A071
**zoneN** = zone number (e.g. Zone0, Zone1, Zone2, Zone3)

#### Supported workMode:
workMode supported values:
* `mqttClient`    - MQTT client
* `manualInput`   - Manual static text inputed in UI
* `wallClock`     - NTP sync clock
* `owmWeather`    - Open Weather Map client
* `haClient`      - Home Assistant client
* `wopr`          - WarGames effect


## Display Behavior for MQTT and Manual Modes

Both **MQTT Client** and **Manual Input** modes share the same display logic:

### When the message **fits** on the screen:
- The message appears using the selected **IN effect**.
- **If Infinite Scroll is DISABLED**:
  - If **OUT effect** is `NO_EFFECT`: The message stays static on the display until a new message arrives.
  - If **OUT effect** is any other effect: The animation plays once (**IN** → **Scroll pause** → **OUT**) and then stops. The message does not repeat.
- **If Infinite Scroll is ENABLED**:
  - If **OUT effect** is `NO_EFFECT`: The message stays static on the display until a new message arrives.
  - If **OUT effect** is any other effect: The message loops continuously (**IN** → **Scroll pause** → **OUT** → repeat).

### When the message **does not fit** on the screen:
- It automatically scrolls to show the full text.
- If **Infinite Scroll** is enabled: It scrolls in a loop.
- If **Infinite Scroll** is disabled: It scrolls once and stops.

### Service Messages (e.g., "mqtt ok", "mqtt err", mode names)
- If a service message doesn't fit, it scrolls once and then remains static.
- Duplicate status messages are ignored to prevent flickering.
- Can be completely disabled via the **"Disable service messages"** setting.


## Wall clock
Wall clock mode support next following display options:
* `HH:MM`       - Hours : Minutes [21:43]
* `HH:MM:SS`    - Hours : Minutes : Seconds [21:43:54]
* `HH`          - Hours [21]
* `MM`          - Minutes [43]
* `dd.mm.yyyy`  - Day.Month.Year [21.06.2022]
* `dd.mm`       - Day.Month [21.06]
* `dd.mm aa`    - Day.Month weekday name (e.g., Sun) [21.06 Tue] *in Cyrillic font weekday name will be in Cyrillic
* `aa`          - Weekday name (e.g. Sun) *in Cyrillic font weekday name will be in Cyrillic

### Display Logic
* **If text fits in the zone:**
  - Displays statically.
  - Dots blink (unless disabled).
* **If text does NOT fit in the zone:**
  - Text scrolls completely until it disappears.
  - Screen remains blank for the duration of **Scroll Pause**.
  - Text starts scrolling again.
  - Dots do **NOT** blink to prevent interrupting the scroll cycle.

> **Note:** The **Scroll Effect Out** setting in the Web UI is disabled (greyed out) for Wall Clock mode because the loop animation doesn't use an exit effect.


## Scroll effect list
Default effects:
* `PA_RANDOM`
* `PA_PRINT`
* `PA_SCAN_HORIZ`
* `PA_SCROLL_LEFT`
* `PA_WIPE`
* `PA_SCAN_VERTX`
* `PA_SCROLL_UP_LEFT`
* `PA_SCROLL_UP`
* `PA_FADE`
* `PA_OPENING_CURSOR`
* `PA_GROW_UP`
* `PA_SCROLL_UP_RIGHT`
* `PA_BLINDS`
* `PA_CLOSING`
* `PA_GROW_DOWN`
* `PA_SCAN_VERT`
* `PA_SCROLL_DOWN_LEFT`
* `PA_WIPE_CURSOR`
* `PA_SCAN_HORIZX`
* `PA_DISSOLVE`
* `PA_MESH`
* `PA_OPENING`
* `PA_CLOSING_CURSOR`
* `PA_SCROLL_DOWN_RIGHT`
* `PA_SCROLL_RIGHT`
* `PA_SLICE`
* `PA_SCROLL_DOWN`
* `PA_SPRITE` (Generic)

### Custom Sprites
* `PACMAN`
* `WAVE`
* `ROLL`
* `LINES`
* `ARROW`
* `SAILBOAT`
* `STEAMBOAT`
* `HEART`
* `INVADER`
* `ROCKET`
* `FBALL`
* `CHEVRON`
* `WALKER`
* `MARIO` (New!)
* `GHOST` (New!)
* `DINO` (New!)


## Demo Video
<a href="https://youtu.be/0IlsjbE2lU0" target="_blank">Watch the effects demo on YouTube</a>


## Home Assistant automation example:
Send sensor value each time when a value has been changed
```
---
- alias: Outside temp change - send new temp to wled panel mqtt
  initial_state: 'on'
  trigger:
    platform: state
    entity_id: sensor.outside_thp_sensor_3
  action:
    - service: mqtt.publish
      data:
        topic: wledPixel-A071/zone0/text
        payload_template: "{{ states('sensor.outside_thp_sensor_3') }} C"
```
Change **wledPixel-A071** to device name with your prefix
