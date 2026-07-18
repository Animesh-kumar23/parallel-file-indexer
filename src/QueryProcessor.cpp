#include "file_indexer/QueryProcessor.hpp"

#include <algorithm>
#include <map>
#include <unordered_set>

namespace file_indexer {

QueryProcessor::QueryProcessor(const InvertedIndex& index, const Tokenizer& tokenizer)
    : index_(index), tokenizer_(tokenizer) {}

std::vector<SearchResult> QueryProcessor::search(const std::string_view query) const {
    const auto tokens = tokenizer_.tokenize(query);
    std::unordered_set<std::string> uniqueTerms(tokens.begin(), tokens.end());
    std::map<std::filesystem::path, SearchResult> byFile;

    for (const auto& term : uniqueTerms) {
        for (const auto& posting : index_.postings(term)) {
            auto& result = byFile[posting.filePath];
            result.filePath = posting.filePath;
            result.score += posting.frequency;
            result.totalMatches += posting.frequency;
            ++result.matchedTerms;
        }
    }

    std::vector<SearchResult> results;
    results.reserve(byFile.size());
    for (auto& [path, result] : byFile) {
        (void)path;
        results.push_back(std::move(result));
    }

    std::sort(results.begin(), results.end(), [](const SearchResult& left, const SearchResult& right) {
        if (left.score != right.score) {
            return left.score > right.score;
        }
        return left.filePath.generic_string() < right.filePath.generic_string();
    });
    return results;
}

}  // namespace file_indexer
