/**
 * @file reducer.h
 * @brief Fase Reduce del motor MapReduce
 * 
 * Consolida resultados de múltiples mappers en conteos globales,
 * índice invertido y estadísticas agregadas.
 */

#ifndef PARALEX_REDUCER_H
#define PARALEX_REDUCER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "mapreduce/mapper.h"

namespace paralex {

/**
 * Resultado consolidado de la fase Reduce.
 */
struct ReduceResult {
    std::map<std::string, int> global_word_counts;                    // término → frecuencia global
    std::map<int, std::map<std::string, int>> per_doc_counts;        // doc_id → {término → freq}
    std::map<std::string, std::vector<int>> inverted_index;          // término → [doc_ids]
    std::map<int, int> doc_total_words;                              // doc_id → total palabras
    std::map<int, int> doc_unique_words;                             // doc_id → palabras únicas
    std::map<int, std::string> doc_names;                            // doc_id → nombre

    int total_documents;
    int total_words;
    int total_unique_words;

    ReduceResult() : total_documents(0), total_words(0), total_unique_words(0) {}
};

class Reducer {
public:
    /**
     * Reduce: Consolida resultados Map de todos los procesos.
     * Fusiona conteos, construye índice invertido, calcula estadísticas.
     * @param map_results Resultados de todos los mappers
     * @return Resultado consolidado
     */
    ReduceResult reduce(const std::vector<MapResult>& map_results) const;

    /**
     * Reduce incremental: fusiona un nuevo MapResult con un ReduceResult existente.
     */
    void reduce_into(ReduceResult& accumulated, const MapResult& new_result) const;

    /**
     * Genera ranking de las top-N palabras más frecuentes.
     * @return Vector de (término, frecuencia, porcentaje) ordenado por frecuencia
     */
    static std::vector<std::tuple<std::string, int, double>> get_top_words(
        const std::map<std::string, int>& global_counts, int top_n = 50);

    /**
     * Calcula la densidad léxica: unique_words / total_words.
     */
    static double lexical_density(int unique_words, int total_words);

    /**
     * Calcula la diversidad léxica (type-token ratio) para un documento.
     */
    static double lexical_diversity(const std::map<std::string, int>& word_counts);
};

} // namespace paralex

#endif // PARALEX_REDUCER_H
