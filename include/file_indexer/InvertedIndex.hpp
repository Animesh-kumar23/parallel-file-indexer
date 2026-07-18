#pragma once

#include "file_indexer/Types.hpp"

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace file_indexer {

class InvertedIndex {
public:
    void merge(FileIndexResult result);
    [[nodiscard]] PostingList postings(std::string_view word) const;
    [[nodiscard]] IndexStats stats() const;
    [[nodiscard]] std::unordered_map<std::string, PostingList> snapshot() const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PostingList> index_;
    std::size_t indexedFiles_{};
    std::size_t totalTokens_{};
};

}  // namespace file_indexer
