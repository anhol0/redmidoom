#include "w5500.hpp"
#include "hardware/gpio.h"
#include <W5500/w5500.h>
#include <cstdint>
#include <cstdio>
#include <hardware/structs/io_bank0.h>
#include <pico/platform/common.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <pico/types.h>

#include "wizchip_conf.h"

void chip_select() {
	gpio_put(W5500_PIN_CS, 0);
}

void chip_deselect() {
	gpio_put(W5500_PIN_CS, 1);
}

uint8_t spi_read_byte() {
	uint8_t byte = 0;
	spi_read_blocking(W5500_SPI, 0x00, &byte, 1);
	return byte;
}

void spi_write_byte(uint8_t value) {
	spi_write_blocking(W5500_SPI, &value, 1);
}

void spi_read_block(uint8_t* buffer, uint16_t len) {
	spi_read_blocking(W5500_SPI, 0x00, buffer, len);
}

void spi_write_block(uint8_t* buffer, uint16_t len) {
	spi_write_blocking(W5500_SPI, buffer, len);
}

void initialize_spi() {
	gpio_init(W5500_PIN_CS);
	gpio_set_dir(W5500_PIN_CS, GPIO_OUT);
	gpio_put(W5500_PIN_CS, 1);

	uint actual_speed = spi_init(W5500_SPI, 40000000);
	printf("W5500 SPI: %u Hz\n", actual_speed);
	spi_set_format(W5500_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

	gpio_set_function(W5500_PIN_MISO, GPIO_FUNC_SPI);
	gpio_set_function(W5500_PIN_MOSI, GPIO_FUNC_SPI);
	gpio_set_function(W5500_PIN_SCK, GPIO_FUNC_SPI);
}

bool init_w5500() {
	// Tell the library how to communicate with the chip
	// Library doesn't care and know about the backend so we pass function pointers
	reg_wizchip_cs_cbfunc(chip_select, chip_deselect);
	reg_wizchip_spi_cbfunc(spi_read_byte, spi_write_byte);
	reg_wizchip_spiburst_cbfunc(spi_read_block, spi_write_block);

	wizchip_sw_reset();
	sleep_ms(10);

	const uint8_t version = getVERSIONR();

	printf("W5500 VERSIONR = 0x%02x\n", version);

	if(version != 0x04) {
		printf("EXPECTED VERSIONR 0x04!\n");
		return false;
	}


	// The W5500 has 16 KiB TX and RX memory
	// We give all of each to the socket 0 for the test
	uint8_t tx_sizes[8] = { 16, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t rx_sizes[8] = { 16, 0, 0, 0, 0, 0, 0, 0 };
	if(wizchip_init(tx_sizes, rx_sizes) != 0) {
		printf("Invalid W5500 socket memory configuration!\n");
		return false;
	}

	// wiz_NetInfo info = { .mac  = { 0xEE, 0x4A, 0x4F, 0xA1, 0x22, 0xDE },
	// 					 .ip   = { 192, 168, 1, 67 },
	// 					 .sn   = { 255, 255, 255, 0 },
	// 					 .gw   = { 192, 168, 1, 1 },
	// 					 .dns  = { 192, 168, 1, 1 },
	// 					 .dhcp = NETINFO_STATIC };

	wiz_NetInfo info = {
        .mac  = { 0xEE, 0x4A, 0x4F, 0xA1, 0x22, 0xDE },
        .ip   = { 192, 168, 1, 67 },  // Pico
        .sn   = { 255, 255, 255, 0 },
        .gw   = { 192, 168, 1, 1 },   // OpenWrt router
        .dns  = { 192, 168, 1, 1 },
        .dhcp = NETINFO_STATIC
	};

	wizchip_setnetinfo(&info);

	wiz_NetInfo actual{};
	wizchip_getnetinfo(&actual);
	printf(
		"Pico IP: %u.%u.%u.%u\n",
		actual.ip[0],
		actual.ip[1],
		actual.ip[2],
		actual.ip[3]
	);

	return true;
}
