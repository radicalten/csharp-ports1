#include "writer.hpp"
#include "formats.hpp"
#include <stdexcept>
#include <algorithm>

namespace dsm {

static std::vector<uint8_t> serialize_dsarc(const ArchiveDocument& doc,
    const std::function<void(double)>& progress) {
    auto& entries = doc.root->children;
    int count = (int)entries.size();
    int header = DSARC_HEADER + count * (NAME_SIZE + DSARC_ENTRY_INFO);
    std::vector<std::pair<std::string, std::vector<uint8_t>>> data_list;
    int total = 0;
    for (int i = 0; i < count; i++) {
        auto d = serialize_entry(entries[i]);
        data_list.emplace_back(entries[i]->name, d);
        total += (int)d.size();
        if (progress) progress((double)(i+1) / (count*2));
    }
    std::vector<uint8_t> out;
    out.reserve(header + total);
    out.insert(out.end(), MAGIC_DSARC.begin(), MAGIC_DSARC.end());
    auto w32 = [&](int v){ auto p=(uint8_t*)&v; out.insert(out.end(),p,p+4); };
    w32(count); w32(DSARC_VERSION);
    int off = header;
    for (auto& [n,d] : data_list) {
        auto pn = pad_name(n);
        out.insert(out.end(), pn.begin(), pn.end());
        w32((int)d.size()); w32(off); off += (int)d.size();
    }
    for (size_t i = 0; i < data_list.size(); i++) {
        out.insert(out.end(), data_list[i].second.begin(), data_list[i].second.end());
        if (progress) progress(0.5 + (double)(i+1)/(data_list.size()*2));
    }
    return out;
}

std::vector<uint8_t> serialize_entry(const ArchiveEntryPtr& entry) {
    if (!entry->nested_type || entry->children.empty())
        return entry->data;
    if (*entry->nested_type == ArchiveType::MSND) {
        std::vector<uint8_t> sseq, sbnk, swar;
        for (auto& c : entry->children) {
            std::string ext = std::filesystem::path(c->name).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".sseq") sseq = c->data;
            else if (ext == ".sbnk") sbnk = c->data;
            else if (ext == ".swar") swar = c->data;
        }
        auto unk = entry->data.size() >= MSND_HEADER
            ? extract_msnd_unknown(entry->data) : std::vector<uint8_t>(4,0);
        return build_msnd(sseq, sbnk, swar, unk);
    }
    // DSARC nested
    ArchiveDocument tmp;
    tmp.file_type = ArchiveType::DSARC;
    tmp.root = entry;
    return serialize_dsarc(tmp, nullptr);
}

std::vector<uint8_t> serialize_document(const ArchiveDocument& doc,
    const std::function<void(double)>& progress) {
    if (doc.file_type == ArchiveType::MSND) {
        std::vector<uint8_t> sseq, sbnk, swar;
        for (auto& c : doc.root->children) {
            std::string ext = std::filesystem::path(c->name).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".sseq") sseq = c->data;
            else if (ext == ".sbnk") sbnk = c->data;
            else if (ext == ".swar") swar = c->data;
        }
        if (sseq.empty() && sbnk.empty() && swar.empty())
            return build_empty_msnd();
        return build_msnd(sseq, sbnk, swar);
    }
    return serialize_dsarc(doc, progress);
}

} // namespace dsm
