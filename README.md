# chlight
'Change Light' Linux utility to change brightness of ALL devices e.g. monitor, keyboard, mouse, caps-lock etc.

```
Usage: chlight [option] [index|name] [brightness|max]
Options:
        -h --help     | Displays this information
        -v            | Verbose
Examples:
        chlight       | List devices | index device brightness max_brightness
        chlight 1 3   | Index 1 device changed to brightness 3
        chlight kbd m | Device with 'kbd' in its name changed to max_brightness
```
## INSTALLATION

```
git clone https://github.com/koivuniemi/chlight.git
cd chlight
make
sudo cp chlight /usr/local/bin/
```
