#include "doomgeneric.h"
#include "doomtype.h"
#include "doomkeys.h"
#include "i_video.h"

#include <stdbool.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

// `DG_GetTicksMs()` using `clock_gettime(CLOCK_MONOTONIC, ...)`
// `DG_SleepMs()` using `nanosleep()`
// `DG_GetKey()` returning no events
// `DG_SetWindowTitle()` as a log or no-op
// `DG_Init()` as a log
// `DG_DrawFrame()` as a frame counter

extern boolean palette_changed;
extern struct color colors[256];

#define UDP_PORT 5000
#define UDP_ADDR "192.168.1.69"

enum { HEADER_LEN     = 24 };
enum { QUEUE_CAPACITY = 32 };

enum PacketType {
    PACKET_FRAME_STRIPE = 1,
    PACKET_PALETTE      = 2,
};

const int LINES_IN_BATCH = 4;

typedef struct Header {
    uint32_t packet_type;
    uint32_t data_len;
    uint32_t row;
    uint32_t width;
    uint32_t height;
    uint32_t index;
    // uint32_t pallete_version; - in future
} Header;


typedef struct Peer {
    struct sockaddr_in address;
    int socketfd;
    uint16_t port;
    const char* ip;
} Peer;

typedef struct {
    int pressed;
    unsigned char key;
} KeyEvent;

uint8_t* buffer;
Peer peer = {0};

static KeyEvent key_queue[QUEUE_CAPACITY];
unsigned int queue_write_index;
unsigned int queue_read_index;
unsigned int queue_count;
static uint16_t previous_button_state;

struct timespec start;

static const unsigned char doom_keys[9] = {
    KEY_UPARROW,
    KEY_DOWNARROW,
    KEY_LEFTARROW,
    KEY_RIGHTARROW,
    KEY_ENTER,
    KEY_USE,
    KEY_RSHIFT,
    KEY_ESCAPE,
    KEY_FIRE,
};

int connect_to_peer(uint16_t port, const char* ip, Peer* peer) {
    // 1. Create a UDP socket (SOCK_DGRAM)
    peer->socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (peer->socketfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // 2. Clear and configure the destination address structure
    memset(&peer->address, 0, sizeof(peer->address));
    peer->address.sin_family = AF_INET;
    peer->address.sin_port = htons(port); // Convert port to network byte order

    // 3. Convert IP address from text to binary format
    if (inet_pton(AF_INET, ip, &peer->address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(peer->socketfd);
        return -1;
    }
    peer->port = port;
    peer->ip = ip;
    return 0;
}

int send_data(Peer* peer, const uint8_t* data, size_t data_size) {
    ssize_t bytes_sent = sendto(peer->socketfd, data, data_size, 0, (struct sockaddr *)&peer->address, sizeof(peer->address));

    if (bytes_sent < 0) {
        perror("Data transmission failed");
        return -1;
    }

    return 0;
}

int send_packet(const uint8_t* payload, const size_t payload_size, Header header) {
    if (payload_size + HEADER_LEN > 1472) {
        return -1;
    }
    header.data_len = payload_size;
    memcpy(buffer + HEADER_LEN, payload, payload_size);
    buffer[0] = (uint8_t)(header.packet_type >> 24);
    buffer[1] = (uint8_t)(header.packet_type >> 16);
    buffer[2] = (uint8_t)(header.packet_type >> 8);
    buffer[3] = (uint8_t)(header.packet_type);

    buffer[4] = (uint8_t)(header.data_len >> 24);
    buffer[5] = (uint8_t)(header.data_len >> 16);
    buffer[6] = (uint8_t)(header.data_len >> 8);
    buffer[7] = (uint8_t)(header.data_len);

    buffer[8] = (uint8_t)(header.row >> 24);
    buffer[9] = (uint8_t)(header.row >> 16);
    buffer[10] = (uint8_t)(header.row >> 8);
    buffer[11] = (uint8_t)(header.row);

    buffer[12] = (uint8_t)(header.width >> 24);
    buffer[13] = (uint8_t)(header.width >> 16);
    buffer[14] = (uint8_t)(header.width >> 8);
    buffer[15] = (uint8_t)(header.width);

    buffer[16] = (uint8_t)(header.height >> 24);
    buffer[17] = (uint8_t)(header.height >> 16);
    buffer[18] = (uint8_t)(header.height >> 8);
    buffer[19] = (uint8_t)(header.height);

    buffer[20] = (uint8_t)(header.index >> 24);
    buffer[21] = (uint8_t)(header.index >> 16);
    buffer[22] = (uint8_t)(header.index >> 8);
    buffer[23] = (uint8_t)(header.index);

    return send_data(&peer, buffer, HEADER_LEN + payload_size);
}

static int send_current_palette(void)
{
    Header header = {0};
    header.packet_type = PACKET_PALETTE;

    uint8_t payload[256 * 2];
    for (size_t i = 0; i < 256; ++i) {
        uint16_t rgb565 =
            ((uint16_t)(colors[i].r & 0xf8) << 8) |
            ((uint16_t)(colors[i].g & 0xfc) << 3) |
            ((uint16_t)(colors[i].b) >> 3);

        // Serialize most-significant byte first for the display.
        payload[i * 2]     = (uint8_t)(rgb565 >> 8);
        payload[i * 2 + 1] = (uint8_t)rgb565;
    }

    return send_packet(payload, sizeof(payload), header);
}

void send_frame(pixel_t* framebuffer) {
    int remaining_lines = DOOMGENERIC_RESY;
    Header header = {0};
    header.width = DOOMGENERIC_RESX;
    header.index = 0;
    header.packet_type = PACKET_FRAME_STRIPE;

    while(remaining_lines != 0) {
        int lc = remaining_lines % 4 == 0 ? 4 : remaining_lines % 4;
        size_t source_base = (size_t)(DOOMGENERIC_RESY - remaining_lines) * DOOMGENERIC_RESX;
        size_t pixel_count = (size_t)DOOMGENERIC_RESX * (size_t)lc;
        header.row = DOOMGENERIC_RESY - remaining_lines;
        header.height = lc;
        if(send_packet(framebuffer + source_base, pixel_count, header) != 0) {
            perror("Failed to send frame stripe");
            return;
        }
        usleep(500);

        remaining_lines -= lc;
		header.index++;
    }
}

// Queue operations
static int add_key_to_queue(int pressed, unsigned char key) {
    if(queue_count == QUEUE_CAPACITY) {
        return 0;
    }

    key_queue[queue_write_index].pressed = pressed;
    key_queue[queue_write_index].key = key;

    queue_write_index = (queue_write_index + 1) % QUEUE_CAPACITY;
    ++queue_count;
    return 1;
}

static int remove_key_from_queue(KeyEvent *event) {
    if(queue_count == 0) {
        return 0;
    }
    *event = key_queue[queue_read_index];
    queue_read_index = (queue_read_index + 1) % QUEUE_CAPACITY;
    --queue_count;
    return 1;
}

static void process_button_state(uint16_t state) {
    uint16_t changed = state ^ previous_button_state;

    for(unsigned int bit = 0; bit < 9; ++bit) {
        uint16_t mask = (uint16_t)(1u << bit);
        if(changed & mask) {
            add_key_to_queue((state & mask) != 0, doom_keys[bit]);
        }
    }
    previous_button_state = state;
}

static void poll_input(void) {
    while (1) {
        uint8_t payload[2];

        ssize_t received = recvfrom(
            peer.socketfd,
            payload,
            sizeof(payload),
            MSG_DONTWAIT,
            NULL,
            NULL
        );

        if (received != sizeof(payload)) {
            break;
        }

        uint16_t state = ((uint16_t)payload[0] << 8) | payload[1];
        process_button_state(state);
    }
}

void DG_Init(void) {
    const size_t batch_size = DOOMGENERIC_RESX * (size_t)LINES_IN_BATCH;
    buffer = (uint8_t*)malloc(batch_size + HEADER_LEN);
    if (buffer == NULL) {
        fprintf(stderr, "Cannot allocate conversion buffer\n");
        exit(EXIT_FAILURE);
    }

    int rc = connect_to_peer(UDP_PORT, UDP_ADDR, &peer);
    if(rc < 0) {
        printf("Failed to connect to peer\n");
        exit(EXIT_FAILURE);
    }

    // Continue init
}

uint32_t DG_GetTicksMs(void) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    return (uint32_t)(
        (uint64_t)start.tv_sec * 1000u +
        (uint64_t)start.tv_nsec / 1000000u
    );
}

void DG_SleepMs(uint32_t ms) {
    usleep(ms*1000);
}

void DG_DrawFrame(void) {
    if(palette_changed)
        if(send_current_palette() == 0)
            palette_changed = false;

    send_frame(DG_ScreenBuffer);
}

void DG_SetWindowTitle(const char *title) {
    (void)title;
    return;
}

int DG_GetKey(int *pressed, unsigned char *key) {
    poll_input();

    KeyEvent event;
    if(!remove_key_from_queue(&event)) {
        return 0;
    }
    *pressed = event.pressed;
    *key = event.key;
    return 1;
}
