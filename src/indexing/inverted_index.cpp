/**
 * @file inverted_index.cpp
 * @brief Implementación del índice invertido en memoria
 */

#include "indexing/inverted_index.h"
#include <algorithm>
#include <iterator>

namespace paralex {

void InvertedIndex::build(const std::map<int, std::map<std::string, int>>& per_doc_counts) {
    index_.clear();
    for (const auto& [doc_id, words] : per_doc_counts) {
        for (const auto& [term, freq] : words) {
            index_[term].emplace_back(doc_id, freq);
        }
    }
    
    // Sort postings by document ID for efficient intersection
    for (auto& [term, postings] : index_) {
        std::sort(postings.begin(), postings.end(), [](const Posting& a, const Posting& b) {
            return a.doc_id < b.doc_id;
        });
    }
}

void InvertedIndex::add_document(int doc_id, const std::map<std::string, int>& word_counts) {
    for (const auto& [term, freq] : word_counts) {
        index_[term].emplace_back(doc_id, freq);
    }
}

std::vector<Posting> InvertedIndex::lookup(const std::string& term) const {
    auto it = index_.find(term);
    if (it != index_.end()) {
        std::vector<Posting> postings = it->second;
        // Sort by term frequency descending for relevance
        std::sort(postings.begin(), postings.end(), [](const Posting& a, const Posting& b) {
            return a.term_frequency > b.term_frequency;
        });
        return postings;
    }
    return {};
}

std::vector<int> InvertedIndex::search_and(const std::vector<std::string>& terms) const {
    if (terms.empty()) return {};

    std::vector<std::vector<int>> all_doc_ids;
    for (const auto& term : terms) {
        auto it = index_.find(term);
        if (it == index_.end()) return {}; // If any term is missing, AND fails

        std::vector<int> doc_ids;
        for (const auto& p : it->second) {
            doc_ids.push_back(p.doc_id);
        }
        all_doc_ids.push_back(doc_ids);
    }

    std::vector<int> result = all_doc_ids[0];
    for (size_t i = 1; i < all_doc_ids.size(); ++i) {
        std::vector<int> intersection;
        std::set_intersection(result.begin(), result.end(),
                              all_doc_ids[i].begin(), all_doc_ids[i].end(),
                              std::back_inserter(intersection));
        result = intersection;
    }
    return result;
}

std::map<int, int> InvertedIndex::search_or(const std::vector<std::string>& terms) const {
    std::map<int, int> doc_match_count;
    for (const auto& term : terms) {
        auto it = index_.find(term);
        if (it != index_.end()) {
            for (const auto& p : it->second) {
                doc_match_count[p.doc_id]++;
            }
        }
    }
    return doc_match_count;
}

int InvertedIndex::document_frequency(const std::string& term) const {
    auto it = index_.find(term);
    return (it != index_.end()) ? static_cast<int>(it->second.size()) : 0;
}

std::vector<std::string> InvertedIndex::get_vocabulary() const {
    std::vector<std::string> vocab;
    vocab.reserve(index_.size());
    for (const auto& [term, _] : index_) {
        vocab.push_back(term);
    }
    return vocab;
}

std::map<std::string, std::vector<int>> InvertedIndex::to_map() const {
    std::map<std::string, std::vector<int>> result;
    for (const auto& [term, postings] : index_) {
        for (const auto& p : postings) {
            result[term].push_back(p.doc_id);
        }
    }
    return result;
}

} // namespace paralex
