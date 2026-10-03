#include "st7789v.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/structs/io_bank0.h>
#include <pico/time.h>
#include <unistd.h>
void write_command(uint8_t command, const uint8_t* data, std::size_t data_length) {
	gpio_put(ST7789V_PIN_CS, 0);
	gpio_put(ST7789V_PIN_DC, 0);
	spi_write_blocking(SPI_LCD, &command, 1);
	if(data_length > 0) {
		gpio_put(ST7789V_PIN_DC, 1);
		spi_write_blocking(SPI_LCD, data, data_length);
	}
	gpio_put(ST7789V_PIN_CS, 1);
}

void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	const uint8_t columns[] = { static_cast<uint8_t>(x0 >> 8),
								static_cast<uint8_t>(x0),
								static_cast<uint8_t>(x1 >> 8),
								static_cast<uint8_t>(x1) };
	const uint8_t rows[]	= { static_cast<uint8_t>(y0 >> 8),
								static_cast<uint8_t>(y0),
								static_cast<uint8_t>(y1 >> 8),
								static_cast<uint8_t>(y1) };

	write_command(0x2A, columns, sizeof(columns));
	write_command(0x2B, rows, sizeof(rows));
}

void initialize_display() {
	gpio_init(ST7789V_PIN_CS);
	gpio_init(ST7789V_PIN_DC);
	gpio_init(ST7789V_PIN_RST);

	gpio_set_dir(ST7789V_PIN_CS, GPIO_OUT);
	gpio_set_dir(ST7789V_PIN_DC, GPIO_OUT);
	gpio_set_dir(ST7789V_PIN_RST, GPIO_OUT);

	gpio_put(ST7789V_PIN_CS, 1);
	gpio_put(ST7789V_PIN_DC, 1);
	gpio_put(ST7789V_PIN_RST, 1);

	spi_init(SPI_LCD, 62'500'000);
	spi_set_format(SPI_LCD, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

	gpio_set_function(ST7789V_PIN_SCK, GPIO_FUNC_SPI);
	gpio_set_function(ST7789V_PIN_MOSI, GPIO_FUNC_SPI);

	gpio_put(ST7789V_PIN_RST, 0);
	sleep_ms(20);
	gpio_put(ST7789V_PIN_RST, 1);
	sleep_ms(120);

	write_command(0x01); // Software reset
	sleep_ms(150);

	write_command(0x11); // Sleep out
	sleep_ms(120);

	const uint8_t pixel_format = 0x55; // 16 bit RGB565
	write_command(0x3A, &pixel_format, 1);

	// Rotate native 240x320  memory onto 320x240 landscape layout
	const uint8_t memory_access = 0x60;
	write_command(0x36, &memory_access, 1);

	write_command(0x20); // Display inversion off
	write_command(0x13); // Normal display mode
	write_command(0x29); // Display on
	sleep_ms(120);
}

void write_rgb565(uint8_t* data, size_t size, int x, int y, uint32_t width, uint32_t height) {
	set_address_window(x, y, width - 1, y + height - 1);
	write_command(0x2C); // RAMWR

	gpio_put(ST7789V_PIN_DC, 1);
	gpio_put(ST7789V_PIN_CS, 0);

	spi_write_blocking(SPI_LCD, data, size);

	gpio_put(ST7789V_PIN_CS, 1);
}

void clear_display() {
	uint8_t black_rows[WIDTH * 2 * 2]{};

	for(uint16_t row = 0; row < HEIGHT; row += 2) {
		write_rgb565(black_rows, sizeof(black_rows), 0, row, WIDTH, 2);
	}
}
