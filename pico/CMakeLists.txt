cmake_minimum_required(VERSION 3.13)

include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

project(picoside C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

set(IOLIB_DIR
    ${CMAKE_CURRENT_LIST_DIR}/thirdparty/ioLibrary_Driver
)

add_library(w5500_iolib STATIC
    ${IOLIB_DIR}/Ethernet/wizchip_conf.c
    ${IOLIB_DIR}/Ethernet/socket.c
    ${IOLIB_DIR}/Ethernet/W5500/w5500.c
)

target_include_directories(w5500_iolib PUBLIC
    ${IOLIB_DIR}/Ethernet
)

target_compile_definitions(w5500_iolib PUBLIC
    _WIZCHIP_=W5500
)

add_executable(picocontroller
    src/st7789v.cpp
    src/w5500.cpp
    src/main.cpp
)

target_link_libraries(picocontroller PRIVATE
    pico_stdlib
    hardware_spi
    w5500_iolib
)

pico_enable_stdio_usb(picocontroller 1)
pico_enable_stdio_uart(picocontroller 0)
pico_add_extra_outputs(picocontroller)
