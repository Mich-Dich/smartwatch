


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
