/**
 * @file analyzer.h
 * @brief Análisis documental avanzado: estadísticas, keywords, temas
 */

#ifndef PARALEX_ANALYZER_H
#define PARALEX_ANALYZER_H

#include <string>
#include <vector>
#include <map>

namespace paralex {

/**
 * Estadísticas de un documento individual.
 */
struct DocStats {
    int doc_id;
    std::string name;
    int total_words;
    int unique_words;
    double lexical_density;
    double lexical_diversity;
    std::vector<std::pair<std::string, double>> keywords;  // top keywords con TF-IDF score
};

/**
 * Estadísticas globales del corpus.
 */
struct CorpusStats {
    int total_documents;
    int total_words;
    int total_unique_words;
    double avg_doc_length;
    double global_lexical_density;
    std::vector<std::pair<std::string, int>> top_words;      // top palabras globales
    std::vector<DocStats> doc_stats;                          // stats por documento
};

/**
 * Tema detectado: grupo de keywords relacionados.
 */
struct DetectedTopic {
    int topic_id;
    std::vector<std::string> keywords;
    std::vector<int> doc_ids;
    double coherence_score;
};

class Analyzer {
public:
    /**
     * Calcula estadísticas completas del corpus.
     */
    static CorpusStats compute_corpus_stats(
        const std::map<int, std::map<std::string, int>>& per_doc_counts,
        const std::map<std::string, int>& global_counts,
        const std::map<int, std::string>& doc_names,
        const std::map<int, std::map<std::string, double>>& tfidf_scores
    );

    /**
     * Detecta temas basándose en co-ocurrencia de keywords TF-IDF.
     * Agrupación simple por similitud de keywords.
     */
    static std::vector<DetectedTopic> detect_topics(
        const std::map<int, std::map<std::string, double>>& tfidf_scores,
        int num_keywords_per_doc = 5,
        int max_topics = 10
    );

    /**
     * Identifica los documentos más "importantes" del corpus.
     * Basado en diversidad de vocabulario y cantidad de keywords únicos.
     * @return doc_ids ordenados por importancia descendente
     */
    static std::vector<std::pair<int, double>> rank_documents_by_importance(
        const std::map<int, std::map<std::string, int>>& per_doc_counts,
        const std::map<int, std::map<std::string, double>>& tfidf_scores
    );
};

} // namespace paralex

#endif // PARALEX_ANALYZER_H
