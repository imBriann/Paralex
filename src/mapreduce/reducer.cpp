/**
 * @file reducer.cpp
 * @brief Implementación de la fase Reduce
 */

#include "mapreduce/reducer.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>

namespace paralex {

ReduceResult Reducer::reduce(const std::vector<MapResult>& map_results) const {
    ReduceResult result;
    result.total_documents = static_cast<int>(map_results.size());

    for (const auto& mr : map_results) {
        reduce_into(result, mr);
    }

    // Calcular totales globales
    result.total_words = 0;
    for (const auto& [doc_id, total] : result.doc_total_words) {
        result.total_words += total;
    }
    result.total_unique_words = static_cast<int>(result.global_word_counts.size());

    LOG_INFO("Reducer", "Reduce completado: " + 
             std::to_string(result.total_documents) + " docs, " +
             std::to_string(result.total_words) + " palabras, " +
             std::to_string(result.total_unique_words) + " unicas");

    return result;
}

void Reducer::reduce_into(ReduceResult& accumulated, const MapResult& new_result) const {
    int doc_id = new_result.doc_id;

    // Merge word counts
    accumulated.per_doc_counts[doc_id] = std::map<std::string, int>(
        new_result.word_counts.begin(), new_result.word_counts.end());

    // Update global counts and inverted index
    for (const auto& [word, count] : new_result.word_counts) {
        accumulated.global_word_counts[word] += count;
        
        // Inverted index: agregar doc_id si no está ya
        auto& doc_list = accumulated.inverted_index[word];
        if (std::find(doc_list.begin(), doc_list.end(), doc_id) == doc_list.end()) {
            doc_list.push_back(doc_id);
        }
    }

    // Document metadata
    accumulated.doc_total_words[doc_id] = new_result.total_words;
    accumulated.doc_unique_words[doc_id] = new_result.unique_words;
    accumulated.doc_names[doc_id] = new_result.doc_name;
}

std::vector<std::tuple<std::string, int, double>> Reducer::get_top_words(
    const std::map<std::string, int>& global_counts, int top_n) {
    
    // Calculate total for percentages
    int total = 0;
    for (const auto& [_, count] : global_counts) {
        total += count;
    }

    // Sort by count descending
    std::vector<std::pair<std::string, int>> sorted(
        global_counts.begin(), global_counts.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Build result
    std::vector<std::tuple<std::string, int, double>> result;
    int limit = std::min(top_n, static_cast<int>(sorted.size()));
    for (int i = 0; i < limit; ++i) {
        double pct = total > 0 ? (static_cast<double>(sorted[i].second) / total) * 100.0 : 0.0;
        result.emplace_back(sorted[i].first, sorted[i].second, pct);
    }

    return result;
}

double Reducer::lexical_density(int unique_words, int total_words) {
    if (total_words == 0) return 0.0;
    return static_cast<double>(unique_words) / static_cast<double>(total_words);
}

double Reducer::lexical_diversity(const std::map<std::string, int>& word_counts) {
    if (word_counts.empty()) return 0.0;
    int total = 0;
    for (const auto& [_, count] : word_counts) {
        total += count;
    }
    return lexical_density(static_cast<int>(word_counts.size()), total);
}

} // namespace paralex
