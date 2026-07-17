#pragma once
#include "archive_types.hpp"
#include "commands.hpp"
#include <functional>

namespace dsm {

class DocumentManager {
public:
    ArchiveDocument current() const { return proc.current; }
    bool can_undo() const { return proc.can_undo(); }
    bool can_redo() const { return proc.can_redo(); }
    std::function<void()> document_changed;

    ArchiveDocument load(const std::string& p) {
        auto c = std::make_unique<LoadCommand>(); c->path = p;
        auto d = proc.execute(std::move(c)); if (document_changed) document_changed(); return d;
    }
    ArchiveDocument import_folder(const std::string& f) {
        auto c = std::make_unique<ImportFolderCommand>(); c->folder = f;
        auto d = proc.execute(std::move(c)); if (document_changed) document_changed(); return d;
    }
    ArchiveDocument replace_entry(const ArchiveEntryPtr& e, const std::vector<uint8_t>& d) {
        auto c = std::make_unique<ReplaceEntryCommand>();
        c->doc = proc.current; c->target = e->id; c->new_data = d;
        auto r = proc.execute(std::move(c)); if (document_changed) document_changed(); return r;
    }
    ArchiveDocument replace_chunk(const ArchiveEntryPtr& p, const ArchiveEntryPtr& ch, const std::vector<uint8_t>& d) {
        auto c = std::make_unique<ReplaceChunkCommand>();
        c->doc = proc.current; c->parent = p->id; c->chunk = ch->id; c->data = d;
        auto r = proc.execute(std::move(c)); if (document_changed) document_changed(); return r;
    }
    ArchiveDocument save(const std::string& path) {
        auto c = std::make_unique<SaveCommand>();
        c->doc = proc.current; c->path = path;
        auto r = proc.execute(std::move(c)); if (document_changed) document_changed(); return r;
    }
    ArchiveDocument add_blank(const std::optional<int64_t>& parent, const std::string& name,
                              bool container, std::optional<ArchiveType> ct) {
        auto c = std::make_unique<AddBlankCommand>();
        c->doc = proc.current; c->parent = parent; c->name = name; c->is_container = container; c->ctype = ct;
        auto r = proc.execute(std::move(c)); if (document_changed) document_changed(); return r;
    }
    ArchiveDocument del(const ArchiveEntryPtr& e) {
        auto c = std::make_unique<DeleteCommand>();
        c->doc = proc.current; c->target = e->id;
        auto r = proc.execute(std::move(c)); if (document_changed) document_changed(); return r;
    }
    ArchiveDocument rename(const ArchiveEntryPtr& e, const std::string& n) {
        auto c = std::make_unique<RenameCommand>();
        c->doc = proc.current; c->target = e->id; c->new_name = n;
        auto r = proc.execute(std::move(c)); if (document_changed) document_changed(); return r;
    }
    ArchiveDocument undo() { auto d = proc.undo(); if (document_changed) document_changed(); return d; }
    ArchiveDocument redo() { auto d = proc.redo(); if (document_changed) document_changed(); return d; }
    void set_current(const ArchiveDocument& d) { proc.set_current(d); if (document_changed) document_changed(); }

    bool is_root_empty() const {
        if (proc.current.file_type == ArchiveType::DSARC) return proc.current.root->children.empty();
        return std::all_of(proc.current.root->children.begin(), proc.current.root->children.end(),
                           [](auto& c){ return c->size == 0; });
    }
    bool has_blank_files() const {
        std::function<bool(const ArchiveEntryPtr&)> blank = [&](const ArchiveEntryPtr& e) {
            for (auto& c : e->children) {
                if (c->nested_type == ArchiveType::MSND)
                    { if (c->children.empty() || std::all_of(c->children.begin(),c->children.end(),[](auto&x){return x->size==0;})) return true; }
                else if (c->nested_type == ArchiveType::DSARC) { if (blank(c)) return true; }
                else if (c->size == 0) return true;
            }
            return false;
        };
        return blank(proc.current.root);
    }
    void remove_all_empty() {
        // loop delete until none found (mirror C#)
        bool changed = true;
        while (changed) {
            changed = false;
            std::function<ArchiveEntryPtr(const ArchiveEntryPtr&)> find = [&](const ArchiveEntryPtr& r) -> ArchiveEntryPtr {
                for (auto& c : r->children) {
                    bool empty = c->nested_type == ArchiveType::MSND
                        ? (c->children.empty() || std::all_of(c->children.begin(),c->children.end(),[](auto&x){return x->size==0;}))
                        : c->nested_type == ArchiveType::DSARC ? (c->children.empty() || find(c) != nullptr) : c->size==0;
                    if (empty) return c;
                    if (c->nested_type == ArchiveType::DSARC) { auto f = find(c); if (f) return f; }
                }
                return ArchiveEntryPtr{};
            };
            auto target = find(proc.current.root);
            if (target) { del(target); changed = true; }
        }
    }
    bool has_duplicate(const ArchiveEntryPtr& e, const std::string& n) {
        auto p = find_parent(proc.current.root, e->id);
        if (!p) return false;
        return std::any_of(p->children.begin(), p->children.end(),
            [&](auto& c){ return c->id != e->id && c->name == n; });
    }
    bool can_sort(const ArchiveEntryPtr& c) {
        if (!c) return proc.current.file_type == ArchiveType::DSARC && proc.current.root->children.size() > 1;
        return c->nested_type == ArchiveType::DSARC && c->children.size() > 1;
    }
    void sort_alpha(std::optional<int64_t> cid) {
        auto doc = clone_doc(proc.current);
        auto& container = cid ? find_by_id(doc.root, *cid) : doc.root;
        if (container->nested_type == ArchiveType::MSND) return;
        std::sort(container->children.begin(), container->children.end(),
            [](auto& a, auto& b){ return a->name < b->name; });
        doc.is_modified = true;
        proc.set_current(doc); // simplified; full impl would push undo
        if (document_changed) document_changed();
    }

private:
    CommandProcessor proc;
};

} // namespace dsm
