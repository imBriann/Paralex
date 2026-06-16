/**
 * @file text_processor.cpp
 * @brief Implementación del pipeline de preprocesamiento de texto
 */

#include "utils/text_processor.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace paralex {

TextProcessor::TextProcessor() : remove_numbers_(true) {}

size_t TextProcessor::load_stopwords(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("TextProcessor", "No se pudo abrir stopwords: " + filepath);
        return 0;
    }

    std::string word;
    size_t count = 0;
    while (std::getline(file, word)) {
        // Trim whitespace
        word.erase(0, word.find_first_not_of(" \t\r\n"));
        word.erase(word.find_last_not_of(" \t\r\n") + 1);
        
        if (!word.empty()) {
            stopwords_.insert(to_lowercase(word));
            count++;
        }
    }

    LOG_INFO("TextProcessor", "Stopwords cargadas: " + std::to_string(count) + 
             " desde " + filepath);
    return count;
}

void TextProcessor::add_stopwords(const std::vector<std::string>& words) {
    for (const auto& w : words) {
        stopwords_.insert(to_lowercase(w));
    }
}

std::string TextProcessor::to_lowercase(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = text[i];
        if (c < 128) {
            result += static_cast<char>(std::tolower(c));
        } else if (c == 0xC3 && i + 1 < text.size()) {
            unsigned char next_c = text[i+1];
            // Spanish uppercase to lowercase
            if (next_c == 0x81) { result += "\xC3\xA1"; i++; } // Á -> á
            else if (next_c == 0x89) { result += "\xC3\xA9"; i++; } // É -> é
            else if (next_c == 0x8D) { result += "\xC3\xAD"; i++; } // Í -> í
            else if (next_c == 0x93) { result += "\xC3\xB3"; i++; } // Ó -> ó
            else if (next_c == 0x9A) { result += "\xC3\xBA"; i++; } // Ú -> ú
            else if (next_c == 0x91) { result += "\xC3\xB1"; i++; } // Ñ -> ñ
            else if (next_c == 0x9C) { result += "\xC3\xBC"; i++; } // Ü -> ü
            else { result += c; }
        } else {
            result += c;
        }
    }
    return result;
}

std::string TextProcessor::remove_special_chars(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = text[i];
        if (c < 128) {
            if (std::isalpha(c) || std::isspace(c) || std::isdigit(c)) {
                result += static_cast<char>(c);
            } else {
                result += ' ';
            }
        } else if (c == 0xC3 && i + 1 < text.size()) {
            // Keep spanish characters valid
            unsigned char next_c = text[i+1];
            if ((next_c >= 0xA1 && next_c <= 0xBC) || (next_c >= 0x81 && next_c <= 0x9C)) {
                result += c;
                result += next_c;
                i++;
            } else {
                result += ' ';
            }
        } else {
            result += ' ';
        }
    }
    return result;
}

std::string TextProcessor::remove_digits(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (unsigned char c : text) {
        if (!std::isdigit(c)) {
            result += static_cast<char>(c);
        }
    }
    return result;
}

std::vector<std::string> TextProcessor::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    
    while (stream >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::vector<std::string> TextProcessor::filter_stopwords(
    const std::vector<std::string>& tokens) const {
    std::vector<std::string> filtered;
    filtered.reserve(tokens.size());
    
    for (const auto& token : tokens) {
        if (stopwords_.find(token) == stopwords_.end()) {
            filtered.push_back(token);
        }
    }
    return filtered;
}

std::vector<std::string> TextProcessor::filter_short_tokens(
    const std::vector<std::string>& tokens, size_t min_length) {
    std::vector<std::string> filtered;
    filtered.reserve(tokens.size());
    
    for (const auto& token : tokens) {
        if (token.size() >= min_length) {
            filtered.push_back(token);
        }
    }
    return filtered;
}

std::vector<std::string> TextProcessor::process(const std::string& text) const {
    // Pipeline: lowercase → remove_special → (remove_digits) → tokenize → 
    //           filter_stopwords → filter_short
    std::string processed = to_lowercase(text);
    processed = remove_special_chars(processed);
    
    if (remove_numbers_) {
        processed = remove_digits(processed);
    }
    
    auto tokens = tokenize(processed);
    tokens = filter_stopwords(tokens);
    tokens = filter_short_tokens(tokens, 2);
    
    return tokens;
}

std::string TextProcessor::serialize_stopwords() const {
    std::ostringstream oss;
    for (const auto& sw : stopwords_) {
        oss << sw << "\n";
    }
    return oss.str();
}

void TextProcessor::deserialize_stopwords(const std::string& data) {
    stopwords_.clear();
    std::istringstream iss(data);
    std::string word;
    while (std::getline(iss, word)) {
        if (!word.empty()) {
            stopwords_.insert(word);
        }
    }
}

} // namespace paralex
