/*
 * Privateer 2: The Darkening -- Data Dumper
 * Copyright (c) 2016, 2017, Thomas Perl
 *
 * fb10 decompression algorihm Based on the description at
 * http://simswiki.info/wiki.php?title=Sims_3:DBPF/Compression
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

#include "fb10.h"

#include <cstdint>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <string>

#include "priv2.h"

namespace priv2 {
namespace fb10 {

bool
is_compressed(const char *buf, size_t len)
{
    if (len < 2) {
        return false;
    }

    uint8_t *read_ptr = (uint8_t *)buf;
    uint8_t sig0 = *read_ptr++;
    uint8_t sig1 = *read_ptr++;

    return (sig0 == 0x10 && sig1 == 0xfb);
}

std::vector<char>
decompress(const char *buf, size_t len)
{
    if (!is_compressed(buf, len)) {
        priv2::fail("Invalid compression detected");
    }

    std::vector<char> out;

    uint8_t *read_ptr = (uint8_t *)buf;
    uint8_t *end_ptr = (uint8_t *)buf + len;

    // Skip over signature
    read_ptr += 2;

    uint8_t size2 = *read_ptr++;
    uint8_t size1 = *read_ptr++;
    uint8_t size0 = *read_ptr++;
    uint32_t uncompressed_size = (size2 << 16 | size1 << 8 | size0);
    printf("Uncompressed size: %d (compressed size: %d)\n",
            uncompressed_size, (int)len);

    while (read_ptr < end_ptr) {
        uint8_t byte0 = *read_ptr++;

        uint32_t num_plain_text = 0;
        uint32_t num_to_copy = 0;
        uint32_t copy_offset = 1;

        if (byte0 >= 0x00 && byte0 <= 0x7f) {
            uint8_t byte1 = *read_ptr++;

            num_plain_text = byte0 & 0x03;
            num_to_copy = ((byte0 & 0x1C) >> 2) + 3;
            copy_offset = ((byte0 & 0x60) << 3) + byte1 + 1;
        } else if (byte0 >= 0x80 && byte0 <= 0xbf) {
            uint8_t byte1 = *read_ptr++;
            uint8_t byte2 = *read_ptr++;

            num_plain_text = ((byte1 & 0xc0) >> 6) & 0x03;
            num_to_copy = (byte0 & 0x3F) + 4;
            copy_offset = ((byte1 & 0x3f) << 8) + byte2 + 1;
        } else if (byte0 >= 0xc0 && byte0 <= 0xdf) {
            uint8_t byte1 = *read_ptr++;
            uint8_t byte2 = *read_ptr++;
            uint8_t byte3 = *read_ptr++;

            num_plain_text = byte0 & 0x03;
            num_to_copy = ((byte0 & 0x0c) << 6) + byte3 + 5;
            copy_offset = ((byte0 & 0x10) << 12) + (byte1 << 8) + byte2 + 1;
        } else if (byte0 >= 0xe0 && byte0 <= 0xfb) {
            int count = (byte0 - 0xdf) * 4;
            for (int i=0; i<count; i++) {
                out.push_back(*read_ptr++);
            }
            continue;
        } else if (byte0 >= 0xfc && byte0 <= 0xff) {
            num_plain_text = byte0 & 0x03;
            num_to_copy = 0;
        } else {
            printf("Unhandled control character: 0x%02x\n", byte0);
            priv2::fail("TODO");
        }

        for (int i=0; i<num_plain_text; i++) {
            out.push_back(*read_ptr++);
        }

        size_t cur_offset = out.size() - copy_offset;
        for (int i=0; i<num_to_copy; i++) {
            out.push_back(out[cur_offset++]);
        }
    }

    return out;
}

};
};
