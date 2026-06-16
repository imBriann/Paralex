/**
 * @file document.h
 * @brief Modelo de documento con serialización MPI
 */

#ifndef PARALEX_DOCUMENT_H
#define PARALEX_DOCUMENT_H

#include <string>
#include <vector>
#include <sstream>

namespace paralex {

struct Document {
    int id;
    std::string name;
    std::string path;
    std::string format;
    std::string text;
    size_t size_bytes;

    Document() : id(-1), size_bytes(0) {}

    Document(int id, const std::string& name, const std::string& path,
             const std::string& format, const std::string& text, size_t size)
        : id(id), name(name), path(path), format(format), text(text), size_bytes(size) {}

    /**
     * Serializa el documento a un string para envío MPI.
     * Formato: id\tname\tpath\tformat\tsize\n<text>
     */
    std::string serialize() const {
        std::ostringstream oss;
        oss << id << "\t" << name << "\t" << path << "\t" 
            << format << "\t" << size_bytes << "\n" << text;
        return oss.str();
    }

    /**
     * Deserializa un documento desde un string recibido por MPI.
     */
    static Document deserialize(const std::string& data) {
        Document doc;
        size_t header_end = data.find('\n');
        if (header_end == std::string::npos) return doc;

        std::string header = data.substr(0, header_end);
        doc.text = data.substr(header_end + 1);

        std::istringstream iss(header);
        std::string token;

        if (std::getline(iss, token, '\t')) doc.id = std::stoi(token);
        if (std::getline(iss, token, '\t')) doc.name = token;
        if (std::getline(iss, token, '\t')) doc.path = token;
        if (std::getline(iss, token, '\t')) doc.format = token;
        if (std::getline(iss, token, '\t')) doc.size_bytes = std::stoull(token);

        return doc;
    }

    bool is_valid() const {
        return id >= 0 && !name.empty() && !text.empty();
    }
};

/**
 * Carga el manifiesto de documentos desde archivo TSV.
 * Formato por línea: id\tname\tcorpus_path\tformat\tsize
 */
std::vector<Document> load_manifest(const std::string& manifest_path);

/**
 * Lee el contenido de texto de un archivo corpus.
 */
std::string read_corpus_file(const std::string& path);

} // namespace paralex

#endif // PARALEX_DOCUMENT_H
