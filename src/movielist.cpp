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
#include "movielist.h"

namespace priv2 {
namespace movielist {

bool
is_movielist(const char *buf, size_t len)
{
    if (len < 15 || len % 15 != 0) {
        return false;
    }

    std::string first_filename(buf + 2, 13);
    return first_filename.find(".tgv") != std::string::npos;
}

void
handle_movielist(const char *buf, size_t len, const std::string &filename_prefix)
{
    if (!is_movielist(buf, len)) {
        priv2::fail("Not a movie list");
    }

    const uint8_t *start_ptr = (const uint8_t *)buf;
    const uint8_t *end_ptr = (const uint8_t *)(buf + len);
    const uint8_t *read_ptr = start_ptr;

    std::string lines = "== Movie List ==\n";

    int i = 0;
    while (read_ptr < end_ptr) {
        const uint16_t cd_index = *((const uint16_t *)read_ptr);
        read_ptr += 2;
        std::string filename = (const char *)read_ptr;
        read_ptr += 13;

        lines += priv2::format(" Entry %3d: CD %d, Filename: '%s'\n", i, cd_index, filename.c_str());
        i++;
    }

    printf("%s\n", lines.c_str());
    priv2::write_file(lines, "%s-movielist.txt", filename_prefix.c_str());
}

};
};
