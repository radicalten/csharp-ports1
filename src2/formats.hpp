// formats.hpp
#pragma once
#include "archive_types.hpp"
namespace dsm {

constexpr int NAME_SIZE = 40;
constexpr int DSARC_HEADER = 16;
constexpr int DSARC_ENTRY_INFO = 8;
constexpr int DSARC_VERSION = 1;
constexpr int MSND_HEADER = 48;

inline const std::vector<uint8_t> MAGIC_DSARC = {0x44,0x53,0x41,0x52,0x43,0x20,0x46,0x4C};
inline const std::vector<uint8_t> MAGIC_MSND  = {0x44,0x53,0x45,0x51};

std::vector<uint8_t> pad_name(const std::string& n);
ArchiveType detect_type(const std::vector<uint8_t>& buf);
MsndOffsets parse_msnd_offsets(const std::vector<uint8_t>& buf);
std::vector<uint8_t> build_msnd(const std::vector<uint8_t>& sseq,
                                const std::vector<uint8_t>& sbnk,
                                const std::vector<uint8_t>& swar,
                                const std::vector<uint8_t>& unknown = {});
std::vector<uint8_t> build_dsarc(const std::vector<std::pair<std::string,std::vector<uint8_t>>>& entries);
} // namespace dsm
