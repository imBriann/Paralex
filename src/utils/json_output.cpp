/**
 * @file json_output.cpp
 * @brief Implementación del generador JSON
 */

#include "utils/json_output.h"
#include "utils/logger.h"
#include <fstream>
#include <algorithm>

namespace paralex {

std::string JsonWriter::escape(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 10);
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;      break;
        }
    }
    return result;
}

void JsonWriter::write_indent() {
    for (int i = 0; i < indent_level_; ++i) {
        buffer_ << "  ";
    }
}

JsonWriter& JsonWriter::start_object() {
    buffer_ << "{\n";
    indent_level_++;
    return *this;
}

JsonWriter& JsonWriter::end_object() {
    indent_level_--;
    buffer_ << "\n";
    write_indent();
    buffer_ << "}";
    return *this;
}

JsonWriter& JsonWriter::start_array() {
    buffer_ << "[\n";
    indent_level_++;
    return *this;
}

JsonWriter& JsonWriter::end_array() {
    indent_level_--;
    buffer_ << "\n";
    write_indent();
    buffer_ << "]";
    return *this;
}

JsonWriter& JsonWriter::key(const std::string& k) {
    write_indent();
    buffer_ << "\"" << escape(k) << "\": ";
    return *this;
}

JsonWriter& JsonWriter::value_string(const std::string& v) {
    buffer_ << "\"" << escape(v) << "\"";
    return *this;
}

JsonWriter& JsonWriter::value_int(int64_t v) {
    buffer_ << v;
    return *this;
}

JsonWriter& JsonWriter::value_double(double v, int precision) {
    buffer_ << std::fixed << std::setprecision(precision) << v;
    return *this;
}

JsonWriter& JsonWriter::value_bool(bool v) {
    buffer_ << (v ? "true" : "false");
    return *this;
}

JsonWriter& JsonWriter::value_null() {
    buffer_ << "null";
    return *this;
}

JsonWriter& JsonWriter::comma() {
    buffer_ << ",\n";
    return *this;
}

JsonWriter& JsonWriter::kv_string(const std::string& k, const std::string& v) {
    key(k); value_string(v);
    return *this;
}

JsonWriter& JsonWriter::kv_int(const std::string& k, int64_t v) {
    key(k); value_int(v);
    return *this;
}

JsonWriter& JsonWriter::kv_double(const std::string& k, double v, int precision) {
    key(k); value_double(v, precision);
    return *this;
}

JsonWriter& JsonWriter::kv_bool(const std::string& k, bool v) {
    key(k); value_bool(v);
    return *this;
}

void JsonWriter::write_map_string_int(const std::string& key_name,
                                       const std::map<std::string, int>& m) {
    key(key_name);
    start_object();
    bool first = true;
    for (const auto& [k, v] : m) {
        if (!first) comma();
        kv_int(k, v);
        first = false;
    }
    end_object();
}

void JsonWriter::write_map_string_double(const std::string& key_name,
                                          const std::map<std::string, double>& m) {
    key(key_name);
    start_object();
    bool first = true;
    for (const auto& [k, v] : m) {
        if (!first) comma();
        kv_double(k, v, 6);
        first = false;
    }
    end_object();
}

void JsonWriter::write_map_string_vec_int(const std::string& key_name,
                                           const std::map<std::string, std::vector<int>>& m) {
    key(key_name);
    start_object();
    bool first_outer = true;
    for (const auto& [k, vec] : m) {
        if (!first_outer) comma();
        key(k);
        buffer_ << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) buffer_ << ", ";
            buffer_ << vec[i];
        }
        buffer_ << "]";
        first_outer = false;
    }
    end_object();
}

bool JsonWriter::write_to_file(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("JsonWriter", "No se pudo escribir: " + filepath);
        return false;
    }
    file << buffer_.str();
    file.close();
    LOG_INFO("JsonWriter", "Resultado escrito: " + filepath);
    return true;
}

bool write_results_json(
    const std::string& filepath,
    const std::string& command,
    int num_processes,
    double time_ms,
    int total_docs,
    int total_words,
    int unique_words,
    const std::map<std::string, int>& global_word_counts,
    const std::map<int, std::map<std::string, int>>& per_doc_counts,
    const std::map<std::string, std::vector<int>>& inverted_index,
    const std::vector<std::tuple<std::string, int, double>>& top_words
) {
    JsonWriter jw;
    jw.start_object();
    
    jw.kv_string("command", command);
    jw.comma();
    jw.kv_int("num_processes", num_processes);
    jw.comma();
    jw.kv_double("processing_time_ms", time_ms, 2);
    jw.comma();
    jw.kv_int("total_documents", total_docs);
    jw.comma();
    jw.kv_int("total_words", total_words);
    jw.comma();
    jw.kv_int("unique_words", unique_words);

    // Top words
    jw.comma();
    jw.key("top_words");
    jw.start_array();
    for (size_t i = 0; i < top_words.size(); ++i) {
        if (i > 0) jw.comma();
        jw.write_indent();
        jw.start_object();
        jw.kv_string("word", std::get<0>(top_words[i]));
        jw.comma();
        jw.kv_int("count", std::get<1>(top_words[i]));
        jw.comma();
        jw.kv_double("percentage", std::get<2>(top_words[i]), 4);
        jw.end_object();
    }
    jw.end_array();

    // Global word counts (limited to top 200 for file size)
    jw.comma();
    std::map<std::string, int> limited_counts;
    int count = 0;
    // Sort by value descending
    std::vector<std::pair<std::string, int>> sorted_counts(
        global_word_counts.begin(), global_word_counts.end());
    std::sort(sorted_counts.begin(), sorted_counts.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    for (const auto& [k, v] : sorted_counts) {
        limited_counts[k] = v;
        if (++count >= 200) break;
    }
    jw.write_map_string_int("global_word_counts", limited_counts);

    // Per-document counts (top 50 per doc)
    jw.comma();
    jw.key("per_doc_counts");
    jw.start_object();
    bool first_doc = true;
    for (const auto& [doc_id, counts] : per_doc_counts) {
        if (!first_doc) jw.comma();
        std::map<std::string, int> top_50;
        std::vector<std::pair<std::string, int>> sorted(counts.begin(), counts.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        int n = 0;
        for (const auto& [k, v] : sorted) {
            top_50[k] = v;
            if (++n >= 50) break;
        }
        std::map<std::string, int> doc_map;
        for (const auto& [k, v] : top_50) doc_map[k] = v;
        jw.key(std::to_string(doc_id));
        jw.start_object();
        bool first_word = true;
        for (const auto& [k, v] : doc_map) {
            if (!first_word) jw.comma();
            jw.kv_int(k, v);
            first_word = false;
        }
        jw.end_object();
        first_doc = false;
    }
    jw.end_object();

    // Inverted index
    jw.comma();
    jw.write_map_string_vec_int("inverted_index", inverted_index);

    jw.end_object();
    return jw.write_to_file(filepath);
}

} // namespace paralex
