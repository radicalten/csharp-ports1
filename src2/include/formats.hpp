#pragma once
#include "archive_types.hpp"
#include <vector>

namespace dsm {

constexpr int NAME_SIZE = 40;
constexpr int DSARC_HEADER = 16;
constexpr int DSARC_ENTRY_INFO = 8;
constexpr int DSARC_VERSION = 1;
constexpr int MSND_HEADER = 48;
constexpr int MSND_UNKNOWN_OFFSET = 0x2C;

extern const std::vector<uint8_t> MAGIC_DSARC;
extern const std::vector<uint8_t> MAGIC_MSND;
extern const std::vector<std::string> MSND_ORDER;

std::vector<uint8_t> pad_name(const std::string& n);
ArchiveType detect_type(const std::vector<uint8_t>& buf);
std::optional<ArchiveType> detect_type_from_buffer(const std::vector<uint8_t>& data);
MsndOffsets parse_msnd_offsets(const std::vector<uint8_t>& buf);
bool are_msnd_bounds_valid(size_t len, const MsndOffsets& o);
std::vector<uint8_t> build_msnd(const std::vector<uint8_t>& sseq,
                                const std::vector<uint8_t>& sbnk,
                                const std::vector<uint8_t>& swar,
                                const std::vector<uint8_t>& unknown = {});
std::vector<uint8_t> build_empty_msnd();
std::vector<uint8_t> extract_msnd_unknown(const std::vector<uint8_t>& msnd);
void populate_msnd_children(const std::vector<uint8_t>& data, ArchiveEntryPtr parent,
                            const std::string& base_name);
std::vector<uint8_t> replace_chunk(const std::vector<uint8_t>& msnd,
                                   const std::string& ext,
                                   const std::vector<uint8_t>& new_data);

} // namespace dsm
