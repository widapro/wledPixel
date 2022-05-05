# wled-pixel-v2
Dot matrix panel management
![img](https://raw.githubusercontent.com/widapro/wled-pixel-v2/master/img/main.jpg)

[The latest firmware version can be found in Releases](https://github.com/widapro/wled-pixel-v2/releases)

What's new:

```
1. Fully rewritten code, fixed WDT errors
2. Web UI for Wifi configuration (all settings in UI)
3. Web UI for configuration
4. 4 work modes: 
4.1 MQTT client
4.2 Wall clock
4.3 Open Weather Map client
4.4 Manual send text
5. Support 12 segments of display (3 panels by 4 segment each)
6. Support 3 separated work zones
```

First boot:
> - Device will boot in wifi access point mode and write the wifi name and password on the display.
> - AP wifi name: **wled-AP**
> - AP wifi password: **12345678**
> - Connect to this AP and configure your wifi settings (connect to your wifi network)
> - When the device connects to the WIFI network, the device's IP address will be displayed on the zone0
> - Open the browser and go to the device IP address

Firmware compiled with next parameters:
```
// Display pinout
#define DATA_PIN  D7                     // WeMos D1 mini GPIO13
#define CS_PIN    D6                     // WeMos D1 mini GPIO12
#define CLK_PIN   D5                     // WeMos D1 mini GPIO14
```
