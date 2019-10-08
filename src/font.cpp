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
#include <algorithm>

#include "priv2.h"
#include "codepoint.h"
#include "font.h"

namespace {

struct FontChar {
    FontChar(uint32_t codepoint=0, uint32_t offset=0, uint32_t width=0)
        : codepoint(codepoint)
        , offset(offset)
        , width(width)
    {}

    uint32_t codepoint;
    uint32_t offset;
    uint32_t width;
};

};


namespace priv2 {
namespace font {

bool
is_font(const char *buf, size_t len)
{
    uint32_t *read_ptr = (uint32_t *)buf;

    return (len >= 4 && memcmp(priv2::fourcc(*read_ptr).c_str(), "1.\0\0", 4) == 0);
}

void
decode_font(const char *buf, size_t len, const std::string &filename_prefix)
{
    if (!is_font(buf, len)) {
        priv2::fail("Invalid signature");
    }

    uint32_t *read_ptr = (uint32_t *)buf;

    // Skip signature
    read_ptr++;

    uint32_t num_chars = *read_ptr++;
    uint32_t height = *read_ptr++;
    uint32_t unknown = *read_ptr++;
    printf("Num chars: %d, height: %d, unknown: 0x%08x\n",
            num_chars, height, unknown);

    std::vector<FontChar> fontdef;
    for (int i=0; i<num_chars; i++) {
        uint32_t offset = *read_ptr++;
        uint16_t width = (*((uint32_t *)(buf + offset)) & 0xFFFF);
        fontdef.emplace_back(i, offset, width);
    }

    bool dump_to_console = false;

    uint32_t total_width = 1;
    const char *lut[] = {" ", "░", "▒", "▓", "█"};
    const size_t lutlen = sizeof(lut) / sizeof(lut[0]);
    for (auto &def: fontdef) {
        const char *codepoint_utf8 = priv2::codepoint::get_utf8(def.codepoint);
        std::string repr =
            codepoint_utf8 ? codepoint_utf8 :
            priv2::format("%c", (def.codepoint > 31 && def.codepoint < 127) ? def.codepoint : '.');

        if (def.width) {
            total_width += def.width + 1;

            printf("Offset of char %3d / 0x%02x (%s): 0x%08x (width = %5d)\n",
                    def.codepoint, def.codepoint, repr.c_str(), def.offset, def.width);

            if (dump_to_console) {
                uint8_t *pixel_ptr = ((uint8_t *)(buf + def.offset + 4));
                for (int y=0; y<height; y++) {
                    for (int x=0; x<def.width; x++) {
                        uint8_t value = *pixel_ptr++;
                        if (value == 0x01) {
                            // FIXUP FOR GAMEFLOW.IFF FONTS
                            value = 0xff;
                        }
                        uint32_t scaled = value * lutlen / 256;
                        printf("%s", lut[scaled]);
                    }
                    printf("\n");
                }
                printf("\n");
            }
        }
    }

    std::vector<char> tmp(total_width * height * 4);

    uint8_t max_pixel = 0;
    for (auto &def: fontdef) {
        uint8_t *pixel_ptr = ((uint8_t *)(buf + def.offset + 4));
        for (int y=0; y<height; y++) {
            for (int x=0; x<def.width; x++) {
                uint8_t value = *pixel_ptr++;
                max_pixel = std::max(max_pixel, value);
            }
        }
    }

    printf("Font pixel intensity range: 0-%d\n", max_pixel);

    std::string chardef;
    uint32_t xoffset = 0;
    for (auto &def: fontdef) {
        if (!def.width) {
            continue;
        }

        chardef += (char)def.codepoint;

        uint8_t *pixel_ptr = ((uint8_t *)(buf + def.offset + 4));
        for (int y=0; y<height; y++) {
            char *write_ptr = tmp.data() + (y * total_width + xoffset) * 4;
            *write_ptr++ = (y == 0) ? 0xFF : 0;
            *write_ptr++ = 0x00;
            *write_ptr++ = (y == 0) ? 0xFF : 0;
            *write_ptr++ = (y == 0) ? 0xFF : 0;
            for (int x=0; x<def.width; x++) {
                uint32_t value = *pixel_ptr++;
                // Scale pixel value to full intensity range
                value = value * 255 / max_pixel;
                *write_ptr++ = value & 0xFF; // r
                *write_ptr++ = value & 0xFF; // g
                *write_ptr++ = value & 0xFF; // b
                *write_ptr++ = value & 0xFF; // a
            }
            *write_ptr++ = (y == 0) ? 0xFF : 0;
            *write_ptr++ = 0x00;
            *write_ptr++ = (y == 0) ? 0xFF : 0;
            *write_ptr++ = (y == 0) ? 0xFF : 0;
        }
        xoffset += def.width + 1;
    }

    priv2::write_file(chardef, "%s-font.txt", filename_prefix.c_str());
    priv2::write_png(tmp.data(), total_width, height, "%s-font.png", filename_prefix.c_str());
}

};
};
