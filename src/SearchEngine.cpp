#include "file_indexer/SearchEngine.hpp"

#include "file_indexer/QueryProcessor.hpp"
#include "file_indexer/ThreadPool.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

namespace file_indexer {

SearchEngine::SearchEngine(const std::size_t workerCount, std::filesystem::path metadataDatabase)
    : metadataStore_(std::move(metadataDatabase)), workerCount_(workerCount) {
    if (workerCount == 0) {
        throw std::invalid_argument("worker count must be greater than zero");
    }
}

IndexStats SearchEngine::indexDirectory(const std::filesystem::path& directory) {
    return indexDirectoryWithWorkers(directory, workerCount_);
}

IndexStats SearchEngine::indexDirectoryWithWorkers(
    const std::filesystem::path& directory, const std::size_t workerCount) {
    index_.clear();
    const auto started = std::chrono::steady_clock::now();
    const ScanResult scanResult = scanner_.scan(directory);

    IndexStats result;
    result.discoveredFiles = scanResult.files.size() + scanResult.skippedFiles +
                             scanResult.inaccessibleEntries;
    result.skippedFiles = scanResult.skippedFiles + scanResult.inaccessibleEntries;
    result.workerCount = workerCount;

    std::atomic<std::size_t> indexedFiles{0};
    std::atomic<std::size_t> failedFiles{0};
    std::mutex warningMutex;
    std::vector<FileMetadata> metadataUpdates;
    ThreadPool pool(workerCount);

    for (const auto& path : scanResult.files) {
        pool.submit([this, path, &indexedFiles, &failedFiles, &warningMutex, &metadataUpdates] {
            try {
                FileIndexResult processed = processor_.process(path);
                const std::size_t tokenCount = processed.totalTokens;
                index_.merge(std::move(processed));

                std::error_code error;
                const auto size = std::filesystem::file_size(path, error);
                const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch());
                {
                    std::lock_guard<std::mutex> lock(warningMutex);
                    metadataUpdates.push_back(FileMetadata{
                        path,
                        error ? 0 : static_cast<std::int64_t>(size),
                        static_cast<std::int64_t>(tokenCount),
                        now.count(),
                    });
                }

                indexedFiles.fetch_add(1, std::memory_order_relaxed);
            } catch (const std::exception& error) {
                failedFiles.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(warningMutex);
                std::cerr << "warning: " << error.what() << '\n';
            } catch (...) {
                failedFiles.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(warningMutex);
                std::cerr << "warning: unknown failure while processing " << path << '\n';
            }
        });
    }
    pool.waitUntilIdle();
    metadataStore_.save(metadataUpdates);
    result.indexedFiles = indexedFiles.load(std::memory_order_relaxed);
    result.failedFiles = failedFiles.load(std::memory_order_relaxed);

    const IndexStats indexStats = index_.stats();
    result.uniqueWords = indexStats.uniqueWords;
    result.totalTokens = indexStats.totalTokens;
    result.elapsed = std::chrono::steady_clock::now() - started;
    lastStats_ = result;
    return result;
}

std::vector<SearchResult> SearchEngine::search(const std::string_view query) const {
    return QueryProcessor(index_, tokenizer_).search(query);
}

BenchmarkResult SearchEngine::benchmark(const std::filesystem::path& directory) {
    const std::size_t hardwareWorkers = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    std::vector<std::size_t> workerCounts{1, 2, 4, hardwareWorkers};
    std::sort(workerCounts.begin(), workerCounts.end());
    workerCounts.erase(std::unique(workerCounts.begin(), workerCounts.end()), workerCounts.end());

    // Warm the filesystem cache once and discard the result. The first read of a
    // freshly written dataset pays cold-cache (and, on Windows, antivirus-scan)
    // costs that would otherwise be charged entirely to the one-worker baseline
    // and inflate every reported speedup.
    static_cast<void>(indexDirectoryWithWorkers(directory, workerCounts.front()));

    BenchmarkResult benchmarkResult;
    IndexStats baseline;
    for (const std::size_t count : workerCounts) {
        const IndexStats runStats = indexDirectoryWithWorkers(directory, count);
        if (benchmarkResult.runs.empty()) {
            baseline = runStats;
        }
        const bool equivalent =
            runStats.discoveredFiles == baseline.discoveredFiles &&
            runStats.indexedFiles == baseline.indexedFiles &&
            runStats.failedFiles == baseline.failedFiles &&
            runStats.skippedFiles == baseline.skippedFiles &&
            runStats.uniqueWords == baseline.uniqueWords &&
            runStats.totalTokens == baseline.totalTokens;
        const double seconds = runStats.elapsed.count();
        benchmarkResult.runs.push_back(BenchmarkRun{
            count,
            runStats.elapsed,
            seconds > 0.0 ? baseline.elapsed.count() / seconds : 0.0,
            runStats,
            equivalent,
        });
    }
    return benchmarkResult;
}

IndexStats SearchEngine::stats() const {
    return lastStats_;
}

void SearchEngine::clear() {
    index_.clear();
    lastStats_ = {};
    lastStats_.workerCount = workerCount_;
}

}  // namespace file_indexer
