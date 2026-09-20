# cmake_minimum_required(VERSION 3.13...3.27)

# include(pico_sdk_import.cmake)

# project(redmidoom)

# set(CMAKE_SYSTEM_NAME linux)
# set(CMAKE_SYSTEM_ARCH mips)

# pico_sdk_init()

# set(CMAKE_C_COMPILER mipsel-openwrt-linux-musl-gcc)
# set(CMAKE_CXX_COMPILER mipsel-openwrt-linux-musl-g++)

cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(w5500_test C CXX ASM)

pico_sdk_init()

add_executable(w5500_test
    src/pico/w5500_test.cpp
)

target_link_libraries(w5500_test
    pico_stdlib
    hardware_spi
)

pico_enable_stdio_usb(w5500_test 1)
pico_enable_stdio_uart(w5500_test 0)

pico_add_extra_outputs(w5500_test)
