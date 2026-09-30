#include "st7789v.hpp"
#include "w5500.hpp"

#include "socket.h"
#include "wizchip_conf.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pico/stdio.h>
#include <pico/time.h>

constexpr uint8_t HEADER_LEN = 20;

int main() {

	// Initializing the ST7789V display
	initialize_display();

	uint8_t black_rows[WIDTH * 2 * 2]{};

	for(uint16_t row = 0; row < HEIGHT; row += 2) {
		write_rgb565(black_rows, sizeof(black_rows), 0, row, WIDTH, 2);
	}

	stdio_init_all();
	sleep_ms(2000);

	// Initialization of W5500
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
	uint8_t buffer[1472]; // 1472 bytes is the max size of unfragmented UDP packet
	static uint8_t framebuffer[320 * 240 * 2];
	int packets_recv = 0;
	while(true) {
		uint8_t sender_ip[4]{ 0 };
		uint16_t sender_port = 0;
		const int32_t received =
			recvfrom(UDP_SOCKET, buffer, sizeof(buffer), sender_ip, &sender_port);

		if(received < HEADER_LEN) {
			continue;
		}


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

			uint32_t data_len = ((uint32_t)buffer[0] << 24) |
				((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[2] << 8) | buffer[3];

			uint32_t row = ((uint32_t)buffer[4] << 24) | ((uint32_t)buffer[5] << 16) |
				((uint32_t)buffer[6] << 8) | buffer[7];

			uint32_t width = ((uint32_t)buffer[8] << 24) | ((uint32_t)buffer[9] << 16) |
				((uint32_t)buffer[10] << 8) | buffer[11];

			uint32_t height = ((uint32_t)buffer[12] << 24) |
				((uint32_t)buffer[13] << 16) | ((uint32_t)buffer[14] << 8) |
				buffer[15];

			uint32_t total_packets = ((uint32_t)buffer[16] << 24) |
				((uint32_t)buffer[17] << 16) | ((uint32_t)buffer[18] << 8) |
				buffer[19];

			if(width != WIDTH || height == 0)
				continue;

			if(row >= HEIGHT || height > HEIGHT - row)
				continue;

			const uint32_t expected_data_len = width * height * 2;

			if(data_len != expected_data_len)
				continue;

			if(static_cast<uint32_t>(received) != HEADER_LEN + data_len)
				continue;
			packets_recv++;
			memcpy(framebuffer + row * WIDTH * 2, buffer + HEADER_LEN, data_len);
			if(packets_recv == total_packets) {
				write_rgb565(framebuffer, sizeof(framebuffer), 0, 0, WIDTH, HEIGHT);
			}
		}

		tight_loop_contents();
	}
}
