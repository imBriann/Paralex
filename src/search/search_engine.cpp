/**
 * @file search_engine.cpp
 * @brief Implementación del motor de búsqueda con ranking TF-IDF
 */

#include "search/search_engine.h"
#include <algorithm>

namespace paralex {

SearchEngine::SearchEngine(const TextProcessor& processor)
    : processor_(processor), index_(nullptr), tfidf_scores_(nullptr), doc_names_(nullptr), total_docs_(0) {}

void SearchEngine::configure(
    const InvertedIndex& index,
    const std::map<int, std::map<std::string, double>>& tfidf_scores,
    const std::map<int, std::string>& doc_names,
    int total_docs) {
    index_ = &index;
    tfidf_scores_ = &tfidf_scores;
    doc_names_ = &doc_names;
    total_docs_ = total_docs;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, int max_results) const {
    if (!index_ || !tfidf_scores_ || query.empty()) return {};

    if (is_exact_phrase(query)) {
        return search_exact_phrase(query);
    }

    std::vector<std::string> terms = processor_.process(query);
    if (terms.empty()) return {};

    auto results = search_terms(terms);

    if (results.size() > static_cast<size_t>(max_results)) {
        results.resize(max_results);
    }

    return results;
}

bool SearchEngine::is_exact_phrase(const std::string& query) const {
    return query.length() >= 2 && query.front() == '"' && query.back() == '"';
}

std::vector<SearchResult> SearchEngine::search_exact_phrase(const std::string& phrase) const {
    std::string inner = phrase.substr(1, phrase.length() - 2);
    std::vector<std::string> terms = processor_.process(inner);
    if (terms.empty()) return {};

    std::vector<int> doc_ids = index_->search_and(terms);
    std::vector<SearchResult> results;

    for (int doc_id : doc_ids) {
        double total_score = 0;
        for (const auto& term : terms) {
            auto it = tfidf_scores_->find(doc_id);
            if (it != tfidf_scores_->end()) {
                auto term_it = it->second.find(term);
                if (term_it != it->second.end()) {
                    total_score += term_it->second;
                }
            }
        }

        std::string name = doc_names_->count(doc_id) ? doc_names_->at(doc_id) : "Unknown";
        results.emplace_back(doc_id, name, total_score, terms.size());
    }

    std::sort(results.begin(), results.end(), std::greater<SearchResult>());
    return results;
}

std::vector<SearchResult> SearchEngine::search_terms(const std::vector<std::string>& terms) const {
    std::map<int, double> doc_scores;
    std::map<int, int> doc_matches;

    for (const auto& term : terms) {
        auto postings = index_->lookup(term);
        for (const auto& p : postings) {
            int doc_id = p.doc_id;
            doc_matches[doc_id]++;

            double score = 0;
            auto it = tfidf_scores_->find(doc_id);
            if (it != tfidf_scores_->end()) {
                auto term_it = it->second.find(term);
                if (term_it != it->second.end()) {
                    score = term_it->second;
                }
            }
            doc_scores[doc_id] += score;
        }
    }

    std::vector<SearchResult> results;
    for (const auto& [doc_id, score] : doc_scores) {
        // Boost score based on number of terms matched
        double final_score = score * (1.0 + 0.5 * doc_matches[doc_id]);
        std::string name = doc_names_->count(doc_id) ? doc_names_->at(doc_id) : "Unknown";
        results.emplace_back(doc_id, name, final_score, doc_matches[doc_id]);
    }

    std::sort(results.begin(), results.end(), std::greater<SearchResult>());
    return results;
}

} // namespace paralex
