/**
 * @file cosine.cpp
 * @brief Implementación de Similitud Coseno
 */

#include "similarity/cosine.h"
#include <cmath>
#include <algorithm>

namespace paralex {

double CosineSimilarity::compute(const std::vector<double>& vec_a, const std::vector<double>& vec_b) {
    if (vec_a.size() != vec_b.size() || vec_a.empty()) return 0.0;

    double dot_product = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;

    for (size_t i = 0; i < vec_a.size(); ++i) {
        dot_product += vec_a[i] * vec_b[i];
        norm_a += vec_a[i] * vec_a[i];
        norm_b += vec_b[i] * vec_b[i];
    }

    if (norm_a == 0.0 || norm_b == 0.0) return 0.0;
    return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

std::vector<SimilarityPair> CosineSimilarity::compute_matrix(
    const std::map<int, std::map<std::string, double>>& tfidf_scores,
    const std::vector<std::string>& vocabulary,
    const std::map<int, std::string>& doc_names) {
    
    std::map<int, std::vector<double>> vectors;
    std::vector<int> doc_ids;
    
    for (const auto& [doc_id, scores] : tfidf_scores) {
        std::vector<double> vec(vocabulary.size(), 0.0);
        for (size_t i = 0; i < vocabulary.size(); ++i) {
            auto it = scores.find(vocabulary[i]);
            if (it != scores.end()) vec[i] = it->second;
        }
        vectors[doc_id] = std::move(vec);
        doc_ids.push_back(doc_id);
    }

    std::vector<SimilarityPair> pairs;
    for (size_t i = 0; i < doc_ids.size(); ++i) {
        for (size_t j = i + 1; j < doc_ids.size(); ++j) {
            int id1 = doc_ids[i];
            int id2 = doc_ids[j];
            double sim = compute(vectors[id1], vectors[id2]);
            
            std::string n1 = doc_names.count(id1) ? doc_names.at(id1) : "";
            std::string n2 = doc_names.count(id2) ? doc_names.at(id2) : "";
            pairs.emplace_back(id1, id2, n1, n2, sim);
        }
    }

    std::sort(pairs.begin(), pairs.end(), std::greater<SimilarityPair>());
    return pairs;
}

std::vector<SimilarityPair> CosineSimilarity::compute_subset(
    const std::map<int, std::vector<double>>& all_vectors,
    const std::vector<std::pair<int, int>>& pairs_to_compute,
    const std::map<int, std::string>& doc_names) {
    
    std::vector<SimilarityPair> results;
    results.reserve(pairs_to_compute.size());

    for (const auto& p : pairs_to_compute) {
        int id1 = p.first;
        int id2 = p.second;
        
        if (all_vectors.count(id1) && all_vectors.count(id2)) {
            double sim = compute(all_vectors.at(id1), all_vectors.at(id2));
            std::string n1 = doc_names.count(id1) ? doc_names.at(id1) : "";
            std::string n2 = doc_names.count(id2) ? doc_names.at(id2) : "";
            results.emplace_back(id1, id2, n1, n2, sim);
        }
    }

    std::sort(results.begin(), results.end(), std::greater<SimilarityPair>());
    return results;
}

std::vector<SimilarityPair> CosineSimilarity::get_top_similar(
    const std::vector<SimilarityPair>& all_pairs, int top_n) {
    
    std::vector<SimilarityPair> top = all_pairs;
    if (top.size() > static_cast<size_t>(top_n)) {
        top.resize(top_n);
    }
    return top;
}

std::vector<SimilarityPair> CosineSimilarity::detect_duplicates(
    const std::vector<SimilarityPair>& all_pairs, double threshold) {
    
    std::vector<SimilarityPair> duplicates;
    for (const auto& p : all_pairs) {
        if (p.similarity >= threshold) {
            duplicates.push_back(p);
        }
    }
    return duplicates;
}

std::vector<std::vector<double>> CosineSimilarity::to_matrix_2d(
    const std::vector<SimilarityPair>& pairs,
    const std::vector<int>& doc_ids) {
    
    size_t n = doc_ids.size();
    std::vector<std::vector<double>> mat(n, std::vector<double>(n, 1.0)); // 1.0 for self-similarity
    
    std::map<int, size_t> id_to_idx;
    for (size_t i = 0; i < n; ++i) {
        id_to_idx[doc_ids[i]] = i;
    }

    for (const auto& p : pairs) {
        if (id_to_idx.count(p.doc_id_1) && id_to_idx.count(p.doc_id_2)) {
            size_t i = id_to_idx[p.doc_id_1];
            size_t j = id_to_idx[p.doc_id_2];
            mat[i][j] = p.similarity;
            mat[j][i] = p.similarity;
        }
    }

    return mat;
}

} // namespace paralex
