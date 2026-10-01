#include "st7789v.hpp"
#include "w5500.hpp"

#include "socket.h"
#include "wizchip_conf.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pico/stdio.h>
#include <pico/time.h>

constexpr size_t DOOM_WIDTH = 320;
constexpr size_t DOOM_HEIGHT = 200;
constexpr size_t RGB565_BYTES_PER_PIXEL = 2;

constexpr size_t HEADER_LEN = 24;
constexpr size_t FRAMEBUFFER_SIZE =
    DOOM_WIDTH * DOOM_HEIGHT * RGB565_BYTES_PER_PIXEL;

static uint8_t palette[256 * 2];
static bool palette_valid = false;

enum PacketType {
    PACKET_FRAME_STRIPE = 1,
    PACKET_PALETTE      = 2,
};

struct Header {
    uint32_t packet_type;
    uint32_t data_len;
    uint32_t row;
    uint32_t width;
    uint32_t height;
    uint32_t index;
    // uint32_t pallete_version; - in future
};

bool parse_packet(
	const uint8_t* buffer,
	const uint32_t size,
	Header* header
) {
	if(size < HEADER_LEN) {
		return false;
	}

	uint32_t packet_type =
	    ((uint32_t)buffer[0] << 24) |
		((uint32_t)buffer[1] << 16) |
	    ((uint32_t)buffer[2] << 8)  |
				   buffer[3];

    uint32_t data_len =
        ((uint32_t)buffer[4] << 24) |
		((uint32_t)buffer[5] << 16) |
		((uint32_t)buffer[6] << 8)  |
		           buffer[7];

    uint32_t row =
        ((uint32_t)buffer[8] << 24) |
        ((uint32_t)buffer[9] << 16) |
		((uint32_t)buffer[10] << 8) |
		           buffer[11];

    uint32_t width =
        ((uint32_t)buffer[12] << 24) |
        ((uint32_t)buffer[13] << 16) |
		((uint32_t)buffer[14] << 8)  |
		           buffer[15];

    uint32_t height =
        ((uint32_t)buffer[16] << 24) |
		((uint32_t)buffer[17] << 16) |
		((uint32_t)buffer[18] << 8)  |
		           buffer[19];

    uint32_t index =
        ((uint32_t)buffer[20] << 24) |
		((uint32_t)buffer[21] << 16) |
		((uint32_t)buffer[22] << 8)  |
		           buffer[23];

    if (packet_type != PACKET_FRAME_STRIPE &&
        packet_type != PACKET_PALETTE) {
        return false;
    }

    if (size != HEADER_LEN + data_len) {
        return false;
    }

    if (packet_type == PACKET_PALETTE) {
        if (data_len != sizeof(palette)) {
            return false;
        }
    } else {
        if (!palette_valid ||
            width != DOOM_WIDTH ||
            height == 0 ||
            row >= DOOM_HEIGHT ||
            height > DOOM_HEIGHT - row) {
            return false;
        }

        const uint32_t expected_data_len = width * height;

        if (data_len != expected_data_len) {
            return false;
        }
    }

    *header = {
        packet_type,
        data_len,
        row,
        width,
        height,
        index,
    };

    return true;
}

bool mark_received(uint32_t row, uint32_t* checklist) {
	if(row >= DOOM_HEIGHT) {
		return false;
	}

	const uint32_t word = row >> 5;
	const uint32_t bit = row & 31;
    const uint32_t mask = 1u << bit;

    if(checklist[word] & mask) {
        return false;
    }

    checklist[word] |= mask;
    return true;
}


int main() {

	// Initializing the ST7789V display
	initialize_display();

	uint8_t black_rows[DOOM_WIDTH * 2 * 2]{};

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
	const int8_t result = socket(UDP_SOCKET, Sn_MR_UDP, UDP_PORT, SF_IO_NONBLOCK);
	if(result != UDP_SOCKET) {
		printf("Socket initialization failed: %d\n", result);
		while(true) {
			tight_loop_contents();
		}
	}

	printf("Listening on UDP port %d\n", UDP_PORT);
	uint8_t buffer[1472]; // 1472 bytes is the max size of unfragmented UDP packet

	alignas(4) static uint8_t framebuffer[FRAMEBUFFER_SIZE];
	static uint32_t recv_rows[(DOOM_HEIGHT + 31) / 32] = {};
	uint16_t recv_rows_count = 0;

	while(true) {
	    // Logging packets recieved
		uint8_t sender_ip[4]{ 0 };
		uint16_t sender_port = 0;
		const int32_t received =
			recvfrom(UDP_SOCKET, buffer, sizeof(buffer), sender_ip, &sender_port);

		if(received <= 0 || static_cast<size_t>(received) < HEADER_LEN) {
		    tight_loop_contents();
			continue;
		}

		// Parse packets
		Header header{0};
		bool rc = parse_packet(buffer, received, &header);

		if(!rc)
		    continue;

		if(header.packet_type == PACKET_PALETTE) {
		    if (header.data_len != sizeof(palette)) {
                continue;
			}

			std::memcpy(palette, buffer + HEADER_LEN, sizeof(palette));
			palette_valid = true;

			std::memset(recv_rows, 0, sizeof(recv_rows));
			recv_rows_count = 0;

			continue;
		}



		const uint8_t *indices = buffer + HEADER_LEN;
		const size_t first_pixel = static_cast<size_t>(header.row) * DOOM_WIDTH;

		for (size_t i = 0; i < header.data_len; ++i) {
            const uint8_t index = indices[i];
            const size_t destination = (first_pixel + i) * 2;

            framebuffer[destination]     = palette[index * 2];
            framebuffer[destination + 1] = palette[index * 2 + 1];
		}

		for(uint32_t row = header.row; row < header.row + header.height; ++row) {
			if(mark_received(row, recv_rows)) {
				++recv_rows_count;
			}
		}

		if(recv_rows_count == DOOM_HEIGHT) {
			write_rgb565(framebuffer, sizeof(framebuffer), 0, 20, DOOM_WIDTH, DOOM_HEIGHT);
			std::memset(recv_rows, 0, sizeof(recv_rows));
			recv_rows_count = 0;
		}

		tight_loop_contents();
	}
}
