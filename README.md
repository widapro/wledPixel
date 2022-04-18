# wled-pixel-v2
Dot matrix panel management

[This is a new version of firmware, **v2.0.1**](https://github.com/widapro/wled-pixel-v2/releases/tag/v2.0.1)

What's new:

```
1. fully rewritten code, fixed WDT errors
2. Web UI for Wifi configuration (all settings in UI)
3. Web UI for configuration
4. 3 work modes: MQTT client, Wall clock, Manual send text
```

First boot:
> - Device will boot in wifi access point mode and write the wifi name and password on the display.
> - AP wifi name: wled-AP
>   AP wifi password: 12345678
> - Connect to this AP and configure your wifi settings (connect to your wifi network)
> - When the device connects to the WIFI network, the device's IP address will be displayed on the zone0
> - Open the browser and go to the device IP address

Download firmware: https://github.com/widapro/wled-pixel-v2/releases/tag/v2.0.1

Firmware compiled with next parameters:
```
// Display pinout
#define DATA_PIN  D7                     // WeMos D1 mini GPIO13
#define CS_PIN    D6                     // WeMos D1 mini GPIO12
#define CLK_PIN   D5                     // WeMos D1 mini GPIO14
```
