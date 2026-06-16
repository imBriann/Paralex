/**
 * @file sqlite_manager.cpp
 * @brief Implementación del gestor de base de datos SQLite
 */

#include "database/sqlite_manager.h"
#include "utils/logger.h"
#include "core/document.h"
#include <iostream>

namespace paralex {

SQLiteManager::SQLiteManager(const std::string& db_path) : db_path_(db_path), db_(nullptr) {}

SQLiteManager::~SQLiteManager() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool SQLiteManager::initialize() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc) {
        LOG_ERROR("SQLite", "No se puede abrir DB: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    std::string create_docs_table = 
        "CREATE TABLE IF NOT EXISTS documents ("
        "doc_id INTEGER PRIMARY KEY, "
        "name TEXT NOT NULL, "
        "size INTEGER DEFAULT 0, "
        "upload_date TEXT, "
        "word_count INTEGER DEFAULT 0, "
        "unique_terms INTEGER DEFAULT 0, "
        "content TEXT, "
        "tags TEXT);";

    std::string create_index_table = 
        "CREATE TABLE IF NOT EXISTS inverted_index ("
        "term TEXT NOT NULL, "
        "doc_id INTEGER NOT NULL, "
        "term_freq INTEGER NOT NULL, "
        "PRIMARY KEY (term, doc_id), "
        "FOREIGN KEY(doc_id) REFERENCES documents(doc_id));";
        
    std::string create_idx = "CREATE INDEX IF NOT EXISTS idx_term ON inverted_index(term);";

    std::string create_exec_history_table =
        "CREATE TABLE IF NOT EXISTS execution_history ("
        "exec_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "date TEXT, "
        "docs_processed INTEGER, "
        "time_ms REAL, "
        "mpi_procs INTEGER, "
        "speedup REAL, "
        "cpu_usage REAL, "
        "ram_usage REAL);";

    std::string create_topics_table =
        "CREATE TABLE IF NOT EXISTS document_topics ("
        "doc_id INTEGER PRIMARY KEY, "
        "primary_theme TEXT, "
        "secondary_theme TEXT, "
        "FOREIGN KEY(doc_id) REFERENCES documents(doc_id));";

    if (!execute_query(create_docs_table) || !execute_query(create_index_table) || 
        !execute_query(create_idx) || !execute_query(create_exec_history_table) || 
        !execute_query(create_topics_table)) {
        return false;
    }

    LOG_INFO("SQLite", "Base de datos inicializada en: " + db_path_);
    return true;
}

bool SQLiteManager::execute_query(const std::string& query) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, query.c_str(), 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQLite", "Error SQL: " + std::string(err_msg));
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool SQLiteManager::save_document(const Document& doc, int word_count, int unique_terms, const std::string& upload_date) {
    std::string sql = "INSERT OR REPLACE INTO documents (doc_id, name, size, upload_date, word_count, unique_terms, content) VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, doc.id);
    sqlite3_bind_text(stmt, 2, doc.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, static_cast<int>(doc.size_bytes));
    sqlite3_bind_text(stmt, 4, upload_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, word_count);
    sqlite3_bind_int(stmt, 6, unique_terms);
    sqlite3_bind_text(stmt, 7, doc.text.c_str(), -1, SQLITE_TRANSIENT);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return result;
}

bool SQLiteManager::save_execution_history(const std::string& date, int docs_processed, double time_ms, int mpi_procs, double speedup, double cpu_usage, double ram_usage) {
    std::string sql = "INSERT INTO execution_history (date, docs_processed, time_ms, mpi_procs, speedup, cpu_usage, ram_usage) VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, docs_processed);
    sqlite3_bind_double(stmt, 3, time_ms);
    sqlite3_bind_int(stmt, 4, mpi_procs);
    sqlite3_bind_double(stmt, 5, speedup);
    sqlite3_bind_double(stmt, 6, cpu_usage);
    sqlite3_bind_double(stmt, 7, ram_usage);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return result;
}

bool SQLiteManager::save_postings(const std::map<int, std::map<std::string, int>>& per_doc_counts) {
    execute_query("BEGIN TRANSACTION;");

    std::string sql = "INSERT OR REPLACE INTO inverted_index (term, doc_id, term_freq) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        execute_query("ROLLBACK;");
        return false;
    }

    for (const auto& [doc_id, words] : per_doc_counts) {
        for (const auto& [term, freq] : words) {
            sqlite3_bind_text(stmt, 1, term.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, doc_id);
            sqlite3_bind_int(stmt, 3, freq);
            
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                LOG_ERROR("SQLite", "Error insertando posting para " + term);
            }
            sqlite3_reset(stmt);
        }
    }

    sqlite3_finalize(stmt);
    execute_query("COMMIT;");
    return true;
}

std::vector<PostingRow> SQLiteManager::get_postings(const std::string& term) const {
    std::vector<PostingRow> results;
    std::string sql = "SELECT doc_id, term_freq FROM inverted_index WHERE term = ? ORDER BY term_freq DESC;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    sqlite3_bind_text(stmt, 1, term.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PostingRow row;
        row.term = term;
        row.doc_id = sqlite3_column_int(stmt, 0);
        row.term_frequency = sqlite3_column_int(stmt, 1);
        results.push_back(row);
    }

    sqlite3_finalize(stmt);
    return results;
}

} // namespace paralex
