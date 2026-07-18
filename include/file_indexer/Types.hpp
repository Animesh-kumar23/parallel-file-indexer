#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace file_indexer {

struct FileIndexResult {
    std::filesystem::path filePath;
    std::unordered_map<std::string, std::size_t> wordCounts;
    std::size_t totalTokens{};
};

struct Posting {
    std::filesystem::path filePath;
    std::size_t frequency{};
};

using PostingList = std::vector<Posting>;

struct SearchResult {
    std::filesystem::path filePath;
    std::size_t score{};
    std::size_t totalMatches{};
    std::size_t matchedTerms{};
};

struct ScanResult {
    std::vector<std::filesystem::path> files;
    std::size_t skippedFiles{};
    std::size_t inaccessibleEntries{};
};

struct IndexStats {
    std::size_t discoveredFiles{};
    std::size_t indexedFiles{};
    std::size_t failedFiles{};
    std::size_t skippedFiles{};
    std::size_t uniqueWords{};
    std::size_t totalTokens{};
    std::size_t workerCount{};
    std::chrono::duration<double> elapsed{};
};

struct BenchmarkRun {
    std::size_t workerCount{};
    std::chrono::duration<double> elapsed{};
    double speedup{};
    IndexStats stats;
    bool equivalentToBaseline{};
};

struct BenchmarkResult {
    std::vector<BenchmarkRun> runs;
};

}  // namespace file_indexer
