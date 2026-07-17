#include "folder_import.hpp"
#include "formats.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <stdexcept>

namespace dsm {
namespace fs = std::filesystem;

static std::vector<uint8_t> read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return {(std::istreambuf_iterator<char>(f)), {}};
}

static std::vector<uint8_t> build_msnd_from_folder(const fs::path& folder) {
    std::vector<uint8_t> sseq, sbnk, swar;
    for (auto& ext : MSND_ORDER) {
        for (auto& e : fs::directory_iterator(folder)) {
            if (e.path().extension().string() == ext)
                (ext==".sseq"?sseq:(ext==".sbnk"?sbnk:swar)) = read_file(e.path());
        }
    }
    std::vector<uint8_t> unk(4,0);
    for (auto& e : fs::directory_iterator(folder)) {
        if (e.path().extension() == ".txt" && e.path().filename() != "mapper.txt") {
            auto d = read_file(e.path());
            if (d.size() >= 4) unk.assign(d.begin(), d.begin()+4);
            break;
        }
    }
    return build_msnd(sseq, sbnk, swar, unk);
}

ArchiveDocument analyze_folder(const std::string& folder) {
    if (!fs::exists(folder)) throw std::runtime_error("Folder not found: " + folder);
    ArchiveDocument doc;
    fs::path mapper = fs::path(folder) / "mapper.txt";
    if (fs::exists(mapper)) {
        doc.file_type = ArchiveType::DSARC;
        std::ifstream mf(mapper); std::string line;
        while (std::getline(mf, line)) {
            if (line.find('=') == std::string::npos) continue;
            auto eq = line.find('=');
            std::string an = line.substr(0, eq), sn = line.substr(eq+1);
            auto e = std::make_shared<ArchiveEntry>();
            e->name = an; e->data = read_file(fs::path(folder)/sn);
            e->size = (int)e->data.size(); e->import_order = (int)doc.root->children.size();
            doc.root->children.push_back(e);
        }
    } else {
        // check msnd structure
        bool msnd = true;
        int msnd_count = 0;
        for (auto& e : fs::directory_iterator(folder)) {
            if (e.is_regular_file()) {
                std::string ext = e.path().extension().string();
                if (std::find(MSND_ORDER.begin(), MSND_ORDER.end(), ext) != MSND_ORDER.end()) msnd_count++;
                else if (ext != ".txt") msnd = false;
            }
        }
        if (msnd && msnd_count > 0) {
            doc.file_type = ArchiveType::MSND;
            doc.root->nested_type = ArchiveType::MSND;
            auto msnd_data = build_msnd_from_folder(folder);
            doc.root->data = msnd_data; doc.root->size = (int)msnd_data.size();
            populate_msnd_children(msnd_data, doc.root, fs::path(folder).stem().string());
        } else {
            doc.file_type = ArchiveType::DSARC;
            for (auto& e : fs::directory_iterator(folder)) {
                if (e.path().filename() == "mapper.txt") continue;
                auto en = std::make_shared<ArchiveEntry>();
                en->name = e.path().filename().string();
                if (e.is_directory()) {
                    auto nested = build_msnd_from_folder(e.path());
                    en->data = nested; en->nested_type = ArchiveType::MSND;
                    en->size = (int)nested.size();
                    populate_msnd_children(nested, en, en->name);
                } else { en->data = read_file(e.path()); en->size = (int)en->data.size(); }
                en->import_order = (int)doc.root->children.size();
                doc.root->children.push_back(en);
            }
        }
    }
    doc.has_content = true;
    doc.root->name = fs::path(folder).filename().string();
    return doc;
}

} // namespace dsm
