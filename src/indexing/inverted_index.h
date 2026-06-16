/**
 * @file inverted_index.h
 * @brief Índice invertido distribuido para recuperación de información
 */

#ifndef PARALEX_INVERTED_INDEX_H
#define PARALEX_INVERTED_INDEX_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace paralex {

/**
 * Posting: entrada del índice invertido para un término en un documento.
 */
struct Posting {
    int doc_id;
    int term_frequency;
    std::vector<int> positions;

    Posting() : doc_id(-1), term_frequency(0) {}
    Posting(int id, int tf) : doc_id(id), term_frequency(tf) {}
    Posting(int id, int tf, const std::vector<int>& pos) 
        : doc_id(id), term_frequency(tf), positions(pos) {}
};

class InvertedIndex {
public:
    /**
     * Construye el índice invertido a partir de resultados MapReduce.
     * @param per_doc_counts Conteos por documento: doc_id → {term → freq}
     */
    void build(const std::map<int, std::map<std::string, int>>& per_doc_counts);

    /**
     * Actualización incremental: agrega un documento al índice existente.
     */
    void add_document(int doc_id, const std::map<std::string, int>& word_counts);

    /**
     * Busca documentos que contienen un término.
     * @return Lista de postings ordenados por frecuencia descendente
     */
    std::vector<Posting> lookup(const std::string& term) const;

    /**
     * Busca documentos que contienen TODOS los términos (AND).
     * @return doc_ids que aparecen en todas las posting lists
     */
    std::vector<int> search_and(const std::vector<std::string>& terms) const;

    /**
     * Busca documentos que contienen AL MENOS UN término (OR).
     * @return doc_ids con su conteo de términos encontrados
     */
    std::map<int, int> search_or(const std::vector<std::string>& terms) const;

    /**
     * Obtiene el document frequency (número de documentos que contienen el término).
     */
    int document_frequency(const std::string& term) const;

    /**
     * Obtiene el tamaño del vocabulario (número de términos únicos indexados).
     */
    size_t vocabulary_size() const { return index_.size(); }

    /**
     * Obtiene todos los términos del vocabulario.
     */
    std::vector<std::string> get_vocabulary() const;

    /**
     * Convierte el índice a formato map para serialización JSON.
     */
    std::map<std::string, std::vector<int>> to_map() const;

    /** Acceso directo al índice interno. */
    const std::map<std::string, std::vector<Posting>>& get_index() const { return index_; }

private:
    std::map<std::string, std::vector<Posting>> index_; // término → postings
};

} // namespace paralex

#endif // PARALEX_INVERTED_INDEX_H
