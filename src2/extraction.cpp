#include "extraction.hpp"
#include "writer.hpp"
#include "formats.hpp"
#include <filesystem>
#include <fstream>

namespace dsm {
namespace fs = std::filesystem;

static void write(const fs::path& p, const std::vector<uint8_t>& d) {
    std::ofstream f(p, std::ios::binary); f.write((const char*)d.data(), d.size());
}

void extract_single(const ArchiveEntryPtr& e, const std::string& dest) {
    fs::create_directories(dest);
    write(fs::path(dest) / e->name, e->data);
}

void extract_all(const ArchiveDocument& doc, const std::string& dest, bool nested) {
    fs::path base = fs::path(dest) / fs::path(doc.file_path).stem().string();
    fs::create_directories(base);
    if (doc.file_type == ArchiveType::MSND) {
        for (auto& c : doc.root->children) write(base / c->name, c->data);
    } else {
        std::ofstream map(base / "mapper.txt");
        for (auto& c : doc.root->children) {
            auto d = serialize_entry(c);
            std::string outname = c->name;
            if (nested && detect_type_from_buffer(d)) {
                fs::path sub = base / fs::path(c->name).stem().string();
                fs::create_directories(sub);
                // recurse simple
                write(sub / c->name, d);
                outname = fs::path(c->name).stem().string();
            } else {
                map << c->name << "=" << c->name << "\n";
                write(base / c->name, d);
            }
        }
    }
}

void extract_nested(const ArchiveEntryPtr& e, const std::string& dest) {
    fs::path base = fs::path(dest) / fs::path(e->name).stem().string();
    fs::create_directories(base);
    auto d = serialize_entry(e);
    for (auto& c : e->children) write(base / c->name, c->data);
    if (e->nested_type == ArchiveType::MSND)
        write(base / (fs::path(e->name).stem().string() + ".txt"), extract_msnd_unknown(d));
}

} // namespace dsm
