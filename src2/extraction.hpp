#pragma once
#include "archive_types.hpp"
#include <string>
#include <functional>

namespace dsm {
    void extract_all(const ArchiveDocument& doc, const std::string& dest, bool nested);
    void extract_single(const ArchiveEntryPtr& e, const std::string& dest);
    void extract_nested(const ArchiveEntryPtr& e, const std::string& dest);
} // namespace dsm
