// formats.cpp
#include "formats.hpp"
#include <algorithm>

namespace dsm {

std::vector<uint8_t> pad_name(const std::string& n) {
    std::vector<uint8_t> out(NAME_SIZE, 0);
    auto b = reinterpret_cast<const uint8_t*>(n.data());
    size_t len = std::min(n.size(), (size_t)NAME_SIZE);
    std::copy(b, b + len, out.begin());
    return out;
}

ArchiveType detect_type(const std::vector<uint8_t>& buf) {
    if (buf.size() >= 8 && std::equal(buf.begin(), buf.begin()+8, MAGIC_DSARC.begin()))
        return ArchiveType::DSARC;
    if (buf.size() >= 4 && std::equal(buf.begin(), buf.begin()+4, MAGIC_MSND.begin()))
        return ArchiveType::MSND;
    throw std::runtime_error("Unknown archive format");
}

MsndOffsets parse_msnd_offsets(const std::vector<uint8_t>& b) {
    if (b.size() < MSND_HEADER) throw std::runtime_error("MSND too small");
    auto i32 = [&](int o){ return *reinterpret_cast<const int32_t*>(b.data()+o); };
    return { i32(16)-16, i32(20), i32(24), i32(32), i32(36), i32(40) };
}

std::vector<uint8_t> build_msnd(const std::vector<uint8_t>& sseq,
                                const std::vector<uint8_t>& sbnk,
                                const std::vector<uint8_t>& swar,
                                const std::vector<uint8_t>& unknown) {
    int sseq_off = MSND_HEADER;
    int sbnk_off = sseq_off + sseq.size();
    int swar_off = sbnk_off + sbnk.size();
    std::vector<uint8_t> out;
    out.reserve(MSND_HEADER + sseq.size() + sbnk.size() + swar.size());
    out.insert(out.end(), MAGIC_MSND.begin(), MAGIC_MSND.end());
    out.resize(out.size() + 12);
    auto w32 = [&](int v){ auto p=(uint8_t*)&v; out.insert(out.end(), p, p+4); };
    w32(sseq_off+16); w32(sbnk_off); w32(swar_off);
    w32(0); w32(sseq.size()); w32(sbnk.size()); w32(swar.size());
    if (unknown.size()==4) out.insert(out.end(), unknown.begin(), unknown.end());
    else out.resize(out.size()+4);
    out.insert(out.end(), sseq.begin(), sseq.end());
    out.insert(out.end(), sbnk.begin(), sbnk.end());
    out.insert(out.end(), swar.begin(), swar.end());
    return out;
}

std::vector<uint8_t> build_dsarc(const std::vector<std::pair<std::string,std::vector<uint8_t>>>& entries) {
    int count = entries.size();
    int header = DSARC_HEADER + count*(NAME_SIZE+DSARC_ENTRY_INFO);
    int data_size = 0;
    for (auto& e : entries) data_size += e.second.size();
    std::vector<uint8_t> out; out.reserve(header+data_size);
    out.insert(out.end(), MAGIC_DSARC.begin(), MAGIC_DSARC.end());
    auto w32=[&](int v){auto p=(uint8_t*)&v;out.insert(out.end(),p,p+4);};
    w32(count); w32(DSARC_VERSION);
    int off = header;
    for (auto& e : entries) {
        auto pn = pad_name(e.first);
        out.insert(out.end(), pn.begin(), pn.end());
        w32(e.second.size()); w32(off); off += e.second.size();
    }
    for (auto& e : entries) out.insert(out.end(), e.second.begin(), e.second.end());
    return out;
}
} // namespace dsm
