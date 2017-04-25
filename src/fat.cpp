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
#include <sndfile.h>

#include <string>

#include "priv2.h"
#include "fat.h"

namespace {

struct VirtualIO {
    VirtualIO(const char *buf, size_t len) : buf(buf), len(len), pos(0) {}

    size_t get_filelen() { return len; }

    size_t seek(size_t offset, int whence)
    {
        switch (whence) {
            case SEEK_SET:
                pos = 0 + offset;
                break;
            case SEEK_END:
                pos = len + offset;
                break;
            case SEEK_CUR:
                pos = pos + offset;
            default:
                priv2::fail("Invalid seek");
                break;

        }

        return pos;
    }

    size_t read(char *buf, size_t size)
    {
        size_t available = (len - pos);
        if (available < size) {
            size = available;
        }
        memcpy(buf, this->buf + pos, size);
        pos += size;
        return size;
    }

    size_t write(const void *, size_t size)
    {
        priv2::fail("Write not supported");
        return 0;
    }

    size_t tell() { return pos; }

    const char *buf;
    size_t len;
    size_t pos;
};

static sf_count_t
VirtualIO_sf_vio_get_filelen(void *user_data)
{
    return static_cast<VirtualIO *>(user_data)->get_filelen();
}

static sf_count_t
VirtualIO_sf_vio_seek(sf_count_t offset, int whence, void *user_data)
{
    return static_cast<VirtualIO *>(user_data)->seek(offset, whence);
}

static sf_count_t
VirtualIO_sf_vio_read(void *ptr, sf_count_t count, void *user_data)
{
    return static_cast<VirtualIO *>(user_data)->read((char *)ptr, count);
}

static sf_count_t
VirtualIO_sf_vio_write(const void *ptr, sf_count_t count, void *user_data)
{
    return static_cast<VirtualIO *>(user_data)->write(ptr, count);
}

static sf_count_t
VirtualIO_sf_vio_tell(void *user_data)
{
    return static_cast<VirtualIO *>(user_data)->tell();
}

static SF_VIRTUAL_IO
VirtualIO_SoundFileVtable = {
    VirtualIO_sf_vio_get_filelen,
    VirtualIO_sf_vio_seek,
    VirtualIO_sf_vio_read,
    VirtualIO_sf_vio_write,
    VirtualIO_sf_vio_tell,
};

struct SoundChunk {
    enum EncodingFormat {
        ENCODING_PCM_16_BIT = 0x00,
        ENCODING_ADPCM = 0x01,
    };

    enum SampleRate {
        SAMPLE_RATE_11KHZ = 0x00,
        SAMPLE_RATE_22KHZ = 0x01,
        SAMPLE_RATE_16KHZ = 0x03,
    };

    enum FlagIndex {
        FLAG_ENCODING_FORMAT = 0,
        FLAG_UNKNOWN_1 = 1,
        FLAG_UNKNOWN_2 = 2,
        FLAG_UNKNOWN_3 = 3,
        FLAG_UNKNOWN_4 = 4,
        FLAG_UNKNOWN_5 = 5,
        FLAG_UNKNOWN_6 = 6,
        FLAG_UNKNOWN_7 = 7,
        FLAG_SAMPLE_RATE = 8,
    };

    SoundChunk(uint32_t offset, uint32_t uncompressed_size, uint8_t flags[9])
        : offset(offset)
        , uncompressed_size(uncompressed_size)
        , flags()
    {
        for (int i=0; i<9; i++) {
            this->flags[i] = flags[i];
        }
    }

    bool is_adpcm() {
        return (flags[FLAG_ENCODING_FORMAT] == ENCODING_ADPCM);
    }

    const char *get_encoding_name() {
        switch (flags[FLAG_ENCODING_FORMAT]) {
            case ENCODING_PCM_16_BIT: return "16-bit PCM  ";
            case ENCODING_ADPCM:      return " 4-bit ADPCM";
            default: priv2::fail(priv2::format("Unknown encoding format: 0x%02x", flags[FLAG_ENCODING_FORMAT]));
        }

        return nullptr;
    }

    uint32_t get_samplerate()
    {
        switch (flags[FLAG_SAMPLE_RATE]) {
            case SAMPLE_RATE_11KHZ: return 11025;
            case SAMPLE_RATE_22KHZ: return 22050;
            case SAMPLE_RATE_16KHZ: return 16000;
            default: priv2::fail(priv2::format("Unknown samplerate flag: 0x%02x", flags[FLAG_SAMPLE_RATE]));
        }

        return 0;
    }

    uint32_t total_samples()
    {
        return uncompressed_size / 2;
    }

    uint32_t compressed_size()
    {
        switch (flags[FLAG_ENCODING_FORMAT]) {
            case ENCODING_PCM_16_BIT: return uncompressed_size;
            case ENCODING_ADPCM: return total_samples() / 2;
            default: priv2::fail(priv2::format("Unknown encoding format: 0x%02x", flags[FLAG_ENCODING_FORMAT]));
        }

        return 0;
    }

    uint32_t offset;
    uint32_t uncompressed_size;
    uint8_t flags[9];
};

}; // end anonymous namespace

namespace priv2 {
namespace fat {

bool
is_sound(const char *buf, size_t len)
{
    uint32_t *read_ptr = (uint32_t *)buf;
    return (priv2::fourcc(*read_ptr) == "1.00");
}

void
decode_sound(const char *buf, size_t len, const std::string &filename_prefix)
{
    if (!is_sound(buf, len)) {
        priv2::fail("Not a sound");
    }

    // layout:
    // 4 bytes "1.00"
    // 4 bytes <nitems>
    // 16 bytes header per item

    uint32_t *read_ptr = (uint32_t *)buf;

    // skip signature
    read_ptr++;

    // DANI.BIG-ARMOUR1.FAT --> 8.450 bytes --> ca 16.900 SAMPLES
    // numSamplesMaybe /2 = 33706/2 = 16853 (so maybe decompressed 16 bit samples?)
    // a=         1 b=        24 c=       256 (cBE=         1) numSamplesMaybe=     33706

    // SIG: 312e 3030  "1.00"                                      4 BYTES (HEADER)
    // 0100 0000       a=LE 1                                      4 BYTES (NUMBER OF FILES?)
    // 1800 0000       b=LE 24                                     4 BYTES (OFFSET)
    // 9e79 00 ... 01       uncompressed_size (3 bytes), cb (1 byte)  4 BYTES (METADATA)
    // 0001 0a00       ???                                         4 BYTES (???)
    // 0000 0100       ???                                         4 BYTES (???)
    // 0100 077a 1900 b382 d22d 1988 993f 92aa 3313 b291 8c1b 69a7 911b

    // For samplerate 22050 (e.g. ISETS.IFF-ANHUR_TF.IFF-chunk-0x0000002e-TORF-0003-SPCH.bin)
    // unk0=0x00000001
    // unk1=0x01010001
    // For samplerate 11025 (e.g. SPEECH.BIG-W15_5A.fat)
    // unk0=0x0000000a
    // unk1=0x00010001 -or-
    // unk1=0x00010101

    uint32_t number_of_items = *read_ptr++;
    printf("File size: %d, number of sounds in file: %d\n", (int)len, number_of_items);

    std::vector<SoundChunk> sounds;
    for (int i=0; i<number_of_items; i++) {
        uint32_t offset = *read_ptr++;

        uint32_t tmp = *read_ptr++;
        uint32_t uncompressed_size = tmp & 0xFFFFFF;

        uint8_t flag0 = (tmp >> 24) & 0xFF; // always 0x01, except sometimes in SPACE.IFF (0x00)

        uint8_t *read8_ptr = (uint8_t *)read_ptr;
        uint8_t flag1 = *read8_ptr++; // 0x0a?
        uint8_t flag2 = *read8_ptr++; // 0x00?
        uint8_t flag3 = *read8_ptr++; // 0x00?
        uint8_t flag4 = *read8_ptr++; // 0x00?
        read_ptr++;

        uint8_t flag5 = *read8_ptr++; // 0x01?
        uint8_t flag6 = *read8_ptr++; // 0x00? (SPEECH.BIG-W15_5A.fat 0x01?) -> in that case, the sampling rate is something different
        uint8_t flag7 = *read8_ptr++; // 0x01?
        uint8_t flag8 = *read8_ptr++; // 0x00?
        read_ptr++;

        uint8_t flags[] = {
            flag0,
            flag1, flag2, flag3, flag4,
            flag5, flag6, flag7, flag8,
        };

        sounds.emplace_back(offset, uncompressed_size, flags);
    }

    int i = 0;
    for (auto &sound: sounds) {
        auto output_filename = priv2::format("%s-snd%d.wav", filename_prefix.c_str(), i);

        // This is the only file with sound.flags[6] == 0x01
        // Prefix: 'SPEECH.BIG-W15_5A.fat' (German Privateer 2)
        // File size: 15452, number of sounds in file: 1
        // Fatal error: Sound 0: Unexpected mid flags=[00 00 00 01 01 01]
        // Also seems to happen in German version of: SETS.IFF-CRIUS.IFF-chunk-0x00000014-ROOM-FATF
        if (sound.flags[6] == 0x01) {
            sound.flags[6] = 0x00;
            if (sound.flags[SoundChunk::FLAG_SAMPLE_RATE] == SoundChunk::SAMPLE_RATE_11KHZ) {
                printf("Fixing up sampling rate -> 11kHz->22kHz (flags[6] == 0x01) !! THIS IS A HACK !!\n");
                sound.flags[SoundChunk::FLAG_SAMPLE_RATE] = SoundChunk::SAMPLE_RATE_22KHZ;
            } else {
                printf("WARNING: Do not know how to handle this yet, please report: %s\n",
                        output_filename.c_str());
            }
        }

        if (sound.flags[2] != 0x00 || sound.flags[3] != 0x00 || sound.flags[4] != 0x00 || sound.flags[5] != 0x01 ||
                sound.flags[6] != 0x00 || sound.flags[7] != 0x01) {
            priv2::fail(priv2::format("Sound %d: Unexpected mid flags=[%02x %02x %02x %02x %02x %02x]",
                   i,
                   sound.flags[2], sound.flags[3], sound.flags[4],
                   sound.flags[5], sound.flags[6], sound.flags[7]));
        }

        if (sound.flags[0] != 0x00 && sound.flags[0] != 0x01) {
            priv2::fail(priv2::format("Sound %d: Unexpected flags[0]=%02x", i, sound.flags[0]));
        }

        printf("Sound %4d: off=0x%08x size=%6d encoding=%s flags=[%02x] rate=%d samples=%d -> %s\n",
               i, sound.offset, sound.uncompressed_size, sound.get_encoding_name(), sound.flags[1],
               sound.get_samplerate(), sound.total_samples(), output_filename.c_str());

        //priv2::write_file(buf + sound.offset, sound.compressed_size(), "%s.pcm", output_filename.c_str());

        VirtualIO vio(buf + sound.offset, sound.compressed_size());

        SF_INFO ininfo;
        memset(&ininfo, 0, sizeof(ininfo));
        ininfo.samplerate = sound.get_samplerate();
        ininfo.channels = 1;
        ininfo.format = SF_FORMAT_RAW | (sound.is_adpcm() ? SF_FORMAT_VOX_ADPCM : SF_FORMAT_PCM_16) | SF_ENDIAN_LITTLE;
        SNDFILE *insnd = sf_open_virtual(&VirtualIO_SoundFileVtable, SFM_READ, &ininfo, &vio);

        SF_INFO outinfo;
        memset(&outinfo, 0, sizeof(outinfo));
        outinfo.samplerate = ininfo.samplerate;
        outinfo.channels = ininfo.channels;
        outinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_FILE;
        SNDFILE *outsnd = sf_open(output_filename.c_str(), SFM_WRITE, &outinfo);

        std::vector<short> samples(1024);
        size_t count = 0;
        while ((count = sf_read_short(insnd, samples.data(), samples.size())) != 0) {
            sf_write_short(outsnd, samples.data(), count);
        }

        sf_close(outsnd);
        sf_close(insnd);

        i++;
    }
}

};
};
