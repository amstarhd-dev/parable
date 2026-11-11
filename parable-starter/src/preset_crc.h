#pragma once
// Adapted from bakercp/CRC32, pinned provenance/license in third_party/crc32/.
// Copyright (c) 2013 Christopher Baker <https://christopherbaker.net>
// SPDX-License-Identifier: MIT
#include <cstddef>
#include <cstdint>
namespace parable::presets::detail {
inline uint32_t crc32(const void* data, size_t bytes) noexcept {
    static constexpr uint32_t table[] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
    };
    uint32_t state = UINT32_MAX;
    const auto* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < bytes; ++i) {
        state = table[(state ^ p[i]) & 15] ^ (state >> 4);
        state = table[(state ^ (p[i] >> 4)) & 15] ^ (state >> 4);
    }
    return ~state;
}
}
