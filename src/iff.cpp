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
#include <math.h>

#include <string>

#include "priv2.h"
#include "huffman.h"
#include "fb10.h"
#include "deflate.h"
#include "text.h"
#include "iff.h"
#include "fat.h"
#include "font.h"
#include "shp.h"
#include "handler.h"
#include "palette.h"
#include "textdetect.h"

namespace {

struct FormChunk {
    FormChunk(const std::string &sig, std::vector<char> &&content)
        : sig(sig)
        , content()
    {
        std::swap(this->content, content);
    }

    std::string sig;
    std::vector<char> content;
};

struct Form {
    Form(const std::string &sig) : sig(sig) {}

    FormChunk *get_chunk(const std::string &signature) {
        for (auto &chunk: chunks) {
            if (chunk.sig == signature) {
                return &chunk;
            }
        }

        return nullptr;
    }

    bool has_chunk(const std::string &signature) {
        return get_chunk(signature) != nullptr;
    }

    template <typename T>
    T *get_chunk_as(const std::string &signature) {
        auto chunk = get_chunk(signature);
        if (chunk == nullptr) {
            priv2::fail("Could not get chunk");
        }

        return (T *)chunk->content.data();
    }

    std::string sig;
    std::vector<FormChunk> chunks;
};

class IFF {
public:
    IFF(const char *buf, size_t len, const std::string &filename_prefix)
        : buf(buf)
        , len(len)
        , filename_prefix(filename_prefix)
    {}

    bool is_iff();

    void parse();

    void handle_form(const std::string &path_sig, const std::string &sig,
            uint32_t offset, const char *form_buf, size_t form_len);

    void handle_chunk(const std::string &path_sig, const std::string &form_sig, const std::string &sig,
            size_t offset, char *buf, uint32_t len);

    void handle_complete_form(Form &form);

private:
    const char *buf;
    size_t len;
    std::string filename_prefix;
};

void
IFF::handle_chunk(const std::string &path_sig, const std::string &form_sig, const std::string &sig,
        size_t offset, char *buf, uint32_t len)
{
    auto basename = priv2::format("%s-chunk-%#010x-%s%s%s-%s", filename_prefix.c_str(), (uint32_t)offset,
            path_sig.c_str(), (path_sig.empty() ? "" : "-"), form_sig.c_str(), sig.c_str());

    if (sig != "FORM") {
        // Do not write out FORM chunks, as we handle them below
        //priv2::write_file(buf, len, "%s.bin", basename.c_str());
    }

    if (sig == "BMTD") {
        printf("That would be XMI MIDI\n");
        priv2::write_file(buf, len, "%s-midi.xmi", basename.c_str());
    } else if (priv2::handler::handle_data(buf, len, basename)) {
        // Handled
    } else if (sig == "FORM") {
        handle_form(path_sig + (path_sig.empty() ? "" : "-") + form_sig,
                sig, offset, buf, len);
    } else {
        std::vector<std::string> decoded_text;

        auto text_encoding = priv2::textdetect::get_text_encoding(basename);
        switch (text_encoding) {
            case priv2::textdetect::NONE:
                printf("Unhandled chunk of %d bytes\n", len);
                break;
            case priv2::textdetect::STRINGLIST:
                {
                    char *endptr = buf + len;
                    char *pos = buf;
                    char *current = pos;
                    while (pos < endptr) {
                        if (*pos == '\0') {
                            decoded_text.emplace_back(current);
                            current = pos + 1;
                        }

                        pos++;
                    }
                }
                break;
            case priv2::textdetect::HUFFMAN:
                {
                    auto huffman_result = priv2::huffman::decode(buf, len);
                    decoded_text = huffman_result.items;
                    priv2::write_file(huffman_result.graphviz_dot_src, "%s-huffman.dot", basename.c_str());
                }
                break;
            case priv2::textdetect::INDEXED:
                decoded_text = priv2::text::decode(buf, len);
                break;
            default:
                priv2::fail("Unhandled text encoding");
        }

        if (decoded_text.size()) {
            priv2::write_file(decoded_text, "%s-lines.txt", basename.c_str());
        }
    }
}

struct Face {
    Face(int a, int b, int c) : a(a), b(b), c(c) {}

    int a;
    int b;
    int c;
};

struct BRMaterial {
    BRMaterial(const std::string &name, const std::string &colormap) : name(name), colormap(colormap), faces() {}

    std::string name;
    std::string colormap;
    std::vector<Face> faces;
};

void
IFF::handle_complete_form(Form &form)
{
    if (form.sig == "BR3D") {
        printf("Handling BRender 3D Model\n");

        std::vector<BRMaterial> materials;
        for (auto &child: form.chunks) {
            if (child.sig == "FORM") {
                uint8_t *end_ptr = (uint8_t *)(child.content.data() + child.content.size());
                uint32_t *read_ptr = (uint32_t *)child.content.data();
                auto form_name = priv2::fourcc(*read_ptr++);
                if (form_name == "BMAT") {
                    std::string name;
                    std::string colormap;
                    while ((uint8_t *)read_ptr < end_ptr) {
                        auto element_name = priv2::fourcc(*read_ptr++);
                        uint32_t length = priv2::byteswap(*read_ptr++);
                        if (element_name == "MNAM" || element_name == "MCMP") {
                            printf("BMAT Element: %s (length=%d) -> '%s'\n", element_name.c_str(), length,
                                    (char *)read_ptr);
                            if (element_name == "MNAM") {
                                name = (char *)read_ptr;
                            } else if (element_name == "MCMP") {
                                colormap = (char *)read_ptr;
                            }
                        }
                        read_ptr = (uint32_t *)(((uint8_t *)read_ptr) + length + (length % 2));
                    }
                    materials.emplace_back(name, colormap);
                }
            }
        }

        std::string mtlsrc;
        for (auto &material: materials) {
            printf("Material: '%s' -> '%s'\n", material.name.c_str(), material.colormap.c_str());

            std::string cmap = "SPACETEX.IFF-";
            for (auto c: material.colormap) {
                if (c == '.') {
                    // remove ".pix" extension
                    break;
                }
                cmap += tolower(c);
            }
            cmap += ".iff-brpm.png";

            mtlsrc += priv2::format("newmtl %s\n", material.name.c_str());
            mtlsrc += "Ka 1.000 1.000 1.000\n";
            mtlsrc += "Kd 1.000 1.000 1.000\n";
            mtlsrc += "Ks 0.000 0.000 0.000\n";
            mtlsrc += "d 1.0\n";
            mtlsrc += "illum 2\n";
            mtlsrc += priv2::format("map_Ka %s\n", cmap.c_str());
            mtlsrc += priv2::format("map_Kd %s\n", cmap.c_str());
            mtlsrc += "\n";
        }

        auto mtl_filename = priv2::format("%s-mesh.mtl", filename_prefix.c_str());
        priv2::write_file(mtlsrc, "%s", mtl_filename.c_str());

        std::string name = form.get_chunk_as<const char>("3DNM");
        uint16_t n_vertices = *(form.get_chunk_as<uint16_t>("3VTS"));
        uint16_t n_materials = *(form.get_chunk_as<uint16_t>("MATS"));
        uint16_t n_faces = *(form.get_chunk_as<uint16_t>("3FCS"));
        uint16_t flags = *(form.get_chunk_as<uint16_t>("3FLG"));
        printf("Model name: '%s', vertices: %d, faces: %d, materials: %d, flags: 0x%04x\n",
                name.c_str(), n_vertices, n_faces, n_materials, flags);

        std::string objsrc = priv2::format("usemtl %s\n", mtl_filename.c_str());

        auto vertices = form.get_chunk("VERS");
        printf("Vertices size: %d (%d bytes / vertex), %d floats / vertex\n",
                (int)vertices->content.size(), (int)vertices->content.size() / n_vertices,
                (int)(vertices->content.size() / n_vertices / sizeof(float)));

        if (vertices->content.size() != n_vertices * 10 * sizeof(float)) {
            priv2::fail("Invalid vertices data size");
        }

        auto face_materials = form.get_chunk("FMTS");
        if (face_materials->content.size() != n_faces * 32) {
            priv2::fail("Invalid face material size");
        }

        for (int i=0; i<n_faces; i++) {
            char *matname = face_materials->content.data() + i * 32;
            printf("Face material: '%s'\n", matname);
        }

        std::string vtxsrc;

        float *vertexdata = form.get_chunk_as<float>("VERS");
        for (int i=0; i<n_vertices; i++) {
            vtxsrc += priv2::format("v %.10f %.10f %.10f\n", vertexdata[0], vertexdata[1], vertexdata[2]);
            vtxsrc += priv2::format("vt %.10f %.10f\n", vertexdata[3], 1.f-vertexdata[4]);
            //vtxsrc += priv2::format("vn %f %f %f\n", -vertexdata[7], -vertexdata[8], -vertexdata[9]);
            printf("Vtx[%d]: ", i);
            for (int j=0; j<10; j++) {
                printf(" %6.2f ", vertexdata[j]);
            }
            float length = sqrtf(
                    vertexdata[7] * vertexdata[7] +
                    vertexdata[8] * vertexdata[8] +
                    vertexdata[9] * vertexdata[9]
            );
            printf(" normal length=%.2f\n", length);
            vertexdata += 10;
        }

        auto faces = form.get_chunk("FACS");
        printf("Faces size: %d (%d bytes / face)\n",
                (int)faces->content.size(), (int)faces->content.size() / n_faces);
        if (faces->content.size() != n_faces * 18 * sizeof(uint16_t)) {
            priv2::fail("Invalid faces data size");
        }

        uint16_t *facedata = form.get_chunk_as<uint16_t>("FACS");
        for (int i=0; i<n_faces; i++) {
            std::string matname = face_materials->content.data() + i * 32;

            BRMaterial *mat = nullptr;
            for (auto &m: materials) {
                if (m.name == matname) {
                    mat = &m;
                    break;
                }
            }

            if (mat == nullptr) {
                priv2::fail(priv2::format("Could not find material: %s", matname.c_str()));
            }

            mat->faces.emplace_back(facedata[0]+1, facedata[1]+1, facedata[2]+1);
            printf("Face[%d]: ", i);
            for (int j=0; j<18; j++ ){
                printf(" %5d", facedata[j]);
            }
            printf("\n");
            facedata += 18;
        }

        objsrc += vtxsrc;
        for (auto &mat: materials) {
            objsrc += priv2::format("o %s-%s\n", name.c_str(), mat.name.c_str());
            objsrc += priv2::format("usemtl %s\n", mat.name.c_str());
            for (auto &face: mat.faces) {
                objsrc += priv2::format("f %d/%d %d/%d %d/%d\n",
                        face.a, face.a,
                        face.b, face.b,
                        face.c, face.c);
            }
        }

        priv2::write_file(objsrc, "%s-mesh.obj", filename_prefix.c_str());
    }

    if (form.sig == "BRPM" && form.has_chunk("PMIF") && form.has_chunk("PMDT")) {
        printf("Handling BRender Pixmap\n");

        auto pmif = form.get_chunk("PMIF");
        auto pmdt = form.get_chunk("PMDT");

        if (pmif->content.size() != 14) {
            priv2::fail("Invalid PMIF chunk size");
        }

        uint16_t *read_ptr = (uint16_t *)pmif->content.data();
        uint16_t width = *read_ptr++;
        uint16_t unknown0 = *read_ptr++;
        if (unknown0 != 0x203) {
            priv2::fail(priv2::format("Unexpected unknown0 value: 0x%04x", unknown0));
        }
        uint16_t unknown1 = *read_ptr++;
        if (unknown1 != 0) {
            priv2::fail(priv2::format("Unexpected unknown1 value: 0x%04x", unknown1));
        }
        uint16_t unknown2 = *read_ptr++;
        if (unknown2 != width) {
            priv2::fail(priv2::format("Unexpected unknown2 value: 0x%04x", unknown2));
        }
        uint16_t height = *read_ptr++;
        uint16_t unknown3 = *read_ptr++;
        uint16_t unknown4 = *read_ptr++;

        auto filename = priv2::format("%s-brpm.png", filename_prefix.c_str());

        printf("Got PMIF: dt.size=%d, width=%d, height=%d, unks=[%d, %d, %d, %d, %d] -> %s\n",
                (int)pmdt->content.size(), width, height, unknown0, unknown1, unknown2,
                unknown3, unknown4, filename.c_str());

        if (pmdt->content.size() != width * height) {
            priv2::fail("Unexpected data size");
        }

        priv2::gfx::save_png(width, height, (uint8_t *)pmdt->content.data(), filename);
    }
}

void
IFF::handle_form(const std::string &path_sig, const std::string &sig,
        uint32_t offset,
        const char *form_buf, size_t form_len)
{
    uint32_t *read_ptr = (uint32_t *)form_buf;

    if (sig != "FORM") {
        priv2::fail("Expected FORM chunk here");
    }

    auto form_sig = priv2::fourcc(*read_ptr++);

    printf("Form Signature: '%s'\n", form_sig.c_str());

    Form form(form_sig);

    while ((char *)read_ptr < form_buf + form_len) {
        auto local_sig = priv2::fourcc(*read_ptr++);
        uint32_t local_len = priv2::byteswap(*read_ptr++);
        char *local_buf = (char *)read_ptr;
        char *content_buf = local_buf;
        uint32_t content_len = local_len;

        if (local_len == 0) {
            priv2::fail("Zero length chunk, there's probably something wrong with parsing");
        }

        bool deflate_compressed = priv2::deflate::is_compressed(local_buf, local_len);
        bool fb10_compressed = priv2::fb10::is_compressed(local_buf, local_len);

        printf("Local Signature: path='%s', sig='%s', len=%d, deflate=%s, fb10=%s\n",
                path_sig.c_str(), local_sig.c_str(),
                local_len, deflate_compressed ? "true" : "false",
                fb10_compressed ? "true" : "false");

        std::vector<char> tmp;

        if (fb10_compressed) {
            tmp = priv2::fb10::decompress(local_buf, local_len);
            content_buf = tmp.data();
            content_len = tmp.size();
        } else if (deflate_compressed) {
            tmp = priv2::deflate::decompress(local_buf, local_len);
            content_buf = tmp.data();
            content_len = tmp.size();
        } else {
            tmp.resize(local_len);
            memcpy(tmp.data(), local_buf, local_len);
        }

        handle_chunk(path_sig, form_sig, local_sig, offset + local_buf - form_buf, content_buf, content_len);

        form.chunks.emplace_back(local_sig, std::move(tmp));

        read_ptr = (uint32_t *)(local_buf + local_len + (local_len % 2));
    }

    handle_complete_form(form);
}

bool IFF::is_iff()
{
    uint32_t *read_ptr = (uint32_t *)buf;
    auto sig = priv2::fourcc(*read_ptr++);
    return (len >= 8 && sig == "FORM");
}

void IFF::parse()
{
    if (!is_iff()) {
        priv2::fail("Not an IFF");
    }

    uint32_t *read_ptr = (uint32_t *)buf;

    auto sig = priv2::fourcc(*read_ptr++);

    uint32_t local_len = priv2::byteswap(*read_ptr++);
    char *local_buf = (char *)read_ptr;
    uint32_t offset = local_buf - buf;

    constexpr uint32_t HEADER_SIZE = sizeof(uint32_t) * 2;

    uint32_t max_local_len = len - HEADER_SIZE;
    if (local_len > max_local_len) {
        // Seen for some files matching MISSION?.IFF where local_len = 0x04000000
        printf("Local length is 0x%08x (%d bytes); setting to %d bytes (= remaining bytes in file)\n",
                local_len, local_len, max_local_len);

        local_len = max_local_len;
    }

    printf("Starting to parse file with sig '%s', expected length = 0x%08x\n", sig.c_str(), local_len);

    handle_form("", sig, offset, local_buf, local_len);
    int32_t trailing = len - local_len - HEADER_SIZE;
    if (trailing > 0) {
        printf("Also writing unhandled trailing %u bytes\n", trailing);
        priv2::write_file(buf + len - trailing, trailing,
                "%s-chunk-%#010x-taildata.bin", filename_prefix.c_str(), len - trailing);
    }
}

};

namespace priv2 {
namespace iff {

bool
is_iff(const char *buf, size_t len)
{
    return IFF(buf, len, "").is_iff();
}

void
handle_iff(const char *buf, size_t len, const std::string &filename_prefix)
{
    IFF iff(buf, len, filename_prefix);
    iff.parse();
}

};
};
