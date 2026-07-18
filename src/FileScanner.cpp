#include "file_indexer/FileScanner.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace file_indexer {

// Skip any single file larger than this so one huge file cannot use up memory.
constexpr std::uintmax_t kMaxFileSizeBytes = 5 * 1024 * 1024;  // 5 MB

bool FileScanner::isSupported(const std::filesystem::path& path) const {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](const char value) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    });
    return extension == ".txt" || extension == ".md" || extension == ".log" ||
           extension == ".csv";
}

ScanResult FileScanner::scan(const std::filesystem::path& root) const {
    std::error_code error;
    if (!std::filesystem::exists(root, error) || error) {
        throw std::invalid_argument("directory does not exist: " + root.string());
    }
    if (!std::filesystem::is_directory(root, error) || error) {
        throw std::invalid_argument("path is not a directory: " + root.string());
    }

    ScanResult result;
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::recursive_directory_iterator iterator(root, options, error);
    const std::filesystem::recursive_directory_iterator end;

    if (error) {
        ++result.inaccessibleEntries;
        error.clear();
    }

    while (iterator != end) {
        const auto& entry = *iterator;
        const bool regularFile = entry.is_regular_file(error);
        if (error) {
            ++result.inaccessibleEntries;
            error.clear();
        } else if (regularFile) {
            const auto size = entry.file_size(error);
            if (error) {
                ++result.inaccessibleEntries;
                error.clear();
            } else if (isSupported(entry.path()) && size <= kMaxFileSizeBytes) {
                result.files.push_back(entry.path());
            } else {
                ++result.skippedFiles;
            }
        }

        iterator.increment(error);
        if (error) {
            ++result.inaccessibleEntries;
            error.clear();
        }
    }

    std::sort(result.files.begin(), result.files.end());
    return result;
}

}  // namespace file_indexer
