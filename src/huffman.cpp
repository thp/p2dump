/*
 * Privateer 2: The Darkening -- Data Dumper
 * Copyright (c) 2016, 2017, Thomas Perl
 *
 * Algorithm inspired by Mario Brito's C# Huffman decoder from
 * http://hcl.solsector.net/p2huffman.zip
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

#include "huffman.h"
#include "priv2.h"
#include "codepoint.h"

#include <cstdio>
#include <cstdlib>

namespace {

struct HuffTreeNode {
    static constexpr uint8_t NEWLINE_MARKER = 0xFE;
    static constexpr uint8_t PLACEHOLDER_MARKER = 0xF9;
    static constexpr uint8_t PLACEHOLDER_SUFFIX_PLANET_NAME = 0x81;
    static constexpr uint8_t PLACEHOLDER_SUFFIX_REPORTER_NAME = 0x82;
    static constexpr uint8_t END_MARKER = 0xFF;

    static constexpr uint16_t UNUSED = 0xFFFE;
    static constexpr uint16_t EMPTY = 0xFFFF;

    HuffTreeNode() : children() { children[0] = children[1] = UNUSED; }

    void initialize() { children[0] = children[1] = EMPTY; }
    bool is_leaf() const { return children[0] == EMPTY && children[1] == EMPTY; }
    bool is_unused() const { return children[0] == UNUSED; }
    bool is_empty(int child) const { return children[child] == EMPTY; }
    bool has_single_child() const { return children[0] != EMPTY && children[1] == EMPTY; }
    uint16_t get_index(int child) const { return children[child]; }
    void assign(int child, uint16_t value) { children[child] = value; }

private:
    uint16_t children[2];
};

struct IndexEntry {
    IndexEntry(uint32_t byte_offset=0, uint32_t bit_offset=0) : byte_offset(byte_offset), bit_offset(bit_offset) {}

    uint32_t byte_offset;
    uint32_t bit_offset;
};

struct BitStream {
    BitStream(const char *buf, size_t len)
        : buffer(buf)
        , offset_bits(0)
        , length_bits(8 * len)
    {
    }

    void seek(const IndexEntry &entry)
    {
        offset_bits = 8 * entry.byte_offset + entry.bit_offset;
    }

    bool available()
    {
        return offset_bits < length_bits;
    }

    uint8_t read_one()
    {
        uint8_t value = buffer[offset_bits / 8];
        uint8_t result = (value >> (7 - (offset_bits % 8))) & 1;
        offset_bits++;
        return result;
    }

    const char *buffer;
    size_t offset_bits;
    size_t length_bits;
};

struct HuffmanTextChunkDecoder {
    HuffmanTextChunkDecoder(const char *buf, size_t len)
        : buf(buf)
        , bitstream(buf, len)
        , index()
        , root_node(HuffTreeNode::UNUSED)
        , tree()
        , last_was_placeholder_marker(false)
    {
        decode();
    }

    uint32_t count() const { return index.size() - 1; }
    std::string get_entry(uint32_t i);
    std::string get_graphviz_source();

private:
    void decode();
    std::string get_node_name(uint16_t index);

    const char *buf;

    BitStream bitstream;
    std::vector<IndexEntry> index;
    uint32_t root_node;
    std::vector<HuffTreeNode> tree;
    bool last_was_placeholder_marker;
};

std::string
HuffmanTextChunkDecoder::get_node_name(uint16_t index)
{
    std::string result;

    auto &n = tree[index];
    if (n.is_unused()) {
        result = "<unused>";
    } else if (n.is_leaf()) {
        if (index > 31 && index < 127) {
            result = priv2::format("%c", index);
        } else if (index == HuffTreeNode::NEWLINE_MARKER) {
            result = "<newline>";
        } else if (index == HuffTreeNode::PLACEHOLDER_MARKER) {
            last_was_placeholder_marker = true;
            result = "<placeholder:";
        } else if (index == HuffTreeNode::END_MARKER) {
            result = "<end>";
        } else {
            const char *codepoint_utf8 = priv2::codepoint::get_utf8(index);
            result = codepoint_utf8 ? codepoint_utf8 : priv2::format("<0x%x>", index);
        }
    } else {
        result = priv2::format("%d", index);
    }

    return result;
}

void
HuffmanTextChunkDecoder::decode()
{
    uint32_t *read_ptr = (uint32_t *)buf;
    uint32_t start_tree = *read_ptr++;
    uint32_t num_entries = *read_ptr++;

    printf("Tree start: %#010x (%d), num_entries: %#010x (%d)\n",
            start_tree, start_tree, num_entries, num_entries);

    index.reserve(num_entries);
    for (int i=0; i<num_entries; i++) {
        uint32_t byte_offset = *read_ptr++;
        uint32_t bit_offset = *read_ptr++;
        index.emplace_back(byte_offset, bit_offset);
        //printf("Index %d: byte %d, bit %d\n", i, byte_offset, bit_offset);
    }

    if (read_ptr != (uint32_t *)(buf + start_tree)) {
        printf("Warning: Start tree points to different offset\n");
    }

    uint32_t uncompressed_bytes = *read_ptr++;
    printf("Estimated(?) uncompressed bytes in stream: %d\n", uncompressed_bytes);

    uint32_t tree_array_size = *read_ptr++;
    printf("Tree array size: %d\n", tree_array_size);

    std::vector<uint32_t> nodes;
    uint32_t *node_end_ptr = (uint32_t *)(buf + index[0].byte_offset);
    while (read_ptr < node_end_ptr) {
        uint32_t value = *read_ptr++;
        if (value >= tree_array_size) {
            printf("Warning: Ignoring invalid value 0x%08x (array size=%d)\n", value, tree_array_size);
            continue;
        }
        //printf("Tree[%d] = %#010x (%d) '%c'\n", nodes.size(), value, value, (value <= 31 || value >= 127) ? '.' : value);
        nodes.push_back(value);
    }
    root_node = nodes[0];

    // Build Huffman tree
    tree.resize(tree_array_size);
    tree[root_node].initialize();
    for (int i=nodes.size()-1; i >= 1; i--) {
        bool found = false;
        for (int j=tree.size()-1; j>=0; j--) {
            if (!tree[j].is_unused()) {
                for (int k=0; k<2; k++) {
                    if (tree[j].is_empty(k)) {
                        tree[j].assign(k, nodes[i]);
                        tree[nodes[i]].initialize();
                        found = true;
                        break;
                    }
                }
            }

            if (found) {
                break;
            }
        }

        if (!found) {
            priv2::fail("Could not insert node into tree");
        }
    }
}

std::string
HuffmanTextChunkDecoder::get_graphviz_source()
{
    std::string result;

    result += "digraph huffman {\n";
    for (int i=0; i<tree.size(); i++) {
        if (tree[i].is_unused() || tree[i].is_leaf()) {
            continue;
        }

        std::string name = get_node_name(i);

        for (int k=0; k<2; k++) {
            int j = tree[i].get_index(k);
            if (j != HuffTreeNode::EMPTY) {
                result += priv2::format("\"%s\" -> \"%s\" [ label = \"%d\" ];\n", get_node_name(i).c_str(), get_node_name(j).c_str(), k);
            }
        }
    }
    result += "}\n";

    return result;
}

std::string
HuffmanTextChunkDecoder::get_entry(uint32_t i)
{
    std::vector<char> result;
    uint32_t j = root_node;

    last_was_placeholder_marker = false;

    bitstream.seek(index[i]);
    while (bitstream.available()) {
        auto &n = tree[j];

        if (n.is_unused()) {
            priv2::fail("Invalid tree");
        }

        if (n.is_leaf()) {
            if (last_was_placeholder_marker) {
                std::string s;
                // Example usage in BOOTH.IFF, BOOT/NEWS/TXT2
                if (j == HuffTreeNode::PLACEHOLDER_SUFFIX_PLANET_NAME) {
                    s = "planet>";
                } else if (j == HuffTreeNode::PLACEHOLDER_SUFFIX_REPORTER_NAME) {
                    s = "reporter>";
                } else {
                    s = priv2::format("<id 0x%x>", j);
                }

                for (auto c: s) {
                    result.push_back(c);
                }
                last_was_placeholder_marker = false;
            } else if (j == HuffTreeNode::END_MARKER) {
                break;
            } else {
                for (auto c: get_node_name(j)) {
                    result.push_back(c);
                }
            }

            j = root_node;
            continue;
        } else {
            j = n.get_index(bitstream.read_one());
        }
    }

    return std::string(result.data(), result.size());
}

};

namespace priv2 {
namespace huffman {

DecodeResult
decode(const char *buf, size_t len)
{
    DecodeResult result;
    HuffmanTextChunkDecoder dec(buf, len);
    for (int i=0; i<dec.count(); i++) {
        result.items.emplace_back(dec.get_entry(i));
    }
    result.graphviz_dot_src = dec.get_graphviz_source();
    return result;
}

};
};
