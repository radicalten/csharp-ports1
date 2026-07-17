#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <filesystem>

namespace dsm {

enum class ArchiveType { DSARC, MSND };

struct MsndOffsets {
    int sseq_offset = 0, sbnk_offset = 0, swar_offset = 0;
    int sseq_size = 0, sbnk_size = 0, swar_size = 0;
};

struct ArchiveEntry {
    static inline int64_t next_id = 0;
    int64_t id = ++next_id;
    std::string name;
    int size = 0;
    int offset = 0;
    std::optional<ArchiveType> nested_type;
    std::vector<uint8_t> data;
    std::vector<std::shared_ptr<ArchiveEntry>> children;
    bool is_modified = false;
    int import_order = 0;

    std::string display_name() const { return is_modified ? name + " *" : name; }
};

using ArchiveEntryPtr = std::shared_ptr<ArchiveEntry>;

struct ArchiveDocument {
    std::string file_path;
    ArchiveType file_type = ArchiveType::DSARC;
    ArchiveEntryPtr root = std::make_shared<ArchiveEntry>();
    bool is_modified = false;
    bool has_content = false;
    std::string original_file_path;

    std::vector<ArchiveEntryPtr> get_all_entries() const {
        std::vector<ArchiveEntryPtr> result;
        collect(root, result);
        return result;
    }
    static void collect(const ArchiveEntryPtr& e, std::vector<ArchiveEntryPtr>& out) {
        for (auto& c : e->children) {
            out.push_back(c);
            if (!c->children.empty()) collect(c, out);
        }
    }
};

} // namespace dsm
