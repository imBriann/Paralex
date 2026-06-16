/**
 * @file mapper.cpp
 * @brief Implementación de la fase Map con combiner local
 */

#include "mapreduce/mapper.h"
#include "utils/logger.h"
#include <sstream>

namespace paralex {

Mapper::Mapper(const TextProcessor& processor) : processor_(processor) {}

MapResult Mapper::map(const Document& doc) const {
    MapResult result;
    result.doc_id = doc.id;
    result.doc_name = doc.name;

    // Tokenizar y preprocesar
    auto tokens = processor_.process(doc.text);
    result.total_words = static_cast<int>(tokens.size());

    // Fase Map: generar pares (término, 1) con combiner local
    for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
        const auto& token = tokens[i];
        result.word_counts[token]++;
        result.positions[token].push_back(i);
    }

    result.unique_words = static_cast<int>(result.word_counts.size());

    LOG_INFO("Mapper", "Map completado: " + doc.name + 
             " | palabras=" + std::to_string(result.total_words) +
             " | unicas=" + std::to_string(result.unique_words));

    return result;
}

std::vector<MapResult> Mapper::map_batch(const std::vector<Document>& docs) const {
    std::vector<MapResult> results;
    results.reserve(docs.size());
    
    for (const auto& doc : docs) {
        try {
            results.push_back(map(doc));
        } catch (const std::exception& e) {
            LOG_ERROR("Mapper", "Error procesando " + doc.name + ": " + e.what());
            // Tolerancia a fallos: continuar con el siguiente documento
        }
    }
    
    return results;
}

std::string Mapper::serialize_result(const MapResult& result) {
    std::ostringstream oss;
    // Header: doc_id, doc_name, total_words, unique_words
    oss << result.doc_id << "\t" << result.doc_name << "\t" 
        << result.total_words << "\t" << result.unique_words << "\n";
    
    // Word counts with positions
    for (const auto& [word, count] : result.word_counts) {
        oss << word << "\t" << count;
        
        // Positions
        auto pos_it = result.positions.find(word);
        if (pos_it != result.positions.end()) {
            oss << "\t";
            for (size_t i = 0; i < pos_it->second.size(); ++i) {
                if (i > 0) oss << ",";
                oss << pos_it->second[i];
            }
        }
        oss << "\n";
    }
    
    return oss.str();
}

MapResult Mapper::deserialize_result(const std::string& data) {
    MapResult result;
    std::istringstream iss(data);
    std::string line;

    // Parse header
    if (std::getline(iss, line)) {
        std::istringstream header_ss(line);
        std::string token;
        if (std::getline(header_ss, token, '\t')) result.doc_id = std::stoi(token);
        if (std::getline(header_ss, token, '\t')) result.doc_name = token;
        if (std::getline(header_ss, token, '\t')) result.total_words = std::stoi(token);
        if (std::getline(header_ss, token, '\t')) result.unique_words = std::stoi(token);
    }

    // Parse word counts
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        std::istringstream line_ss(line);
        std::string word, count_str, pos_str;
        
        if (!std::getline(line_ss, word, '\t')) continue;
        if (!std::getline(line_ss, count_str, '\t')) continue;
        
        result.word_counts[word] = std::stoi(count_str);
        
        if (std::getline(line_ss, pos_str, '\t')) {
            std::istringstream pos_ss(pos_str);
            std::string pos_token;
            while (std::getline(pos_ss, pos_token, ',')) {
                if (!pos_token.empty()) {
                    result.positions[word].push_back(std::stoi(pos_token));
                }
            }
        }
    }

    return result;
}

std::string Mapper::serialize_batch(const std::vector<MapResult>& results) {
    std::ostringstream oss;
    oss << results.size() << "\n";
    
    for (const auto& r : results) {
        std::string serialized = serialize_result(r);
        oss << serialized.size() << "\n";
        oss << serialized;
    }
    
    return oss.str();
}

std::vector<MapResult> Mapper::deserialize_batch(const std::string& data) {
    std::vector<MapResult> results;
    std::istringstream iss(data);
    std::string line;
    
    if (!std::getline(iss, line)) return results;
    int count = std::stoi(line);
    
    for (int i = 0; i < count; ++i) {
        if (!std::getline(iss, line)) break;
        int size = std::stoi(line);
        
        std::string chunk(size, '\0');
        iss.read(&chunk[0], size);
        
        results.push_back(deserialize_result(chunk));
    }
    
    return results;
}

} // namespace paralex
