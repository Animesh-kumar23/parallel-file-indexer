#include "file_indexer/FileProcessor.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace file_indexer {

FileIndexResult FileProcessor::process(const std::filesystem::path& path) const {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open file: " + path.string());
    }

    FileIndexResult result;
    result.filePath = path;
    std::string line;
    while (std::getline(input, line)) {
        for (auto& token : tokenizer_.tokenize(line)) {
            ++result.wordCounts[token];
            ++result.totalTokens;
        }
    }

    if (input.bad()) {
        throw std::runtime_error("error while reading file: " + path.string());
    }
    return result;
}

}  // namespace file_indexer
