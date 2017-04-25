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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <string>

#include "priv2.h"
#include "fb10.h"
#include "palette.h"
#include "base.h"

namespace priv2 {
namespace base {

bool
is_base(const char *buf, size_t len)
{
    constexpr size_t PALETTE_SIZE = 256 * 3;

    if (len < PALETTE_SIZE) {
        return false;
    }

    return priv2::fb10::is_compressed(buf + PALETTE_SIZE, len - PALETTE_SIZE);
}

void
handle_base(const char *buf, size_t len, const std::string &filename_prefix)
{
    if (!is_base(buf, len)) {
        priv2::fail("Is not a base file");
    }

    constexpr size_t PALETTE_SIZE = 256 * 3;

    int width = 640;
    int height = 480;

    priv2::write_file(buf, PALETTE_SIZE, "%s.pal", filename_prefix.c_str());

    priv2::gfx::Palette pal;
    pal.raw_from_buffer(buf, PALETTE_SIZE);

    auto dec = priv2::fb10::decompress(buf + PALETTE_SIZE, len - PALETTE_SIZE);
    uint8_t *pixel_ptr = (uint8_t *)dec.data();

    std::vector<char> tmp(width * height * 4);

    for (int y=0; y<height; y++) {
        uint32_t *write_ptr = (uint32_t *)(tmp.data() + (y * width) * 4);
        for (int x=0; x<width; x++) {
            uint8_t value = *pixel_ptr++;
            uint32_t rgba = pal.lookup(value);
            *write_ptr++ = rgba;
        }
    }

    priv2::write_png(tmp.data(), width, height, "%s-base.png", filename_prefix.c_str());
}

};
};
