/**
 * @file json_output.h
 * @brief Generador JSON ligero para salida de resultados
 * 
 * Construye objetos JSON sin dependencias externas.
 * Soporta objetos anidados, arrays, strings, números y booleanos.
 */

#ifndef PARALEX_JSON_OUTPUT_H
#define PARALEX_JSON_OUTPUT_H

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <iomanip>

namespace paralex {

class JsonWriter {
public:
    JsonWriter() : indent_level_(0) {}

    /** Escapa caracteres especiales en strings JSON. */
    static std::string escape(const std::string& s);

    /** Construye un string JSON con indentación. */
    std::string build() const { return buffer_.str(); }

    // ---- Primitivas ----
    JsonWriter& start_object();
    JsonWriter& end_object();
    JsonWriter& start_array();
    JsonWriter& end_array();

    JsonWriter& key(const std::string& k);
    JsonWriter& value_string(const std::string& v);
    JsonWriter& value_int(int64_t v);
    JsonWriter& value_double(double v, int precision = 6);
    JsonWriter& value_bool(bool v);
    JsonWriter& value_null();
    JsonWriter& comma();

    // ---- Helpers de alto nivel ----
    
    /** Escribe un par clave:valor string. */
    JsonWriter& kv_string(const std::string& k, const std::string& v);
    /** Escribe un par clave:valor entero. */
    JsonWriter& kv_int(const std::string& k, int64_t v);
    /** Escribe un par clave:valor double. */
    JsonWriter& kv_double(const std::string& k, double v, int precision = 6);
    /** Escribe un par clave:valor booleano. */
    JsonWriter& kv_bool(const std::string& k, bool v);

    /** Escribe un map<string,int> como objeto JSON. */
    void write_map_string_int(const std::string& key_name,
                              const std::map<std::string, int>& m);

    /** Escribe un map<string,double> como objeto JSON. */
    void write_map_string_double(const std::string& key_name,
                                 const std::map<std::string, double>& m);

    /** Escribe un map<string, vector<int>> como objeto JSON. */
    void write_map_string_vec_int(const std::string& key_name,
                                  const std::map<std::string, std::vector<int>>& m);

    /** Escribe el JSON resultante a un archivo. */
    bool write_to_file(const std::string& filepath) const;

    void write_indent();
private:
    std::ostringstream buffer_;
    int indent_level_;
};

/**
 * Convierte un resultado de procesamiento completo a JSON y lo escribe a archivo.
 */
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
);

} // namespace paralex

#endif // PARALEX_JSON_OUTPUT_H
