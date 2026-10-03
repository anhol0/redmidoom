#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/structs/io_bank0.h>
#include <pico/time.h>
#include <unistd.h>

#include "hardware/spi.h"

namespace {

spi_inst_t* const SPI_LCD = spi1;
constexpr uint PIN_CS	  = 9;
constexpr uint PIN_SCK	  = 10;
constexpr uint PIN_MOSI	  = 11;
constexpr uint PIN_DC	  = 12;
constexpr uint PIN_RST	  = 13;

constexpr uint16_t WIDTH  = 320;
constexpr uint16_t HEIGHT = 240;

void write_command(uint8_t command, const uint8_t* data = nullptr, std::size_t data_length = 0) {
	gpio_put(PIN_CS, 0);
	gpio_put(PIN_DC, 0);
	spi_write_blocking(SPI_LCD, &command, 1);
	if(data_length > 0) {
		gpio_put(PIN_DC, 1);
		spi_write_blocking(SPI_LCD, data, data_length);
	}
	gpio_put(PIN_CS, 1);
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
	gpio_init(PIN_CS);
	gpio_init(PIN_DC);
	gpio_init(PIN_RST);

	gpio_set_dir(PIN_CS, GPIO_OUT);
	gpio_set_dir(PIN_DC, GPIO_OUT);
	gpio_set_dir(PIN_RST, GPIO_OUT);

	gpio_put(PIN_CS, 1);
	gpio_put(PIN_DC, 1);
	gpio_put(PIN_RST, 1);

	spi_init(SPI_LCD, 62'500'000);
	spi_set_format(SPI_LCD, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

	gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
	gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

	gpio_put(PIN_RST, 0);
	sleep_ms(20);
	gpio_put(PIN_RST, 1);
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

	write_command(0x21); // Display inversion on
	write_command(0x13); // Normal display mode
	write_command(0x29); // Display on
	sleep_ms(120);
}

void fill_display(uint16_t color) {
	set_address_window(0, 0, WIDTH - 1, HEIGHT - 1);
	write_command(0x2C); // RAMWR

	constexpr uint16_t block_size = 128;

	uint8_t block[block_size];
	for(std::size_t i = 0; i < sizeof(block); i += 2) {
		block[i]	 = static_cast<uint8_t>(color >> 8);
		block[i + 1] = static_cast<uint8_t>(color);
	}

	gpio_put(PIN_DC, 1);
	gpio_put(PIN_CS, 0);

	uint32_t remaining_pixels = static_cast<uint32_t>(WIDTH) * HEIGHT;

	while(remaining_pixels > 0) {
		const uint32_t pixels =
			remaining_pixels > block_size / 2 ? block_size / 2 : remaining_pixels;
		spi_write_blocking(SPI_LCD, block, pixels * 2);
		remaining_pixels -= pixels;
	}
	gpio_put(PIN_CS, 1);
}

} // namespace

int main() {
	initialize_display();
	while(true) {
		fill_display(0xF800); // Red
		sleep_ms(1000);

		fill_display(0x07E0); // Green
		sleep_ms(1000);

		fill_display(0x001F); // Blue
		sleep_ms(1000);

		fill_display(0xFFFF); // White
		sleep_ms(1000);

		fill_display(0x0000); // Black
		sleep_ms(1000);
	}
}
