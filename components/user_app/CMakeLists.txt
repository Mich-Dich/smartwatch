# Collect all .cpp files recursively from the component directory
file(GLOB_RECURSE APP_SOURCES "*.cpp")

idf_component_register(
    SRCS ${APP_SOURCES}
    PRIV_REQUIRES esp_adc sd_card_bsp lvgl ui_bsp esp_wifi_bsp ble_scan_bsp qmi8658c
    INCLUDE_DIRS    "./"    # root of component
)

# Set C++ standard and suppress LVGL enum warning
target_compile_options(${COMPONENT_LIB}         PRIVATE
    
    $<$<COMPILE_LANGUAGE:CXX>:
        -std=c++23
        -Wno-deprecated-enum-enum-conversion
    >
)

# Precompile the shared header (this will be used by all .cpp files)
target_precompile_headers(${COMPONENT_LIB}      PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:util/pch.hpp>
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
