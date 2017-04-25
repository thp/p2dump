/*
 * Privateer 2: The Darkening -- Data Dumper
 * Copyright (c) 2016, 2017, Thomas Perl
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <cstdint>
#include <string>

namespace priv2 {
namespace gfx {

struct Palette {
    Palette();
    ~Palette();

    void raw_from_file(const std::string &filename);
    void raw_from_buffer(const char *buf, size_t len);

    uint32_t lookup(uint8_t index);

    uint8_t palette[3 * 256];
    bool is_raw;
};

void save_png(Palette &palette, uint32_t width, uint32_t height,
        uint8_t *output, const std::string &filename);

void save_png(uint32_t width, uint32_t height,
        uint8_t *output, const std::string &filename);

};
};
