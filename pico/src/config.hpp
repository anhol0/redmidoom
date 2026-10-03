#pragma once
#include "wizchip_conf.h"

namespace config {

inline constexpr wiz_NetInfo network{
    .mac  = { 0xEE, 0x4A, 0x4F, 0xA1, 0x22, 0xDE },
    .ip   = { 192, 168, 1, 69 },
    .sn   = { 255, 255, 255, 0 },
    .gw   = { 192, 168, 1, 1 },
    .dns  = { 192, 168, 1, 1 },
    .dhcp = NETINFO_STATIC,
};

}
