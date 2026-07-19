


# Commands

## Check for available ports
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```


## Set Target
```bash
idf.py set-target esp32s3
```

## Export needed commands
```bash
source ~/esp/esp-idf/export.sh
```


## Build & Flash
```bash
idf.py build && idf.py flash monitor
```
