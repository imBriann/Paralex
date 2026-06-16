/**
 * @file cosine.h
 * @brief Similitud coseno entre documentos y detección de duplicados
 */

#ifndef PARALEX_COSINE_H
#define PARALEX_COSINE_H

#include <string>
#include <vector>
#include <map>

namespace paralex {

/**
 * Par de documentos con su similitud.
 */
struct SimilarityPair {
    int doc_id_1;
    int doc_id_2;
    std::string name_1;
    std::string name_2;
    double similarity;

    SimilarityPair() : doc_id_1(-1), doc_id_2(-1), similarity(0.0) {}
    SimilarityPair(int id1, int id2, const std::string& n1, const std::string& n2, double sim)
        : doc_id_1(id1), doc_id_2(id2), name_1(n1), name_2(n2), similarity(sim) {}
    
    bool operator>(const SimilarityPair& other) const { return similarity > other.similarity; }
};

class CosineSimilarity {
public:
    /**
     * Calcula similitud coseno entre dos vectores.
     * cos(a,b) = dot(a,b) / (|a| * |b|)
     */
    static double compute(const std::vector<double>& vec_a, const std::vector<double>& vec_b);

    /**
     * Calcula la matriz de similitud completa entre todos los documentos.
     * @param tfidf_scores doc_id → {término → score}
     * @param vocabulary Lista ordenada de todos los términos
     * @param doc_names doc_id → nombre del documento
     * @return Matriz de similitud como vector de SimilarityPairs
     */
    static std::vector<SimilarityPair> compute_matrix(
        const std::map<int, std::map<std::string, double>>& tfidf_scores,
        const std::vector<std::string>& vocabulary,
        const std::map<int, std::string>& doc_names
    );

    /**
     * Calcula similitud para un subconjunto de pares (para distribución MPI).
     * @param all_vectors Vectores TF-IDF por doc_id
     * @param pairs Pares (i,j) a calcular
     * @param doc_names Nombres de documentos
     * @return Resultados de similitud
     */
    static std::vector<SimilarityPair> compute_subset(
        const std::map<int, std::vector<double>>& all_vectors,
        const std::vector<std::pair<int, int>>& pairs,
        const std::map<int, std::string>& doc_names
    );

    /**
     * Obtiene los top-N pares más similares.
     */
    static std::vector<SimilarityPair> get_top_similar(
        const std::vector<SimilarityPair>& all_pairs, int top_n = 10);

    /**
     * Detecta duplicados potenciales (similitud >= threshold).
     */
    static std::vector<SimilarityPair> detect_duplicates(
        const std::vector<SimilarityPair>& all_pairs, double threshold = 0.90);

    /**
     * Genera la matriz como vector 2D para serialización.
     * @return Matriz NxN de similitudes con doc_ids como índice
     */
    static std::vector<std::vector<double>> to_matrix_2d(
        const std::vector<SimilarityPair>& pairs,
        const std::vector<int>& doc_ids
    );
};

} // namespace paralex

#endif // PARALEX_COSINE_H
