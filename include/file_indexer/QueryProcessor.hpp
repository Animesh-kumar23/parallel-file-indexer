#pragma once

#include "file_indexer/InvertedIndex.hpp"
#include "file_indexer/Tokenizer.hpp"
#include "file_indexer/Types.hpp"

#include <string_view>
#include <vector>

namespace file_indexer {

class QueryProcessor {
public:
    QueryProcessor(const InvertedIndex& index, const Tokenizer& tokenizer);
    [[nodiscard]] std::vector<SearchResult> search(std::string_view query) const;

private:
    const InvertedIndex& index_;
    const Tokenizer& tokenizer_;
};

}  // namespace file_indexer
