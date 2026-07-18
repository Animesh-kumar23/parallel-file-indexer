#include "file_indexer/InvertedIndex.hpp"

#include <mutex>
#include <utility>

namespace file_indexer {

void InvertedIndex::merge(FileIndexResult result) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [word, frequency] : result.wordCounts) {
        index_[std::move(word)].push_back(Posting{result.filePath, frequency});
    }
    ++indexedFiles_;
    totalTokens_ += result.totalTokens;
}

PostingList InvertedIndex::postings(const std::string_view word) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = index_.find(std::string(word));
    return found == index_.end() ? PostingList{} : found->second;
}

IndexStats InvertedIndex::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    IndexStats result;
    result.indexedFiles = indexedFiles_;
    result.uniqueWords = index_.size();
    result.totalTokens = totalTokens_;
    return result;
}

std::unordered_map<std::string, PostingList> InvertedIndex::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_;
}

void InvertedIndex::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    index_.clear();
    indexedFiles_ = 0;
    totalTokens_ = 0;
}

}  // namespace file_indexer
