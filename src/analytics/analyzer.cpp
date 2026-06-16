/**
 * @file analyzer.cpp
 * @brief Implementación de Analyzer para estadísticas y tópicos
 */

#include "analytics/analyzer.h"
#include "indexing/tfidf.h"
#include <algorithm>
#include <set>

namespace paralex {

CorpusStats Analyzer::compute_corpus_stats(
    const std::map<int, std::map<std::string, int>>& per_doc_counts,
    const std::map<std::string, int>& global_counts,
    const std::map<int, std::string>& doc_names,
    const std::map<int, std::map<std::string, double>>& tfidf_scores) {
    
    CorpusStats stats;
    stats.total_documents = per_doc_counts.size();
    stats.total_unique_words = global_counts.size();
    stats.total_words = 0;

    for (const auto& [term, freq] : global_counts) {
        stats.total_words += freq;
        stats.top_words.emplace_back(term, freq);
    }
    
    std::sort(stats.top_words.begin(), stats.top_words.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (stats.top_words.size() > 50) stats.top_words.resize(50);

    stats.avg_doc_length = stats.total_documents > 0 ? 
                           static_cast<double>(stats.total_words) / stats.total_documents : 0.0;
    
    stats.global_lexical_density = stats.total_words > 0 ?
                                   static_cast<double>(stats.total_unique_words) / stats.total_words : 0.0;

    for (const auto& [doc_id, words] : per_doc_counts) {
        DocStats ds;
        ds.doc_id = doc_id;
        ds.name = doc_names.count(doc_id) ? doc_names.at(doc_id) : "Unknown";
        ds.total_words = 0;
        ds.unique_words = words.size();
        
        for (const auto& [_, freq] : words) {
            ds.total_words += freq;
        }

        ds.lexical_density = ds.total_words > 0 ? 
                             static_cast<double>(ds.unique_words) / ds.total_words : 0.0;
        
        // Simplified diversity (TTR)
        ds.lexical_diversity = ds.lexical_density; 

        if (tfidf_scores.count(doc_id)) {
            ds.keywords = TfIdf::extract_keywords(tfidf_scores.at(doc_id), 10);
        }

        stats.doc_stats.push_back(ds);
    }

    return stats;
}

std::vector<DetectedTopic> Analyzer::detect_topics(
    const std::map<int, std::map<std::string, double>>& tfidf_scores,
    int num_keywords_per_doc,
    int max_topics) {
    
    std::vector<DetectedTopic> topics;
    std::vector<std::pair<int, std::vector<std::string>>> doc_keywords;

    for (const auto& [doc_id, scores] : tfidf_scores) {
        auto kw = TfIdf::extract_keywords(scores, num_keywords_per_doc);
        std::vector<std::string> words;
        for (const auto& p : kw) words.push_back(p.first);
        doc_keywords.emplace_back(doc_id, words);
    }

    // Very naive clustering for demo purposes
    int topic_id = 1;
    std::vector<bool> assigned(doc_keywords.size(), false);

    for (size_t i = 0; i < doc_keywords.size() && topics.size() < static_cast<size_t>(max_topics); ++i) {
        if (assigned[i]) continue;
        
        DetectedTopic t;
        t.topic_id = topic_id++;
        t.doc_ids.push_back(doc_keywords[i].first);
        std::set<std::string> kw_set(doc_keywords[i].second.begin(), doc_keywords[i].second.end());

        assigned[i] = true;

        for (size_t j = i + 1; j < doc_keywords.size(); ++j) {
            if (assigned[j]) continue;
            
            int match = 0;
            for (const auto& w : doc_keywords[j].second) {
                if (kw_set.count(w)) match++;
            }

            if (match >= 2) { // Arbitrary threshold
                t.doc_ids.push_back(doc_keywords[j].first);
                assigned[j] = true;
                for (const auto& w : doc_keywords[j].second) kw_set.insert(w);
            }
        }

        t.keywords = std::vector<std::string>(kw_set.begin(), kw_set.end());
        if (t.keywords.size() > 10) t.keywords.resize(10);
        t.coherence_score = static_cast<double>(t.doc_ids.size()) * t.keywords.size();
        topics.push_back(t);
    }

    std::sort(topics.begin(), topics.end(), 
        [](const DetectedTopic& a, const DetectedTopic& b) {
            return a.coherence_score > b.coherence_score;
        });

    return topics;
}

std::vector<std::pair<int, double>> Analyzer::rank_documents_by_importance(
    const std::map<int, std::map<std::string, int>>& per_doc_counts,
    const std::map<int, std::map<std::string, double>>& tfidf_scores) {
    
    std::vector<std::pair<int, double>> ranking;
    for (const auto& [doc_id, words] : per_doc_counts) {
        double score = words.size(); // Unique words contribution
        if (tfidf_scores.count(doc_id)) {
            for (const auto& [_, tfidf] : tfidf_scores.at(doc_id)) {
                score += tfidf; // TF-IDF contribution
            }
        }
        ranking.emplace_back(doc_id, score);
    }

    std::sort(ranking.begin(), ranking.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
        
    return ranking;
}

} // namespace paralex
