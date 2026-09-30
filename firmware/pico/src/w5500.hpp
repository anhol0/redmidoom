#pragma once

#include "hardware/spi.h"

spi_inst_t* const W5500_SPI	  = spi0;
constexpr uint W5500_PIN_MISO = 16;
constexpr uint W5500_PIN_CS	  = 17;
constexpr uint W5500_PIN_SCK  = 18;
constexpr uint W5500_PIN_MOSI = 19;

constexpr uint8_t UDP_SOCKET = 0;
constexpr uint16_t UDP_PORT	 = 5000;

// Pico to ioLibrary wrapper
void chip_select();
void chip_deselect();
uint8_t spi_read_byte();
void spi_write_byte(uint8_t value);
void spi_read_block(uint8_t* buffer, uint16_t len);
void spi_write_block(uint8_t* buffer, uint16_t len);
void initialize_spi();
bool init_w5500();
