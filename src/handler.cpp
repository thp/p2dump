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

#include "priv2.h"

#include "big.h"
#include "iff.h"
#include "shp.h"
#include "fat.h"
#include "font.h"
#include "base.h"
#include "movielist.h"

namespace priv2 {
namespace handler {

bool
handle_data(const char *buf, size_t len, const std::string &filename_prefix)
{
    if (priv2::big::is_big(buf, len)) {
        priv2::big::handle_big(buf, len, filename_prefix);
    } else if (priv2::iff::is_iff(buf, len)) {
        priv2::iff::handle_iff(buf, len, filename_prefix);
    } else if (priv2::shp::is_image(buf, len)) {
        priv2::shp::decode_image(buf, len, filename_prefix);
    } else if (priv2::fat::is_sound(buf, len)) {
        priv2::fat::decode_sound(buf, len, filename_prefix);
    } else if (priv2::font::is_font(buf, len)) {
        priv2::font::decode_font(buf, len, filename_prefix);
    } else if (priv2::base::is_base(buf, len)) {
        priv2::base::handle_base(buf, len, filename_prefix);
    } else if (priv2::movielist::is_movielist(buf, len)) {
        priv2::movielist::handle_movielist(buf, len, filename_prefix);
    } else {
        return false;
    }

    return true;
}

};
};
