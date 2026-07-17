#pragma once
#include "archive_types.hpp"
#include <vector>
#include <functional>

namespace dsm {

std::vector<uint8_t> serialize_document(const ArchiveDocument& doc,
    const std::function<void(double)>& progress = nullptr);
std::vector<uint8_t> serialize_entry(const ArchiveEntryPtr& entry);

} // namespace dsm
