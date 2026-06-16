/**
 * @file mapper.h
 * @brief Fase Map del motor MapReduce
 * 
 * Procesa documentos individuales y genera pares clave-valor
 * (término, frecuencia) con combiner local.
 */

#ifndef PARALEX_MAPPER_H
#define PARALEX_MAPPER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "core/document.h"
#include "utils/text_processor.h"

namespace paralex {

/**
 * Resultado del Map para un documento individual.
 */
struct MapResult {
    int doc_id;
    std::string doc_name;
    std::unordered_map<std::string, int> word_counts;      // término → frecuencia
    std::unordered_map<std::string, std::vector<int>> positions; // término → posiciones
    int total_words;
    int unique_words;

    MapResult() : doc_id(-1), total_words(0), unique_words(0) {}
};

class Mapper {
public:
    explicit Mapper(const TextProcessor& processor);

    /**
     * Ejecuta la fase Map sobre un documento.
     * Tokeniza, preprocesa y cuenta frecuencias con combiner local.
     * @param doc Documento a procesar
     * @return Resultado con conteos y posiciones de términos
     */
    MapResult map(const Document& doc) const;

    /**
     * Ejecuta Map sobre múltiples documentos (lote local).
     * @param docs Vector de documentos
     * @return Vector de resultados Map
     */
    std::vector<MapResult> map_batch(const std::vector<Document>& docs) const;

    /**
     * Serializa un MapResult para envío MPI.
     * Formato: doc_id\ttotal\tunique\nword\tcount\tpos1,pos2,...\n...
     */
    static std::string serialize_result(const MapResult& result);

    /**
     * Deserializa un MapResult recibido por MPI.
     */
    static MapResult deserialize_result(const std::string& data);

    /**
     * Serializa múltiples MapResults separados por un delimitador.
     */
    static std::string serialize_batch(const std::vector<MapResult>& results);

    /**
     * Deserializa múltiples MapResults.
     */
    static std::vector<MapResult> deserialize_batch(const std::string& data);

private:
    const TextProcessor& processor_;
};

} // namespace paralex

#endif // PARALEX_MAPPER_H
