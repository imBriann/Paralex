/**
 * @file search_engine.h
 * @brief Motor de búsqueda con ranking TF-IDF
 */

#ifndef PARALEX_SEARCH_ENGINE_H
#define PARALEX_SEARCH_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include "indexing/inverted_index.h"
#include "indexing/tfidf.h"
#include "utils/text_processor.h"

namespace paralex {

/**
 * Resultado individual de búsqueda.
 */
struct SearchResult {
    int doc_id;
    std::string doc_name;
    double score;
    int match_count;

    SearchResult() : doc_id(-1), score(0.0), match_count(0) {}
    SearchResult(int id, const std::string& name, double s, int m)
        : doc_id(id), doc_name(name), score(s), match_count(m) {}
    
    bool operator>(const SearchResult& other) const { return score > other.score; }
};

class SearchEngine {
public:
    SearchEngine(const TextProcessor& processor);

    /**
     * Configura el motor con el índice invertido y datos TF-IDF.
     */
    void configure(
        const InvertedIndex& index,
        const std::map<int, std::map<std::string, double>>& tfidf_scores,
        const std::map<int, std::string>& doc_names,
        int total_docs
    );

    /**
     * Ejecuta una búsqueda y retorna resultados ordenados por relevancia.
     * Soporta:
     *   - Palabra única: "paralelo"
     *   - Múltiples términos: "computacion paralela mpi"
     *   - Frase exacta: "\"procesamiento distribuido\""
     * @param query Consulta del usuario
     * @param max_results Número máximo de resultados
     * @return Resultados ordenados por score TF-IDF descendente
     */
    std::vector<SearchResult> search(const std::string& query, int max_results = 20) const;

private:
    const TextProcessor& processor_;
    const InvertedIndex* index_;
    const std::map<int, std::map<std::string, double>>* tfidf_scores_;
    const std::map<int, std::string>* doc_names_;
    int total_docs_;

    /**
     * Detecta si la query es una frase exacta (entre comillas).
     */
    bool is_exact_phrase(const std::string& query) const;

    /**
     * Busca una frase exacta en los documentos.
     */
    std::vector<SearchResult> search_exact_phrase(const std::string& phrase) const;

    /**
     * Busca múltiples términos y combina scores.
     */
    std::vector<SearchResult> search_terms(const std::vector<std::string>& terms) const;
};

} // namespace paralex

#endif // PARALEX_SEARCH_ENGINE_H
