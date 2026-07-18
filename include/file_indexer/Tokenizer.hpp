#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace file_indexer {

class Tokenizer {
public:
    [[nodiscard]] std::vector<std::string> tokenize(std::string_view text) const;
};

}  // namespace file_indexer
