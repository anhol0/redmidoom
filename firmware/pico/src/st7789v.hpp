#pragma once

#include "hardware/spi.h"
#include <cstdint>

spi_inst_t* const SPI_LCD		= spi1;
constexpr uint ST7789V_PIN_CS	= 9;
constexpr uint ST7789V_PIN_SCK	= 10;
constexpr uint ST7789V_PIN_MOSI = 11;
constexpr uint ST7789V_PIN_DC	= 12;
constexpr uint ST7789V_PIN_RST	= 13;

constexpr uint16_t WIDTH  = 320;
constexpr uint16_t HEIGHT = 240;

void write_command(uint8_t command, const uint8_t* data = nullptr, std::size_t data_length = 0);
void initialize_display();
void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void write_rgb565(uint8_t* data, size_t size, int x, int y, uint32_t width, uint32_t height);
void clear_display();
