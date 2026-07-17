#pragma once
#include "archive_types.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "folder_import.hpp"
#include <vector>
#include <memory>
#include <optional>

namespace dsm {

// Deep-clone a document (recursive shared_ptr tree copy)
ArchiveDocument clone_doc(const ArchiveDocument& d);

ArchiveEntryPtr find_by_id(const ArchiveEntryPtr& root, int64_t id);
ArchiveEntryPtr find_parent(const ArchiveEntryPtr& root, int64_t child_id);
bool remove_by_id(const ArchiveEntryPtr& parent, int64_t id);
void mark_parents_modified(const ArchiveEntryPtr& root, int64_t child_id);
void clear_modified(const ArchiveEntryPtr& e);
void populate_msnd_modified(const std::vector<uint8_t>& msnd, ArchiveEntryPtr entry,
                            bool mark = true, const std::string& replaced = "");

struct Command {
    virtual ~Command() = default;
    virtual ArchiveDocument execute(const ArchiveDocument& doc) = 0;
};

using CommandPtr = std::unique_ptr<Command>;

struct LoadCommand : Command {
    std::string path;
    ArchiveDocument execute(const ArchiveDocument&) override {
        auto d = load_from_file(path); d.has_content = true; return d;
    }
};

struct ReplaceEntryCommand : Command {
    ArchiveDocument doc; int64_t target; std::vector<uint8_t> new_data;
    ArchiveDocument execute(const ArchiveDocument&) override;
};

struct ReplaceChunkCommand : Command {
    ArchiveDocument doc; int64_t parent, chunk; std::vector<uint8_t> data;
    ArchiveDocument execute(const ArchiveDocument&) override;
};

struct AddBlankCommand : Command {
    ArchiveDocument doc; std::optional<int64_t> parent; std::string name;
    bool is_container; std::optional<ArchiveType> ctype;
    ArchiveDocument execute(const ArchiveDocument&) override;
};

struct DeleteCommand : Command {
    ArchiveDocument doc; int64_t target;
    ArchiveDocument execute(const ArchiveDocument&) override;
};

struct RenameCommand : Command {
    ArchiveDocument doc; int64_t target; std::string new_name;
    ArchiveDocument execute(const ArchiveDocument&) override;
};

struct ImportFolderCommand : Command {
    std::string folder;
    ArchiveDocument execute(const ArchiveDocument&) override;
};

struct SaveCommand : Command {
    ArchiveDocument doc; std::string path;
    ArchiveDocument execute(const ArchiveDocument&) override;
};

struct CommandProcessor {
    ArchiveDocument current;
    std::vector<ArchiveDocument> undo_stack;
    std::vector<ArchiveDocument> redo_stack;
    static constexpr size_t MAX = 50;

    ArchiveDocument execute(CommandPtr cmd) {
        undo_stack.push_back(current);
        if (undo_stack.size() > MAX) undo_stack.erase(undo_stack.begin());
        redo_stack.clear();
        current = cmd->execute(current);
        return current;
    }
    ArchiveDocument undo() {
        if (undo_stack.empty()) return current;
        redo_stack.push_back(current);
        current = undo_stack.back(); undo_stack.pop_back();
        return current;
    }
    ArchiveDocument redo() {
        if (redo_stack.empty()) return current;
        undo_stack.push_back(current);
        current = redo_stack.back(); redo_stack.pop_back();
        return current;
    }
    void set_current(const ArchiveDocument& d) {
        current = d; undo_stack.clear(); redo_stack.clear();
    }
    bool can_undo() const { return !undo_stack.empty(); }
    bool can_redo() const { return !redo_stack.empty(); }
};

} // namespace dsm
