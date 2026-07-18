#include "file_indexer/SearchEngine.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

namespace {

using file_indexer::BenchmarkResult;
using file_indexer::IndexStats;
using file_indexer::SearchEngine;
using file_indexer::SearchResult;

std::string trim(std::string value) {
    const auto notSpace = [](const unsigned char character) { return std::isspace(character) == 0; };
    const auto first = std::find_if(value.begin(), value.end(), notSpace);
    const auto last = std::find_if(value.rbegin(), value.rend(), notSpace).base();
    return first < last ? std::string(first, last) : std::string{};
}

std::string unquote(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '\'' && value.back() == '\''))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

void printHelp() {
    std::cout
        << "Parallel File Indexer and Search Engine\n\n"
        << "Usage:\n"
        << "  file-indexer index <directory>       Index, then open an interactive session\n"
        << "  file-indexer benchmark <directory>   Compare supported worker counts\n"
        << "  file-indexer search \"<query>\"       Search inside an active session\n"
        << "  file-indexer stats                   Show session statistics\n"
        << "  file-indexer clear                   Clear the in-memory session index\n"
        << "  file-indexer help                    Show this help\n\n"
        << "Interactive commands: index, search, stats, benchmark, clear, help, quit\n";
}

void printStats(const IndexStats& stats) {
    std::cout << "Discovered files: " << stats.discoveredFiles << '\n'
              << "Indexed files:    " << stats.indexedFiles << '\n'
              << "Failed files:     " << stats.failedFiles << '\n'
              << "Skipped files:    " << stats.skippedFiles << '\n'
              << "Unique words:     " << stats.uniqueWords << '\n'
              << "Total tokens:     " << stats.totalTokens << '\n'
              << "Workers:          " << stats.workerCount << '\n'
              << "Indexing time:    " << std::fixed << std::setprecision(3)
              << stats.elapsed.count() << " seconds\n";
}

void printResults(const std::vector<SearchResult>& results) {
    if (results.empty()) {
        std::cout << "No matches.\n";
        return;
    }
    for (const auto& result : results) {
        std::cout << result.filePath.generic_string() << " | score=" << result.score
                  << " | matched_terms=" << result.matchedTerms << '\n';
    }
}

void printBenchmark(const BenchmarkResult& result) {
    std::cout << std::left << std::setw(10) << "Workers" << std::setw(14) << "Time"
              << std::setw(12) << "Speedup" << "Equivalent\n";
    for (const auto& run : result.runs) {
        std::ostringstream time;
        time << std::fixed << std::setprecision(3) << run.elapsed.count() << " s";
        std::ostringstream speedup;
        speedup << std::fixed << std::setprecision(2) << run.speedup << 'x';
        std::cout << std::left << std::setw(10) << run.workerCount << std::setw(14) << time.str()
                  << std::setw(12) << speedup.str() << (run.equivalentToBaseline ? "yes" : "NO")
                  << '\n';
    }
}

int interactiveSession(SearchEngine& engine, bool hasIndex) {
    std::cout << "Interactive session. Type 'help' for commands and 'quit' to exit.\n";
    std::string line;
    while (std::cout << "indexer> " && std::getline(std::cin, line)) {
        line = trim(std::move(line));
        if (line.empty()) {
            continue;
        }
        const auto separator = line.find_first_of(" \t");
        const std::string command = line.substr(0, separator);
        const std::string argument = separator == std::string::npos ? "" : unquote(line.substr(separator + 1));
        try {
            if (command == "quit" || command == "exit") {
                return 0;
            }
            if (command == "help") {
                printHelp();
            } else if (command == "index") {
                if (argument.empty()) {
                    std::cerr << "error: index requires a directory\n";
                    continue;
                }
                printStats(engine.indexDirectory(argument));
                hasIndex = true;
            } else if (command == "search") {
                if (!hasIndex) {
                    std::cerr << "error: index a directory in this session before searching\n";
                } else if (argument.empty()) {
                    std::cerr << "error: search requires a non-empty query\n";
                } else {
                    printResults(engine.search(argument));
                }
            } else if (command == "stats") {
                printStats(engine.stats());
            } else if (command == "clear") {
                engine.clear();
                hasIndex = false;
                std::cout << "Index cleared.\n";
            } else if (command == "benchmark") {
                if (argument.empty()) {
                    std::cerr << "error: benchmark requires a directory\n";
                    continue;
                }
                printBenchmark(engine.benchmark(argument));
                hasIndex = true;
            } else {
                std::cerr << "error: unknown command: " << command << '\n';
            }
        } catch (const std::exception& error) {
            std::cerr << "error: " << error.what() << '\n';
        }
    }
    return 0;
}

}  // namespace

int main(const int argc, char* argv[]) {
    const std::size_t workers = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    try {
        SearchEngine engine(workers);
        if (argc == 1) {
            return interactiveSession(engine, false);
        }

        const std::string command = argv[1];
        if (command == "help" || command == "--help" || command == "-h") {
            printHelp();
            return 0;
        }
        if (command == "index" && argc == 3) {
            printStats(engine.indexDirectory(std::filesystem::path(argv[2])));
            return interactiveSession(engine, true);
        }
        if (command == "benchmark" && argc == 3) {
            printBenchmark(engine.benchmark(std::filesystem::path(argv[2])));
            return 0;
        }
        if (command == "stats" && argc == 2) {
            printStats(engine.stats());
            return 0;
        }
        if (command == "clear" && argc == 2) {
            engine.clear();
            std::cout << "Index cleared.\n";
            return 0;
        }
        if (command == "search") {
            std::cerr << "error: the index is in memory and cannot survive separate process runs; "
                         "run 'file-indexer index <directory>' and use search in that session\n";
            return 2;
        }
        printHelp();
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
