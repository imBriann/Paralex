/**
 * @file document.cpp
 * @brief Implementación de carga de documentos y manifiesto
 */

#include "core/document.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace paralex {

std::vector<Document> load_manifest(const std::string& manifest_path) {
    std::vector<Document> docs;
    std::ifstream file(manifest_path);
    
    if (!file.is_open()) {
        LOG_ERROR("Document", "No se pudo abrir manifiesto: " + manifest_path);
        return docs;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string token;
        Document doc;

        if (!std::getline(iss, token, '\t')) continue;
        doc.id = std::stoi(token);

        if (!std::getline(iss, doc.name, '\t')) continue;
        if (!std::getline(iss, doc.path, '\t')) continue;
        if (!std::getline(iss, doc.format, '\t')) continue;

        if (std::getline(iss, token, '\t')) {
            doc.size_bytes = std::stoull(token);
        }

        // Read the corpus file content
        doc.text = read_corpus_file(doc.path);
        
        if (doc.text.empty()) {
            LOG_WARN("Document", "Archivo vacio o no legible: " + doc.path);
            continue;
        }

        docs.push_back(doc);
        LOG_INFO("Document", "Cargado: " + doc.name + " (" + 
                 std::to_string(doc.text.size()) + " bytes)");
    }

    LOG_INFO("Document", "Total documentos cargados: " + std::to_string(docs.size()));
    return docs;
}

std::string read_corpus_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Document", "No se pudo leer corpus: " + path);
        return "";
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

} // namespace paralex
