#pragma once
#include "archive_types.hpp"
#include <vector>
#include <string>

namespace dsm {

ArchiveDocument parse_dsarc(const std::vector<uint8_t>& buf, const std::string& path);
ArchiveDocument parse_msnd(const std::vector<uint8_t>& buf, const std::string& path);
ArchiveDocument load_from_file(const std::string& path);
ArchiveDocument parse_from_buffer(const std::vector<uint8_t>& data, const std::string& virtual_path);

} // namespace dsm
