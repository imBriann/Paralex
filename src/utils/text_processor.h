/**
 * @file text_processor.h
 * @brief Pipeline de preprocesamiento de texto con soporte multiidioma
 * 
 * Implementa tokenización, normalización, eliminación de stopwords,
 * y limpieza de texto con pipeline configurable.
 */

#ifndef PARALEX_TEXT_PROCESSOR_H
#define PARALEX_TEXT_PROCESSOR_H

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <functional>

namespace paralex {

class TextProcessor {
public:
    TextProcessor();

    /**
     * Carga stopwords desde un archivo (una palabra por línea).
     * @param filepath Ruta al archivo de stopwords
     * @return Número de stopwords cargadas
     */
    size_t load_stopwords(const std::string& filepath);

    /** Agrega stopwords manualmente. */
    void add_stopwords(const std::vector<std::string>& words);

    /** Configura si se eliminan números. */
    void set_remove_numbers(bool remove) { remove_numbers_ = remove; }

    /** Convierte texto a minúsculas (UTF-8 safe para ASCII). */
    static std::string to_lowercase(const std::string& text);

    /** Elimina caracteres especiales, manteniendo solo letras y espacios. */
    static std::string remove_special_chars(const std::string& text);

    /** Elimina dígitos del texto. */
    static std::string remove_digits(const std::string& text);

    /** Tokeniza texto en palabras individuales. */
    static std::vector<std::string> tokenize(const std::string& text);

    /** Elimina stopwords de un vector de tokens. */
    std::vector<std::string> filter_stopwords(const std::vector<std::string>& tokens) const;

    /** Elimina tokens con longitud menor a min_length. */
    static std::vector<std::string> filter_short_tokens(
        const std::vector<std::string>& tokens, size_t min_length = 2);

    /**
     * Pipeline completo de procesamiento.
     * lowercase → remove_special → (remove_digits) → tokenize → filter_stopwords → filter_short
     * @param text Texto crudo
     * @return Vector de tokens procesados
     */
    std::vector<std::string> process(const std::string& text) const;

    /** Retorna el conjunto de stopwords cargadas. */
    const std::unordered_set<std::string>& get_stopwords() const { return stopwords_; }

    /** Serializa stopwords a string (para broadcast MPI). */
    std::string serialize_stopwords() const;

    /** Deserializa stopwords desde string (recibido por MPI). */
    void deserialize_stopwords(const std::string& data);

private:
    std::unordered_set<std::string> stopwords_;
    bool remove_numbers_;
};

} // namespace paralex

#endif // PARALEX_TEXT_PROCESSOR_H
