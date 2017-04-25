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

#include <string>

#include "priv2.h"
#include "handler.h"

namespace {

struct Entry {
    Entry(uint32_t offset=0, uint32_t length=0, const std::string &filename="")
        : offset(offset)
        , length(length)
        , filename(filename)
    {
    }

    uint32_t offset;
    uint32_t length;
    std::string filename;
};

}; // end anonymous namespace

namespace priv2 {
namespace big {

bool
is_big(const char *buf, size_t len)
{
    uint32_t *read_ptr = (uint32_t *)buf;
    return (len >= 4 && priv2::fourcc(*read_ptr) == "BIGF");
}

void
handle_big(const char *buf, size_t len, const std::string &filename_prefix)
{
    if (!is_big(buf, len)) {
        priv2::fail("Not a big file");
    }

    uint32_t *read_ptr = (uint32_t *)buf;

    // skip signature
    read_ptr++;

    uint32_t length = priv2::byteswap(*read_ptr++);
    uint32_t n_files = priv2::byteswap(*read_ptr++);
    uint32_t header_length = priv2::byteswap(*read_ptr++);
    printf("Length: %d bytes, %d files, %d header bytes\n",
            length, n_files, header_length);

    std::vector<Entry> entries;
    for (int i=0; i<n_files; i++) {
        uint32_t file_offset = priv2::byteswap(*read_ptr++);
        uint32_t file_length = priv2::byteswap(*read_ptr++);
        char *filename_str = (char *)read_ptr;
        entries.emplace_back(file_offset, file_length, std::string(filename_str));
        while (*filename_str) {
            filename_str++;
        }
        read_ptr = (uint32_t *)(filename_str + 1);
    }

    for (auto &entry: entries) {
        printf("Entry: offset=%u, length=%u, name='%s'\n",
                entry.offset, entry.length, entry.filename.c_str());

        const char *entry_buf = buf + entry.offset;
        uint32_t entry_len = entry.length;

        auto prefix = priv2::format("%s-%s", filename_prefix.c_str(), entry.filename.c_str());
        printf("Prefix: '%s'\n", prefix.c_str());
        priv2::handler::handle_data(entry_buf, entry_len, prefix);

        // TODO: Also pass to other handlers

        priv2::write_file(entry_buf, entry_len, "%s-%s", filename_prefix.c_str(), entry.filename.c_str());
    }
}

};
};
