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

#include "textdetect.h"

namespace {

inline bool
contains(const std::string &s, const std::initializer_list<const char *> &candidates)
{
    for (auto &c: candidates) {
        if (s.find(c) != std::string::npos) {
            return true;
        }
    }

    return false;
}

};

namespace priv2 {
namespace textdetect {

enum TextEncoding
get_text_encoding(const std::string &fn)
{
    if (fn.find("BOOTH.IFF") == 0) {
        if (contains(fn, {"BOOT-STD_-TXT1", "BOOT-LOG2-TXT1"})) {
            // raw/BOOTH.IFF-chunk-0x0018d544-BOOT-STD_-TXT1.bin: 00000000: 5945 5300 4e4f 0042 5559 0053 454c 4c00  YES.NO.BUY.SELL.
            // raw/BOOTH.IFF-chunk-0x001957ec-BOOT-LOG2-TXT1.bin: 00000000: 596f 7520 6172 6520 636c 6561 7265 6420  You are cleared
            return STRINGLIST;
        }

        if (contains(fn, {"BOOT-NEWS-TXT2", "BOOT-BBS_-TXT2",
                          "BOOT-RECD-DAT1", "BOOT-RECD-DAT2", "BOOT-RECD-DAT3", "BOOT-RECD-DAT4"})) {
            // huff/BOOTH.IFF-chunk-0x0078adb6-BOOT-NEWS-TXT2.bin: 00000000: 701d 0000 ad03 0000 4c20 0000 0700 0000  p.......L ......
            // huff/BOOTH.IFF-chunk-0x00850b2a-BOOT-BBS_-TXT2.bin: 00000000: b001 0000 3500 0000 1d04 0000 0000 0000  ....5...........
            // huff/BOOTH.IFF-chunk-0x009a95bc-BOOT-RECD-DAT1.bin: 00000000: c004 0000 9700 0000 6407 0000 0700 0000  ........d.......
            // huff/BOOTH.IFF-chunk-0x00a857a2-BOOT-RECD-DAT2.bin: 00000000: 2005 0000 a300 0000 b407 0000 0700 0000   ...............
            // huff/BOOTH.IFF-chunk-0x00b3fde8-BOOT-RECD-DAT3.bin: 00000000: d00a 0000 5901 0000 4c0d 0000 0600 0000  ....Y...L.......
            // huff/BOOTH.IFF-chunk-0x00d22272-BOOT-RECD-DAT4.bin: 00000000: 7003 0000 6d00 0000 0506 0000 0000 0000  p...m...........
            return HUFFMAN;
        }

        if (contains(fn, {"BOOT-STD_-TEXT"})) {
            // txt/BOOTH.IFF-chunk-0x0018b8f2-BOOT-STD_-TEXT.bin: 00000000: 7805 0000 1006 0000 8c06 0000 1607 0000  x...............
            return INDEXED;
        }
    }

    if (fn.find("MISSION") == 0 && fn.find(".IFF") == 8) {
        if (contains(fn, {"BOOT-TEXT"})) {
            // huff/MISSION0.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 1005 0000 a100 0000 1508 0000 0000 0000  ................
            // huff/MISSION1.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 0c08 0000 0700 0000  ................
            // huff/MISSION2.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 a407 0000 0600 0000  ................
            // huff/MISSION3.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 bc07 0000 0700 0000  ................
            // huff/MISSION4.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 e407 0000 0700 0000  ................
            // huff/MISSION5.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 ec07 0000 0700 0000  ................
            // huff/MISSION6.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 dc07 0000 0700 0000  ................
            // huff/MISSION7.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 4a08 0000 0500 0000  ........J.......
            // huff/MISSION8.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0805 0000 a000 0000 f407 0000 0600 0000  ................
            // huff/MISSION9.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 0005 0000 9f00 0000 ec07 0000 0700 0000  ................
            // huff/MISSIONM.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 880e 0000 d001 0000 0c11 0000 0500 0000  ................
            // huff/MISSIONR.IFF-chunk-0x00000014-BOOT-TEXT.bin: 00000000: 8808 0000 1001 0000 a40b 0000 0700 0000  ................
            return HUFFMAN;
        }

        if (contains(fn, {"BOOT-DATA"})) {
            // txt/MISSION0.IFF-chunk-0x00004dca-BOOT-DATA.bin: 00000000: 8800 0000 9400 0000 9a00 0000 aa00 0000  ................
            // txt/MISSION1.IFF-chunk-0x00003a68-BOOT-DATA.bin: 00000000: 9400 0000 9e00 0000 a500 0000 b700 0000  ................
            // txt/MISSION2.IFF-chunk-0x00002370-BOOT-DATA.bin: 00000000: 5c00 0000 6600 0000 7300 0000 8000 0000  \...f...s.......
            // txt/MISSION3.IFF-chunk-0x00003022-BOOT-DATA.bin: 00000000: 0801 0000 1201 0000 1f01 0000 2c01 0000  ............,...
            // txt/MISSION4.IFF-chunk-0x00002da4-BOOT-DATA.bin: 00000000: 4c00 0000 6400 0000 7400 0000 7900 0000  L...d...t...y...
            // txt/MISSION5.IFF-chunk-0x00003256-BOOT-DATA.bin: 00000000: 4800 0000 4f00 0000 6900 0000 7900 0000  H...O...i...y...
            // txt/MISSION6.IFF-chunk-0x00002cb4-BOOT-DATA.bin: 00000000: 9c00 0000 a600 0000 b000 0000 bc00 0000  ................
            // txt/MISSION7.IFF-chunk-0x00002a22-BOOT-DATA.bin: 00000000: 3400 0000 3f00 0000 4b00 0000 5f00 0000  4...?...K..._...
            // txt/MISSION8.IFF-chunk-0x00002c4c-BOOT-DATA.bin: 00000000: 5c00 0000 6a00 0000 8300 0000 9300 0000  \...j...........
            // txt/MISSION9.IFF-chunk-0x0000390e-BOOT-DATA.bin: 00000000: 4400 0000 5600 0000 6600 0000 7c00 0000  D...V...f...|...
            // txt/MISSIONM.IFF-chunk-0x0000398e-BOOT-DATA.bin: 00000000: 3c00 0000 4900 0000 5a00 0000 7500 0000  <...I...Z...u...
            // txt/MISSIONR.IFF-chunk-0x00004c94-BOOT-DATA.bin: 00000000: 3400 0000 4000 0000 4f00 0000 5c00 0000  4...@...O...\...
            return INDEXED;
        }
    }

    if (fn.find("INSTALL.IFF") == 0) {
        if (contains(fn, {"BOOT-LNG1"})) {
            // txt/INSTALL.IFF-chunk-0x000d7214-BOOT-LNG1.bin: 00000000: 6001 0000 b001 0000 ee01 0000 2c02 0000  `...........,...
            return INDEXED;
        }
    }

    if (fn.find("GAMEFLOW.IFF") == 0) {
        if (contains(fn, {"INIT-TXT2"})) {
            // huff/GAMEFLOW.IFF-chunk-0x00022876-INIT-TXT2.bin: 00000000: 9003 0000 7100 0000 e405 0000 0600 0000  ....q...........
            return HUFFMAN;
        }

        if (contains(fn, {"INIT-TEXT", "INIT-TXT3"})) {
            // txt/GAMEFLOW.IFF-chunk-0x00020d64-INIT-TEXT.bin: 00000000: 900a 0000 930a 0000 980a 0000 a00a 0000  ................
            // txt/GAMEFLOW.IFF-chunk-0x00023be6-INIT-TXT3.bin: 00000000: 5001 0000 5d01 0000 7401 0000 8d01 0000  P...]...t.......
            return INDEXED;
        }
    }

    return NONE;
}

};
};
