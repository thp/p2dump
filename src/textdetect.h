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

#include <string>

namespace priv2 {
namespace textdetect {

enum TextEncoding {
    NONE = 0, // probably not encoded text
    STRINGLIST = 1, // '\0'-separated raw strings
    HUFFMAN = 2, // huffman-based encoding
    INDEXED = 3, // indexed raw strings
};

enum TextEncoding
get_text_encoding(const std::string &fn);

};
};
