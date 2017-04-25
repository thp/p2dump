/*
 * Privateer 2: The Darkening -- Data Dumper
 * Copyright (c) 2016, 2017, Thomas Perl
 *
 * SHP image decoding code based on the example code from
 * http://digitality.comyr.com/flyboy/lb2/progs/spr2bmp_source.zip
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

#include <string>

#include "priv2.h"
#include "palette.h"

#include "shp.h"

namespace {

struct SubImage {
    SubImage(uint32_t offset) : offset(offset), size(0) {}

    bool is_palette() { return size == 3 * 256; }

    uint32_t offset;
    uint32_t size;
};

void unpack_image(uint8_t *in, uint8_t *out, uint32_t width, uint32_t height);

void unpack_image(uint8_t *in, uint8_t *out, uint32_t width, uint32_t height)
{
    uint8_t *read_ptr = in;
    uint8_t *write_ptr = out;
    uint32_t y = 0;

    while (y < height) {
        uint8_t b = *read_ptr++;
        char lsb = (b & 1);
        b >>= 1;

        if (lsb == 0) {
            if (b == 0) {
                // Next line; reposition write ptr
                y++;
                write_ptr = out + y * width;
            } else {
                // Write next byte <b> times
                memset(write_ptr, *read_ptr++, b);
                write_ptr += b;
            }
        } else {
            if (b == 0) {
                // Read <n> and skip <n> bytes in output
                write_ptr += *read_ptr++;
            } else {
                // Copy <b> bytes into output
                memcpy(write_ptr, read_ptr, b);
                read_ptr += b;
                write_ptr += b;
            }
        }
    }
}


}; // end anonymous namespace

namespace priv2 {
namespace shp {

bool
is_image(const char *buf, size_t len)
{
    uint32_t *read_ptr = (uint32_t *)buf;
    return (len >= 4 && priv2::fourcc(*read_ptr++) == "1.40");
}

void
decode_image(const char *buf, size_t len, const std::string &filename_prefix)
{
    if (!is_image(buf, len)) {
        priv2::fail("Not a SHP image");
    }

    priv2::gfx::Palette palette;
    //pal.raw_from_file("SETS.IFF-ANHUR.IFF-chunk-0x000a9a32-ROOM-0008-BASE.bin");

    uint32_t *read_ptr = (uint32_t *)buf;

    // Skip signature
    read_ptr++;

    uint32_t number_of_items = *read_ptr++;
    printf("File size: %d, number of images in file: %d\n", (int)len, number_of_items);

    std::vector<SubImage> images;
    for (int i=0; i<number_of_items; i++) {
        uint32_t offset = *read_ptr++;
        uint32_t zero = *read_ptr++;
        if (zero != 0) {
            priv2::fail("Expected zero here");
        }

        if (images.size()) {
            auto &prev = images.back();
            prev.size = offset - prev.offset;
        }

        images.emplace_back(offset);
    }

    images.back().size = len - images.back().offset;

    int i = 0;
    for (auto &image: images) {
        if (image.is_palette()) {
            printf("Palette @ index %d, offset 0x%08x (%d bytes)\n", i, image.offset, image.size);
            priv2::write_file(buf + image.offset, image.size, "%s-palette-0x%08x.pal",
                    filename_prefix.c_str(), image.offset);
            palette.raw_from_buffer(buf + image.offset, image.size);
        }
        i++;
    }

    i = 0;
    for (auto &image: images) {
        if (image.is_palette()) {
            i++;
            continue;
        }

        int32_t *local_ptr = (int32_t *)(buf + image.offset + 8);

        int32_t displacementX = *local_ptr++;
        int32_t displacementY = *local_ptr++;
        int32_t xDim = 1 + *local_ptr++; // don't forget to inc!
        int32_t yDim = 1 + *local_ptr++;

        // allocate necessary memory for output
        int32_t width = xDim-displacementX;
        int32_t height = yDim-displacementY;
        size_t buffer_size = width * height;

        // save bmp
        auto filename = priv2::format("%s-%d.png", filename_prefix.c_str(), i);

        printf("Image %d: %dx%d size=%d, (displacement x=%d, y=%d) -> %s\n", i,
                width, height, image.size,
                displacementX, displacementY, filename.c_str());

        if (displacementX > 1024 || displacementY > 1024) {
            // Image 60: 3x3 size=24, (displacement x=2147483647, y=2147483335) -> SETS.IFF-BEX.IFF-chunk-0x001a08a8-ROOM-0018-OBJS-SHAP-GRAF.bin-60.png
            printf("NEED TO LOOK INTO THIS, SKIPPING\n");
            continue;
        }

        std::vector<uint8_t> output(buffer_size);

        // unpack data
        unpack_image((uint8_t *)local_ptr, output.data(), width, height);

        priv2::gfx::save_png(palette, width, height, output.data(), filename);

        i++;
    }
}

};
};
