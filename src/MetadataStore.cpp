#include "file_indexer/MetadataStore.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace file_indexer {
namespace {

void runSql(sqlite3* database, const char* sql) {
    char* message = nullptr;
    if (sqlite3_exec(database, sql, nullptr, nullptr, &message) == SQLITE_OK) {
        return;
    }
    const std::string detail = message == nullptr ? sqlite3_errmsg(database) : message;
    sqlite3_free(message);
    throw std::runtime_error("SQLite error: " + detail);
}

}  // namespace

MetadataStore::MetadataStore(const std::filesystem::path& databasePath) {
    const std::string path = databasePath.string();
    if (sqlite3_open(path.c_str(), &database_) != SQLITE_OK) {
        const std::string detail = database_ == nullptr ? "unknown error" : sqlite3_errmsg(database_);
        sqlite3_close(database_);
        database_ = nullptr;
        throw std::runtime_error("could not open SQLite database: " + detail);
    }

    runSql(
        database_,
        "CREATE TABLE IF NOT EXISTS indexed_files ("
        "path TEXT NOT NULL,"
        "size_bytes INTEGER NOT NULL,"
        "token_count INTEGER NOT NULL,"
        "indexed_at INTEGER NOT NULL"
        ");");
}

MetadataStore::~MetadataStore() {
    sqlite3_close(database_);
}

void MetadataStore::save(const std::vector<FileMetadata>& files) {
    if (files.empty()) {
        return;
    }

    // Prepare the statement before opening a transaction, so that a failure
    // here cannot leave the connection stuck inside a half-open BEGIN.
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_,
            "INSERT INTO indexed_files(path, size_bytes, token_count, indexed_at) "
            "VALUES(?1, ?2, ?3, ?4);",
            -1,
            &statement,
            nullptr) != SQLITE_OK) {
        throw std::runtime_error(
            "could not prepare insert statement: " + std::string(sqlite3_errmsg(database_)));
    }

    // Insert every row in one transaction for speed. If any row fails, undo the
    // whole batch instead of leaving a half-written log or an open transaction.
    runSql(database_, "BEGIN;");
    try {
        for (const auto& file : files) {
            const std::string path = file.path.generic_string();
            sqlite3_bind_text(statement, 1, path.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(statement, 2, file.sizeBytes);
            sqlite3_bind_int64(statement, 3, file.tokenCount);
            sqlite3_bind_int64(statement, 4, file.indexedAt);
            if (sqlite3_step(statement) != SQLITE_DONE) {
                throw std::runtime_error(
                    "could not write metadata row: " + std::string(sqlite3_errmsg(database_)));
            }
            sqlite3_reset(statement);
        }
    } catch (...) {
        sqlite3_finalize(statement);
        runSql(database_, "ROLLBACK;");
        throw;
    }

    sqlite3_finalize(statement);
    runSql(database_, "COMMIT;");
}

}  // namespace file_indexer
