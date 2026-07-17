#include "commands.hpp"
#include "formats.hpp"
#include <algorithm>
#include <stdexcept>

namespace dsm {

ArchiveEntryPtr find_by_id(const ArchiveEntryPtr& root, int64_t id) {
    if (root->id == id) return root;
    for (auto& c : root->children) {
        auto f = find_by_id(c, id);
        if (f) return f;
    }
    return nullptr;
}

ArchiveEntryPtr find_parent(const ArchiveEntryPtr& root, int64_t child_id) {
    for (auto& c : root->children) {
        if (c->id == child_id) return root;
        auto f = find_parent(c, child_id);
        if (f) return f;
    }
    return nullptr;
}

bool remove_by_id(const ArchiveEntryPtr& parent, int64_t id) {
    for (size_t i = 0; i < parent->children.size(); i++) {
        if (parent->children[i]->id == id) { parent->children.erase(parent->children.begin()+i); return true; }
        if (remove_by_id(parent->children[i], id)) return true;
    }
    return false;
}

void mark_parents_modified(const ArchiveEntryPtr& root, int64_t child_id) {
    auto p = find_parent(root, child_id);
    if (p && p != root) { p->is_modified = true; mark_parents_modified(root, p->id); }
}

void clear_modified(const ArchiveEntryPtr& e) {
    e->is_modified = false;
    for (auto& c : e->children) clear_modified(c);
}

void populate_msnd_modified(const std::vector<uint8_t>& msnd, ArchiveEntryPtr entry,
                            bool mark, const std::string& replaced) {
    MsndOffsets o = parse_msnd_offsets(msnd);
    std::string base = std::filesystem::path(entry->name).stem().string();
    entry->children.clear();
    auto mk = [&](const std::string& ext, int off, int sz) {
        auto e = std::make_shared<ArchiveEntry>();
        e->name = base + ext; e->size = sz; e->offset = off;
        e->data.assign(msnd.begin()+off, msnd.begin()+off+sz);
        e->is_modified = mark || replaced == ext;
        entry->children.push_back(e);
    };
    mk(".sseq", o.sseq_offset, o.sseq_size);
    mk(".sbnk", o.sbnk_offset, o.sbnk_size);
    mk(".swar", o.swar_offset, o.swar_size);
}

ArchiveDocument clone_doc(const ArchiveDocument& d) {
    ArchiveDocument out = d;
    // shallow clone of root with deep children
    std::function<ArchiveEntryPtr(const ArchiveEntryPtr&)> clone =
        [&](const ArchiveEntryPtr& src) {
            auto e = std::make_shared<ArchiveEntry>(*src);
            e->children.clear();
            for (auto& c : src->children) e->children.push_back(clone(c));
            return e;
        };
    out.root = clone(d.root);
    return out;
}

ArchiveDocument ReplaceEntryCommand::execute(const ArchiveDocument&) {
    auto doc = clone_doc(this->doc);
    auto e = find_by_id(doc.root, target);
    if (!e) throw std::runtime_error("Entry not found.");
    auto t = detect_type_from_buffer(new_data);
    e->data = new_data; e->size = (int)new_data.size(); e->is_modified = true;
    if (t == ArchiveType::MSND) { e->nested_type = ArchiveType::MSND; populate_msnd_modified(new_data, e); }
    else if (t == ArchiveType::DSARC) e->nested_type = ArchiveType::DSARC;
    else if (e->nested_type && !e->children.empty()) { e->nested_type.reset(); e->children.clear(); }
    mark_parents_modified(doc.root, target);
    doc.is_modified = true;
    return doc;
}

ArchiveDocument ReplaceChunkCommand::execute(const ArchiveDocument&) {
    auto doc = clone_doc(this->doc);
    auto p = find_by_id(doc.root, parent);
    auto c = find_by_id(doc.root, chunk);
    if (!p || !c) throw std::runtime_error("Parent/chunk not found.");
    std::string ext = std::filesystem::path(c->name).extension().string();
    auto msnd = replace_chunk(p->data, ext, data);
    p->data = msnd; p->size = (int)msnd.size(); p->is_modified = true;
    populate_msnd_modified(msnd, p, false, ext);
    mark_parents_modified(doc.root, parent);
    doc.is_modified = true;
    return doc;
}

ArchiveDocument AddBlankCommand::execute(const ArchiveDocument&) {
    auto doc = clone_doc(this->doc);
    std::vector<uint8_t> ed = (is_container && ctype == ArchiveType::MSND) ? build_empty_msnd() : std::vector<uint8_t>{};
    auto ne = std::make_shared<ArchiveEntry>();
    ne->name = name; ne->size = (int)ed.size(); ne->is_modified = true;
    ne->import_order = INT_MAX; ne->data = ed;
    ne->nested_type = is_container ? ctype : std::nullopt;
    if (is_container && ctype == ArchiveType::MSND) {
        std::string base = std::filesystem::path(name).stem().string();
        for (auto& ext : MSND_ORDER) {
            auto ch = std::make_shared<ArchiveEntry>();
            ch->name = base + ext; ch->size = 0; ch->offset = MSND_HEADER;
            ch->is_modified = true;
            ne->children.push_back(ch);
        }
    }
    if (!parent) doc.root->children.push_back(ne);
    else {
        auto tp = find_by_id(doc.root, *parent);
        if (tp) { tp->children.push_back(ne); tp->is_modified = true; mark_parents_modified(doc.root, *parent); }
    }
    doc.is_modified = true;
    return doc;
}

ArchiveDocument DeleteCommand::execute(const ArchiveDocument&) {
    auto doc = clone_doc(this->doc);
    auto p = find_parent(doc.root, target);
    bool removed = remove_by_id(doc.root, target);
    if (removed && p && p != doc.root) { p->is_modified = true; mark_parents_modified(doc.root, p->id); }
    doc.is_modified = removed || doc.is_modified;
    return doc;
}

ArchiveDocument RenameCommand::execute(const ArchiveDocument&) {
    auto doc = clone_doc(this->doc);
    auto e = find_by_id(doc.root, target);
    if (e) {
        std::string base = std::filesystem::path(new_name).stem().string();
        e->name = new_name; e->is_modified = true;
        if (e->nested_type == ArchiveType::MSND && e->children.size() == 3)
            for (auto& c : e->children) c->name = base + std::filesystem::path(c->name).extension().string();
        mark_parents_modified(doc.root, target);
    }
    doc.is_modified = true;
    return doc;
}

ArchiveDocument ImportFolderCommand::execute(const ArchiveDocument&) {
    return analyze_folder(folder);
}

ArchiveDocument SaveCommand::execute(const ArchiveDocument&) {
    auto data = serialize_document(doc, nullptr);
    std::ofstream f(path, std::ios::binary);
    f.write((const char*)data.data(), data.size());
    auto d = clone_doc(doc);
    clear_modified(d.root);
    d.file_path = path; d.original_file_path = path; d.is_modified = false;
    return d;
}

} // namespace dsm
