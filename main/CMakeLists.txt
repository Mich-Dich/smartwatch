set(COMPONENT_PCH_HEADER "util/pch.hpp")
idf_component_register(SRCS "main.cpp"
                        "spec_analyser/spec_analyser.cpp"
                        "util/wifi.cpp"
                        "util/display.cpp"
                        INCLUDE_DIRS ".")

target_sources(${COMPONENT_LIB} PRIVATE 
    "fonts/inconsolata_regular_48.c"
    "fonts/inconsolata_regular_64.c"
)
