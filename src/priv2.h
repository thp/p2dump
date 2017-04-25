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

#include <vector>
#include <string>
#include <functional>

namespace priv2 {

inline uint32_t byteswap(uint32_t value)
{
    return ((value >> 24) & 0xFF) |
           ((value >> 8) & 0xFF00) |
           ((value << 8) & 0xFF0000) |
           ((value << 24) & 0xFF000000);
}

std::string basename(const std::string &filename);

struct CLI {
    CLI(int argc, char **argv);

    void for_each(std::function<void(const std::string &, const std::string &)> handler);

    int argc;
    char **argv;
};

static inline std::string
fourcc(uint32_t value)
{
    return std::string((char *)&value, 4);
}

void fail(const char *message);
void fail(const std::string &message);

std::vector<char> read_file(const std::string &filename);
void vwrite_file(const char *buf, size_t len, const char *fmt, va_list ap);
void write_file(const char *buf, size_t len, const char *fmt, ...);
void write_file(const std::string &str, const char *fmt, ...);
void write_file(const std::vector<std::string> &lines, const char *fmt, ...);
void write_png(char *rgba_pixels, int width, int height, const char *fmt, ...);
std::string format(const char *fmt, ...);

};
