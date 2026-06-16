/**
 * @file tfidf.h
 * @brief Cálculo de TF-IDF para vectorización de documentos
 */

#ifndef PARALEX_TFIDF_H
#define PARALEX_TFIDF_H

#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace paralex {

class TfIdf {
public:
    /**
     * Calcula TF (Term Frequency) normalizada.
     * TF(t,d) = freq(t,d) / max_freq(d)
     */
    static double compute_tf(int term_freq, int max_freq);

    /**
     * Calcula IDF (Inverse Document Frequency).
     * IDF(t) = log(N / df(t))
     */
    static double compute_idf(int total_docs, int doc_frequency);

    /**
     * Calcula TF-IDF = TF * IDF.
     */
    static double compute_tfidf(int term_freq, int max_freq, int total_docs, int doc_frequency);

    /**
     * Calcula scores TF-IDF para todos los términos de todos los documentos.
     * @param per_doc_counts doc_id → {término → frecuencia}
     * @param doc_frequency término → número de documentos que lo contienen
     * @param total_docs Número total de documentos
     * @return doc_id → {término → score TF-IDF}
     */
    static std::map<int, std::map<std::string, double>> compute_all(
        const std::map<int, std::map<std::string, int>>& per_doc_counts,
        const std::map<std::string, int>& doc_frequency,
        int total_docs
    );

    /**
     * Extrae las top-N keywords de un documento basándose en TF-IDF.
     * @return Vector de (término, score) ordenado por score descendente
     */
    static std::vector<std::pair<std::string, double>> extract_keywords(
        const std::map<std::string, double>& tfidf_scores, int top_n = 10);

    /**
     * Construye un vector TF-IDF para un documento en el orden del vocabulario dado.
     */
    static std::vector<double> to_vector(
        const std::map<std::string, double>& tfidf_scores,
        const std::vector<std::string>& vocabulary);

    /**
     * Calcula el document frequency a partir de conteos por documento.
     */
    static std::map<std::string, int> compute_doc_frequencies(
        const std::map<int, std::map<std::string, int>>& per_doc_counts);
};

} // namespace paralex

#endif // PARALEX_TFIDF_H
