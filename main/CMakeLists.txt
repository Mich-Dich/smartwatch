set(COMPONENT_PCH_HEADER "util/pch.hpp")
idf_component_register(SRCS "main.cpp"
    "util/wifi.cpp"
    "util/display.cpp"
    "util/RTC.cpp"
    "UI/manager.cpp"
    "screens/clock.cpp"
    "screens/menu.cpp"
    "screens/music.cpp"
    "screens/settings.cpp"
    "screens/spectrum_analyzer.cpp"
    "screens/stopwatch.cpp"
    INCLUDE_DIRS ".")

target_sources(${COMPONENT_LIB} PRIVATE 
    "fonts/inconsolata_regular_48.c"
    "fonts/inconsolata_regular_64.c"
)
