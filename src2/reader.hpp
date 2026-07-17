// reader.hpp
#pragma once
#include "archive_types.hpp"
namespace dsm {
    ArchiveDocument parse_dsarc(const std::vector<uint8_t>& buf, const std::string& path);
    ArchiveDocument parse_msnd(const std::vector<uint8_t>& buf, const std::string& path);
    ArchiveDocument load_from_file(const std::filesystem::path& p);
}
