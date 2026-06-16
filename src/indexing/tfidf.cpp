/**
 * @file tfidf.cpp
 * @brief Implementación de TF-IDF
 */

#include "indexing/tfidf.h"
#include <algorithm>

namespace paralex {

double TfIdf::compute_tf(int term_freq, int max_freq) {
    if (max_freq == 0) return 0.0;
    return 0.5 + 0.5 * (static_cast<double>(term_freq) / max_freq);
}

double TfIdf::compute_idf(int total_docs, int doc_frequency) {
    if (doc_frequency == 0) return 0.0;
    return std::log10(static_cast<double>(total_docs) / doc_frequency);
}

double TfIdf::compute_tfidf(int term_freq, int max_freq, int total_docs, int doc_frequency) {
    return compute_tf(term_freq, max_freq) * compute_idf(total_docs, doc_frequency);
}

std::map<int, std::map<std::string, double>> TfIdf::compute_all(
    const std::map<int, std::map<std::string, int>>& per_doc_counts,
    const std::map<std::string, int>& doc_frequency,
    int total_docs) {
    
    std::map<int, std::map<std::string, double>> tfidf_scores;

    for (const auto& [doc_id, words] : per_doc_counts) {
        int max_freq = 0;
        for (const auto& [term, freq] : words) {
            if (freq > max_freq) max_freq = freq;
        }

        for (const auto& [term, freq] : words) {
            int df = 0;
            auto it = doc_frequency.find(term);
            if (it != doc_frequency.end()) df = it->second;

            double score = compute_tfidf(freq, max_freq, total_docs, df);
            tfidf_scores[doc_id][term] = score;
        }
    }

    return tfidf_scores;
}

std::vector<std::pair<std::string, double>> TfIdf::extract_keywords(
    const std::map<std::string, double>& tfidf_scores, int top_n) {
    
    std::vector<std::pair<std::string, double>> keywords(tfidf_scores.begin(), tfidf_scores.end());
    
    std::sort(keywords.begin(), keywords.end(),
        [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
            return a.second > b.second;
        });

    if (keywords.size() > static_cast<size_t>(top_n)) {
        keywords.resize(top_n);
    }
    return keywords;
}

std::vector<double> TfIdf::to_vector(
    const std::map<std::string, double>& tfidf_scores,
    const std::vector<std::string>& vocabulary) {
    
    std::vector<double> vec(vocabulary.size(), 0.0);
    for (size_t i = 0; i < vocabulary.size(); ++i) {
        auto it = tfidf_scores.find(vocabulary[i]);
        if (it != tfidf_scores.end()) {
            vec[i] = it->second;
        }
    }
    return vec;
}

std::map<std::string, int> TfIdf::compute_doc_frequencies(
    const std::map<int, std::map<std::string, int>>& per_doc_counts) {
    
    std::map<std::string, int> doc_frequency;
    for (const auto& [doc_id, words] : per_doc_counts) {
        for (const auto& [term, _] : words) {
            doc_frequency[term]++;
        }
    }
    return doc_frequency;
}

} // namespace paralex
