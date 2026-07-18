#include "file_indexer/Tokenizer.hpp"

#include <cctype>

namespace file_indexer {

std::vector<std::string> Tokenizer::tokenize(const std::string_view text) const {
    std::vector<std::string> tokens;
    std::string current;

    for (const char value : text) {
        const auto character = static_cast<unsigned char>(value);
        if (std::isalnum(character) != 0) {
            current.push_back(static_cast<char>(std::tolower(character)));
        } else if (!current.empty()) {
            tokens.push_back(std::move(current));
            current.clear();
        }
    }

    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

}  // namespace file_indexer
