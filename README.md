# chlight
'Change Light' Linux utility to change brightness of ALL devices e.g. monitor, keyboard, mouse, caps-lock etc.

```
Usage: chlight [OPTION] [index|name] [max|brightness]
Options:
        -a              | Additional devices (default is only backlight devices)
        -h --help       | Displays this information
        -v              | Verbose
Examples:
        chlight         | List devices | (index) (device) (brightness) (max_brightness)
        0  asus::kbd_backlight  0      3
        1  intel_backlight      10000  19200

        chlight 0       | List index 0 device info
        asus::kbd_backlight 0 3

        chlight 1 3     | Index 1 device changed to brightness 3
        chlight kbd m   | Device with 'kbd' in its name changed to max_brightness
```
## INSTALLATION

```
git clone https://github.com/koivuniemi/chlight.git
cd chlight
chmod +x chlight
sudo cp chlight /usr/bin/
```
