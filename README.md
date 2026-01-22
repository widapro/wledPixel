# wledPixel
![img](https://raw.githubusercontent.com/widapro/wled-pixel-v2/master/img/wledPixel_logoPicS.jpeg)

ESP32 and ESP8266 and MAX7219 dot matrix display management

[The latest firmware version can be found in Releases](https://github.com/widapro/wled-pixel-v2/releases)

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
1. Dot matrix display MAX7219, something like this: https://aliexpress.com/item/32618155357.html
2. ESP32 or WeMos D1 Mini (ESP8266), something like this: https://aliexpress.com/item/32651747570.html

### 3D Printed Case
- **3D Printed Case**: Download from [Printables](https://www.printables.com/model/1565724-wledpixel-smart-wifi-led-matrix-clock-info-display)

## Web UI - Settings
![img](https://raw.githubusercontent.com/widapro/wled-pixel-v2/master/img/wledPixel_settingsS.jpeg)

##### Firmware compiled with next parameters:
**ESP32**
```
// Display pinout
#define DATA_PIN  23         // ESP32 GPIO23
#define CS_PIN    5          // ESP32 GPIO5
#define CLK_PIN   18         // ESP32 GPIO18

// Ds18b20 pinout
const int oneWireBus = 4;    // ESP32 GPIO04
```

**ESP8266**
```
// Display pinout
#define DATA_PIN  D7         // WeMos D1 mini or ESP8266 -> GPIO13
#define CS_PIN    D6         // WeMos D1 mini or ESP8266 -> GPIO12
#define CLK_PIN   D5         // WeMos D1 mini or ESP8266 -> GPIO14

// Ds18b20 pinout
const int oneWireBus = D4;   // WeMos D1 mini or ESP8266 -> GPIO02
```

## Wiring
![img](https://github.com/widapro/wledPixel/blob/9ea4164575de9fc7c5ffbd9eb50f75493889256c/img/WledPixel%20Wiring800.png)


## First boot:
> - Device will boot in wifi access point mode and write the wifi name and password on the display.
> - AP wifi name: **wledPixel-[short MAC address]**
> - AP wifi password: **12345678**
> - Connect to this AP and configure your wifi settings (connect to your wifi network)
> - When the device connects to the WIFI network, the device's IP address will be displayed on the zone0
> - Open the browser and go to the device IP address

## Building from Source

### Prerequisites
- [PlatformIO](https://platformio.org/install) (VSCode extension or CLI)
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
