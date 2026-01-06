# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Fix: Disable wifi_frag feature not supported on ESP32-C5
add_compile_definitions(WIFI_CMD_FRAG_TX_SUPPORTED=0)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(iperf)
