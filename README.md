# chlight
'Change Light' Linux utility to change brightness of ALL devices e.g. monitor, keyboard, mouse, caps-lock etc.

```
Usage: chlight [option] [index|name] [brightness|max]
Options:
        -h --help      | Displays this information
        -v             | Verbose
Examples:
        chlight        | List devices | index device brightness max_brightness
        chlight 1 1000 | Index 1 device changed to brightness 1000
        chlight kbd 3  | Device with 'kbd' in its name changed to 3
```
## INSTALLATION

```
git clone https://github.com/koivuniemi/chlight.git
cd chlight
make
sudo cp chlight /usr/local/bin/
```
