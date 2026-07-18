#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

struct sqlite3;

namespace file_indexer {

struct FileMetadata {
    std::filesystem::path path;
    std::int64_t sizeBytes{};
    std::int64_t tokenCount{};
    std::int64_t indexedAt{};
};

class MetadataStore {
public:
    explicit MetadataStore(const std::filesystem::path& databasePath);
    ~MetadataStore();

    MetadataStore(const MetadataStore&) = delete;
    MetadataStore& operator=(const MetadataStore&) = delete;

    void save(const std::vector<FileMetadata>& files);

private:
    sqlite3* database_{};
};

}  // namespace file_indexer
