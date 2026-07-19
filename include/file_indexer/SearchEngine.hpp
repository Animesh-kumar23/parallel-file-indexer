#pragma once

#include "file_indexer/FileProcessor.hpp"
#include "file_indexer/FileScanner.hpp"
#include "file_indexer/InvertedIndex.hpp"
#include "file_indexer/MetadataStore.hpp"
#include "file_indexer/Tokenizer.hpp"
#include "file_indexer/Types.hpp"

#include <filesystem>
#include <string_view>
#include <vector>

namespace file_indexer {

class SearchEngine {
public:
    explicit SearchEngine(
        std::size_t workerCount,
        std::filesystem::path metadataDatabase = "file-indexer.sqlite3");

    [[nodiscard]] IndexStats indexDirectory(const std::filesystem::path& directory);
    [[nodiscard]] std::vector<SearchResult> search(std::string_view query) const;
    [[nodiscard]] BenchmarkResult benchmark(const std::filesystem::path& directory);
    [[nodiscard]] IndexStats stats() const;
    void clear();

private:
    [[nodiscard]] IndexStats indexDirectoryWithWorkers(
        const std::filesystem::path& directory, std::size_t workerCount, bool logMetadata);

    FileScanner scanner_;
    FileProcessor processor_;
    Tokenizer tokenizer_;
    InvertedIndex index_;
    MetadataStore metadataStore_;
    std::size_t workerCount_;
    IndexStats lastStats_;
};

}  // namespace file_indexer
