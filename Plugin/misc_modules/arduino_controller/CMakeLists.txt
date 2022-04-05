cmake_minimum_required(VERSION 3.13)
project(arduino_controller)

file(GLOB SRC "src/*.cpp")

add_library(arduino_controller SHARED ${SRC})
target_link_libraries(arduino_controller PRIVATE sdrpp_core)
set_target_properties(arduino_controller PROPERTIES PREFIX "")

target_include_directories(arduino_controller PRIVATE "src/" "../../decoder_modules/radio/src")

if (MSVC)
    target_compile_options(arduino_controller PRIVATE /O2 /Ob2 /std:c++17 /EHsc)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(arduino_controller PRIVATE -O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
else ()
    target_compile_options(arduino_controller PRIVATE -O3 -std=c++17)
endif ()

# Install directives
install(TARGETS arduino_controller DESTINATION lib/sdrpp/plugins)