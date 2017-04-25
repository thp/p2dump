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

#include "text.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <string>

#include "priv2.h"
#include "codepoint.h"

namespace priv2 {
namespace text {

std::vector<std::string>
decode(const char *buf, size_t len)
{
    uint32_t *read_ptr = (uint32_t *)buf;

    uint32_t offset = *read_ptr;
    if (offset % 4 != 0) {
        priv2::fail("Expected offset divisible by 4");
    }

    uint32_t n_items = offset / 4;
    std::vector<std::string> result;
    for (int i=0; i<n_items; i++) {
        offset = *read_ptr++;
        const char *msg = buf + offset;

        std::vector<char> charbuf;
        while (*msg) {
            uint8_t codepoint = *msg;

            if (codepoint > 31 && codepoint < 127) {
                charbuf.push_back(codepoint);
            } else {
                const char *codepoint_utf8 = priv2::codepoint::get_utf8(codepoint);
                std::string tmp = codepoint_utf8 ? codepoint_utf8 : priv2::format("<0x%x>", codepoint);
                for (auto c: tmp) {
                    charbuf.push_back(c);
                }
            }

            msg++;
        }

        result.emplace_back(charbuf.data(), charbuf.size());
    }
    return result;
}

};
};
