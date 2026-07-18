#pragma once

#include "file_indexer/Types.hpp"

#include <filesystem>

namespace file_indexer {

class FileScanner {
public:
    [[nodiscard]] ScanResult scan(const std::filesystem::path& root) const;

private:
    [[nodiscard]] bool isSupported(const std::filesystem::path& path) const;
};

}  // namespace file_indexer
