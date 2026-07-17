#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace dsm {

enum class ArchiveType { DSARC, MSND };

struct MsndOffsets {
    int sseq_offset, sbnk_offset, swar_offset;
    int sseq_size, sbnk_size, swar_size;
};

struct ArchiveEntry {
    static inline int64_t next_id = 0;
    int64_t id = ++next_id;
    std::string name;
    int size = 0;
    int offset = 0;
    std::optional<ArchiveType> nested_type;
    std::vector<uint8_t> data;               // in-memory source
    std::vector<std::shared_ptr<ArchiveEntry>> children;
    bool is_modified = false;
    int import_order = 0;

    std::string display_name() const { return is_modified ? name + " *" : name; }
};

struct ArchiveDocument {
    std::string file_path;
    ArchiveType file_type = ArchiveType::DSARC;
    std::shared_ptr<ArchiveEntry> root = std::make_shared<ArchiveEntry>();
    bool is_modified = false;
    bool has_content = false;
};

} // namespace dsm
