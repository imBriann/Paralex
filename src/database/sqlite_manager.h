/**
 * @file sqlite_manager.h
 * @brief Gestor de base de datos SQLite para persistencia del índice
 */

#ifndef PARALEX_SQLITE_MANAGER_H
#define PARALEX_SQLITE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include "database/sqlite3.h"

namespace paralex {

struct PostingRow {
    std::string term;
    int doc_id;
    int term_frequency;
    std::string positions_json;
};

class SQLiteManager {
public:
    SQLiteManager(const std::string& db_path);
    ~SQLiteManager();

    bool initialize();

    /**
     * Persiste los resultados del MapReduce en la base de datos.
     */
    bool save_postings(const std::map<int, std::map<std::string, int>>& per_doc_counts);

    /**
     * Recupera todos los postings de un término específico.
     */
    std::vector<PostingRow> get_postings(const std::string& term) const;

    /**
     * Guarda la metadata y contenido del documento.
     */
    bool save_document(const struct Document& doc, int word_count, int unique_terms, const std::string& upload_date);

    /**
     * Guarda el historial de ejecución.
     */
    bool save_execution_history(const std::string& date, int docs_processed, double time_ms, int mpi_procs, double speedup, double cpu_usage, double ram_usage);


private:
    sqlite3* db_;
    std::string db_path_;

    bool execute_query(const std::string& query);
};

} // namespace paralex

#endif // PARALEX_SQLITE_MANAGER_H
