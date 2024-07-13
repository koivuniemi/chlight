# chlight
"Change Light" Linux utility to change brightness of devices

```
Usage: chlight [index|name] [max|brightness]
Examples:
        chlight       | list devices | (index) (device_name) (brightness) (max_brightness)
        0  asus::kbd_backlight  0      3
        1  intel_backlight      10000  19200

        chlight 0     | list index 0 device info
        asus::kbd_backlight 0 3

        chlight 1 3   | index 1 device changed to brightness 3
        chlight kbd m | device with 'kbd' in its name changed to max_brightness
```
