# Collect all .cpp files recursively from the component directory
file(GLOB_RECURSE APP_SOURCES "*.cpp")

idf_component_register(
    SRCS ${APP_SOURCES}
    PRIV_REQUIRES 
        lvgl
        esp_adc
        esp_wifi_bsp
        esp_http_client
        sd_card_bsp
        ui_bsp
        ble_scan_bsp
        qmi8658c
    INCLUDE_DIRS    "./"
)

# Set C++ standard and suppress LVGL enum warning
target_compile_options(${COMPONENT_LIB}         PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:
        -std=c++23
        -Wno-deprecated-enum-enum-conversion
    >
)

target_sources(${COMPONENT_LIB}                 PRIVATE
    "font/inconsolata_extra_bold_14.c"
    "font/inconsolata_regular_26.c"
    "font/inconsolata_regular_48.c"
    "font/inconsolata_regular_64.c"
    "font/inconsolata_regular_128.c"
    "font/awesome_5_regular_26.c"
    "font/awesome_5_regular_42.c"
    "font/wildgrin_152.c"
)
