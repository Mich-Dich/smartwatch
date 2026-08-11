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

set(COMPILE_TIME_HEADER "${CMAKE_CURRENT_BINARY_DIR}/compile_time.h")

# Use a shell command to generate the header
add_custom_command(
    OUTPUT ${COMPILE_TIME_HEADER}
    COMMAND sh -c "echo '// Auto-generated' > ${COMPILE_TIME_HEADER} && \
                   echo '#define BUILD_HOUR   '$(date +%H) >> ${COMPILE_TIME_HEADER} && \
                   echo '#define BUILD_MINUTE '$(date +%M) >> ${COMPILE_TIME_HEADER} && \
                   echo '#define BUILD_SECOND '$(date +%S) >> ${COMPILE_TIME_HEADER}"
    DEPENDS
    VERBATIM
)

# Ensure the header is built before compiling main_screen.cpp
add_custom_target(gen_compile_time DEPENDS ${COMPILE_TIME_HEADER})

# Make the component library depend on that target so the header is generated before compilation
add_dependencies(${COMPONENT_LIB} gen_compile_time)

target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(main_screen.cpp PROPERTIES OBJECT_DEPENDS ${COMPILE_TIME_HEADER})   # Force recompilation of main_screen.cpp when compile_time.h changes

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
