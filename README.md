
# Commands

## Check for available ports
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

## Export needed commands
```bash
source ~/esp/esp-idf/export.sh
```

## Set Target
```bash
idf.py set-target esp32s3
```

## Build & Flash
```bash
clear; idf.py build && idf.py flash monitor
```










## Font creation
- URL: https://lvgl.io/tools/fontconverter
  - Name format: <font-name>-<font-type (regular, ...)>-<height>    
    - example: inconsolata_regular_64
  - Bpp: 2 bit-per-pixel
  - Output format: C file
  - Range: 0x20-0x7F, 0xB0, 0x2022
    (For Icons: 0xF000-0xF8FF)







# TODO: 
- save brightness to non-volatile memory (for rebooting/powerless/...)
- add Bluetooth/Wifi power-saving functions (Need to figure out how to ensure bluetooth remains active if music screen running)
- detect Powering by cable (reenable screen)
