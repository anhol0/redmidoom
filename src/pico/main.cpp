// Here will be code for Raspberry Pi Pico side implementation of redmidoom
// project. Pico will be used for polling data from the router through W5500
// ethernet module. Then it will display the output to the STM7789V based
// 320x240 display. Input polling will be performed through a set of buttons
// connected to GPIO pins on the Pico. Display and Ethernet controllers will be
// connected to the SPI1 and SPI0 respectively. This is done to prevent fighting
// over the SPI bus. Pico side implementation will be written in C++ using Pico
// SDK, while AC2100 side implementation will be written in C using Libc
// provided by the OpenWRT 25.12.2 system
