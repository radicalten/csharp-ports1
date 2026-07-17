// reader.cpp
#include "reader.hpp"
#include <fstream>
namespace dsm {
    ArchiveDocument load_from_file(const std::filesystem::path& p) {
        std::ifstream f(p, std::ios::binary);
        std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), {});
        ArchiveType t = detect_type(buf);
        return t == ArchiveType::MSND ? parse_msnd(buf, p.string()) : parse_dsarc(buf, p.string());
    }
    // parse_dsarc / parse_msnd implemented similarly to C# using std::vector and offsets
}
