#include "reader.hpp"
#include "formats.hpp"
#include <fstream>
#include <stdexcept>
#include <algorithm>

namespace dsm {

static std::string read_name(const uint8_t* p) {
    std::string s((const char*)p, 0);
    for (int i = 0; i < NAME_SIZE; i++) {
        if (p[i] == 0) break;
        s.push_back((char)p[i]);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
    return s;
}

static int gi32(const std::vector<uint8_t>& b, size_t o) {
    return *reinterpret_cast<const int32_t*>(b.data() + o);
}

ArchiveDocument parse_dsarc(const std::vector<uint8_t>& buf, const std::string& path) {
    if (buf.size() < DSARC_HEADER || !std::equal(buf.begin(), buf.begin()+8, MAGIC_DSARC.begin()))
        throw std::runtime_error("Invalid DSARC header.");
    int count = gi32(buf, 8);
    int version = gi32(buf, 12);
    if (version != DSARC_VERSION) throw std::runtime_error("Unsupported DSARC version: " + std::to_string(version));
    ArchiveDocument doc;
    doc.file_path = path;
    doc.file_type = ArchiveType::DSARC;
    doc.original_file_path = path;
    doc.root->name = std::filesystem::path(path).filename().string();
    int pos = DSARC_HEADER;
    for (int i = 0; i < count; i++) {
        std::string name = read_name(buf.data() + pos);
        if (name.empty()) name = "file_" + std::to_string(i);
        pos += NAME_SIZE;
        int size = gi32(buf, pos);
        int offset = gi32(buf, pos + 4);
        pos += DSARC_ENTRY_INFO;
        if (offset < 0 || size < 0 || (size_t)(offset + size) > buf.size())
            throw std::runtime_error("Entry '" + name + "' exceeds bounds.");
        auto e = std::make_shared<ArchiveEntry>();
        e->name = name; e->size = size; e->offset = offset; e->import_order = i;
        e->data.assign(buf.begin() + offset, buf.begin() + offset + size);
        if (size >= 4 && std::equal(buf.begin()+offset, buf.begin()+offset+4, MAGIC_MSND.begin())) {
            e->nested_type = ArchiveType::MSND;
            populate_msnd_children(buf, e, name);
        } else if (size >= 8 && std::equal(buf.begin()+offset, buf.begin()+offset+8, MAGIC_DSARC.begin())) {
            e->nested_type = ArchiveType::DSARC;
            // nested recursion omitted for brevity (mirror C# ParseDsarcChildren)
        }
        doc.root->children.push_back(e);
    }
    return doc;
}

ArchiveDocument parse_msnd(const std::vector<uint8_t>& buf, const std::string& path) {
    if (buf.size() < MSND_HEADER || !std::equal(buf.begin(), buf.begin()+4, MAGIC_MSND.begin()))
        throw std::runtime_error("Invalid MSND header.");
    ArchiveDocument doc;
    doc.file_path = path; doc.file_type = ArchiveType::MSND; doc.original_file_path = path;
    doc.root->name = std::filesystem::path(path).filename().string();
    doc.root->nested_type = ArchiveType::MSND;
    populate_msnd_children(buf, doc.root, std::filesystem::path(path).stem().string());
    return doc;
}

ArchiveDocument load_from_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), {});
    ArchiveType t = detect_type(buf);
    return t == ArchiveType::MSND ? parse_msnd(buf, path) : parse_dsarc(buf, path);
}

ArchiveDocument parse_from_buffer(const std::vector<uint8_t>& data, const std::string& virtual_path) {
    auto t = detect_type_from_buffer(data);
    if (!t) throw std::runtime_error("Unknown buffer format.");
    return *t == ArchiveType::MSND ? parse_msnd(data, virtual_path) : parse_dsarc(data, virtual_path);
}

} // namespace dsm
