#pragma once

#include "file_indexer/Tokenizer.hpp"
#include "file_indexer/Types.hpp"

#include <filesystem>

namespace file_indexer {

class FileProcessor {
public:
    [[nodiscard]] FileIndexResult process(const std::filesystem::path& path) const;

private:
    Tokenizer tokenizer_;
};

}  // namespace file_indexer
