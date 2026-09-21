#include "hardware/gpio.h"
#include <W5500/w5500.h>
#include <cstdint>
#include <cstdio>
#include <hardware/structs/io_bank0.h>
#include <pico/platform/common.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <pico/types.h>

#include "hardware/spi.h"

#include "socket.h"
#include "wizchip_conf.h"

namespace {

spi_inst_t* const W5500_SPI = spi0;
constexpr uint PIN_MISO		= 16;
constexpr uint PIN_CS		= 17;
constexpr uint PIN_SCK		= 18;
constexpr uint PIN_MOSI		= 19;

constexpr uint8_t UDP_SOCKET = 0;
constexpr uint16_t UDP_PORT	 = 5000;

// Pico to ioLibrary wrapper

void chip_select() {
	gpio_put(PIN_CS, 0);
}

void chip_deselect() {
	gpio_put(PIN_CS, 1);
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
	gpio_init(PIN_CS);
	gpio_set_dir(PIN_CS, GPIO_OUT);
	gpio_put(PIN_CS, 1);

	spi_init(W5500_SPI, 10000000);
	spi_set_format(W5500_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

	gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
	gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
	gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
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

	wiz_NetInfo info = { .mac  = { 0xEE, 0x4A, 0x4F, 0xA1, 0x22, 0xDE },
						 .ip   = { 192, 168, 67, 2 },
						 .sn   = { 255, 255, 255, 0 },
						 .gw   = { 0, 0, 0, 0 },
						 .dns  = { 0, 0, 0, 0 },
						 .dhcp = NETINFO_STATIC };

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

} // namespace

int main() {
	stdio_init_all();
	sleep_ms(2000);
	initialize_spi();
	if(!init_w5500()) {
		while(true) {
			printf("W5500 Initialization Failed\n");
			sleep_ms(1000);
		}
	}

	printf("Waiting for Ethernet link...\n");

	while(wizphy_getphylink() != PHY_LINK_ON) {
		sleep_ms(250);
	}

	printf("Ethernet link established\n");

	// Open W5500 hardware socket
	const uint8_t result = socket(UDP_SOCKET, Sn_MR_UDP, UDP_PORT, SF_IO_NONBLOCK);
	if(result != UDP_SOCKET) {
		printf("Socket initialization failed: %d\n", result);
		while(true) {
			tight_loop_contents();
		}
	}

	printf("Listening on UDP port %d\n", UDP_PORT);
	uint8_t buffer[1400] = { 0 };

	while(true) {
		uint8_t sender_ip[4]{ 0 };
		uint16_t sender_port = 0;
		const int32_t received =
			recvfrom(UDP_SOCKET, buffer, sizeof(buffer), sender_ip, &sender_port);

		if(received > 0) {
			printf(
				"Received %ld bytes from %u.%u.%u.%u:%u\n",
				static_cast<long>(received),
				sender_ip[0],
				sender_ip[1],
				sender_ip[2],
				sender_ip[3],
				sender_port
			);

			const int32_t sent =
				sendto(UDP_SOCKET, buffer, static_cast<uint16_t>(received), sender_ip, sender_port);

			if(sent < 0) {
				printf("sendto() failed: %ld\n", static_cast<long>(sent));
			}
		}

		tight_loop_contents();
	}
}
