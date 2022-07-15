# wled-pixel-v2
Dot matrix wled panel management
![img](https://raw.githubusercontent.com/widapro/wled-pixel-v2/master/img/main.jpg)

[The latest firmware version can be found in Releases](https://github.com/widapro/wled-pixel-v2/releases)

## Supported features:
1. **Home Assistant client** [display sensor values]
2. **OpenWeatherMap client** [display: temperature (C/F), humidity, pressure, wind speed, weather icon]
3. **MQTT client** [each display zone support own topic]
4. **Wall NTP clock** [NTP sync clock]
5. Manual input
6. Full controll through **web UI**
7. Initial setup through wifi AP and web UI
8. Controll through MQTT
9. Support 3 separated display zones
10. Support 12 display segments in the same time
11. Included 3 different fonts

## First boot:
> - Device will boot in wifi access point mode and write the wifi name and password on the display.
> - AP wifi name: **wled-AP**
> - AP wifi password: **12345678**
> - Connect to this AP and configure your wifi settings (connect to your wifi network)
> - When the device connects to the WIFI network, the device's IP address will be displayed on the zone0
> - Open the browser and go to the device IP address

## Firmware compiled with next parameters:
```
// Display pinout
#define DATA_PIN  D7                     // WeMos D1 mini GPIO13
#define CS_PIN    D6                     // WeMos D1 mini GPIO12
#define CLK_PIN   D5                     // WeMos D1 mini GPIO14
```

## Icons in Wled symbol font:
<img width="898" alt="Screen Shot 2022-07-14 at 23 09 11" src="https://user-images.githubusercontent.com/6948905/179143312-908f9cda-a766-4928-9fb2-5f4c08b55dbc.png">

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