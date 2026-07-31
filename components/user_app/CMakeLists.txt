# Collect all .cpp files recursively from the component directory
file(GLOB_RECURSE APP_SOURCES "*.cpp")

idf_component_register(
    SRCS ${APP_SOURCES}
    PRIV_REQUIRES sd_card_bsp lvgl ui_bsp adc_bsp esp_wifi_bsp ble_scan_bsp qmi8658c
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

# target_sources(${COMPONENT_LIB}                 PRIVATE 
#     "fonts/inconsolata_regular_48.c"
#     "fonts/inconsolata_regular_64.c"
# )
