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

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include <png.h>

namespace priv2 {

std::string
basename(const std::string &filename)
{
    int pos = filename.rfind('/');

    if (pos == std::string::npos) {
        pos = filename.rfind('\\');
    }

    if (pos != std::string::npos) {
        return filename.substr(pos + 1);
    }

    return filename;
}

void
fail(const char *message)
{
    fprintf(stderr, "Fatal error: %s\n", message);
    exit(1);
}

void
fail(const std::string &message)
{
    fail(message.c_str());
}

CLI::CLI(int argc, char **argv)
    : argc(argc)
    , argv(argv)
{
    printf(
        "Privateer 2: The Darkening -- Data Dumper\n"
        "-----------------------------------------\n"
        "Ver 1.1 / 2017-04-25 Thomas Perl <thp.io>\n\n"
        "Usage: %s <filename> [...]\n"
        "\n"
        "Supported container formats:\n"
        " - BIGF\n"
        " - IFF\n"
        "\n"
        "Supported compression formats:\n"
        " - Deflate (using zlib)\n"
        " - 0x10fb\n"
        " - Huffman (based on HCl's decoder)\n"
        "\n"
        "Supported data formats:\n"
        " - FAT ADPCM/PCM Audio ..................... WAV (using libsndfile)\n"
        " - SHP (based on shp2bmp by Mario Brito) ... PNG (using libpng)\n"
        " - Sets Base Image ......................... PNG\n"
        " - BRender Pixmap (BRPM) ................... PNG\n"
        " - Fonts ................................... PNG\n"
        " - BRender 3D Model (BR3D) ................. OBJ/MTL\n"
        " - Indexed String list ..................... TXT\n"
        " - Movie List .............................. TXT\n"
        "\n", basename(argv[0]).c_str());
}

void
CLI::for_each(std::function<void(const std::string &, const std::string &)> handler)
{
    if (argc == 1) {
        priv2::fail("Need at least 1 filename as argument");
    }

    for (int i=1; i<argc; i++) {
        std::string filename = argv[i];
        std::string basename = priv2::basename(filename);

        handler(filename, basename);
    }
}

std::vector<char>
read_file(const std::string &filename)
{
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) fail("Could not open file");

    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::vector<char> result(len);
    fread(result.data(), result.size(), 1, fp);
    fclose(fp);

    return result;
}

void
vwrite_file(const char *buf, size_t len, const char *fmt, va_list ap)
{
    char *filename;
    vasprintf(&filename, fmt, ap);

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        priv2::fail(priv2::format("Could not open file for writing: %s", filename));
    }

    fwrite(buf, len, 1, fp);
    fclose(fp);
    free(filename);
}

void
write_file(const char *buf, size_t len, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vwrite_file(buf, len, fmt, ap);
    va_end(ap);
}

void
write_file(const std::string &str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vwrite_file(str.data(), str.size(), fmt, ap);
    va_end(ap);
}

void
write_file(const std::vector<std::string> &lines, const char *fmt, ...)
{
    std::string tmp;
    int i = 0;
    for (auto &line: lines) {
        tmp += format("== Item #%d (0x%08x) ==\n%s\n\n", i, i, line.c_str());
        i++;
    }

    va_list ap;
    va_start(ap, fmt);
    vwrite_file(tmp.data(), tmp.size(), fmt, ap);
    va_end(ap);
}

std::string
format(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *tmp;
    vasprintf(&tmp, fmt, ap);
    va_end(ap);

    std::string result = tmp;
    free(tmp);
    return result;
}

void
write_png(char *rgba_pixels, int width, int height, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *filename;
    vasprintf(&filename, fmt, ap);
    va_end(ap);

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        priv2::fail("libPNG write error");
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        priv2::fail("Could not open file for writing");
    }
    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    std::vector<png_bytep> row_pointers(height);
    for (int i=0; i<height; i++) {
        row_pointers[i] = (png_bytep)(rgba_pixels + width * 4 * i);
    }

    png_write_image(png, row_pointers.data());
    png_write_end(png, NULL);

    png_destroy_write_struct(&png, &info);

    fclose(fp);

    free(filename);
}

};
