set(COMPONENT_PCH_HEADER "util/pch.hpp")
idf_component_register(SRCS "main.cpp"
                        "spec_analyser/spec_analyser.cpp"
                        "util/display.cpp"
                        INCLUDE_DIRS ".")