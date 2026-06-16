/**
 * @file engine.h
 * @brief Motor MapReduce distribuido con MPI (Versión Estabilizada)
 */

#ifndef PARALEX_ENGINE_H
#define PARALEX_ENGINE_H

#include <string>
#include <vector>
#include <mpi.h>
#include "core/document.h"
#include "utils/text_processor.h"
#include "mapreduce/mapper.h"
#include "mapreduce/reducer.h"

namespace paralex {

class MapReduceEngine {
public:
    MapReduceEngine(int rank, int world_size);

    void load_config(const std::string& data_dir);

    /**
     * Ejecuta el pipeline completo MapReduce Básico.
     */
    void process(const std::string& manifest_path, const std::string& output_path);
    void compute_tfidf(const std::string& manifest_path, const std::string& output_path);
    void search(const std::string& query, const std::string& manifest_path, const std::string& output_path);
    void compute_similarity(const std::string& manifest_path, const std::string& output_path);
    void benchmark(const std::string& manifest_path, const std::string& output_path);

    int get_rank() const { return rank_; }
    int get_world_size() const { return world_size_; }

private:
    int rank_;
    int world_size_;
    TextProcessor processor_;
    Mapper mapper_;
    Reducer reducer_;

    std::vector<Document> distribute_documents(const std::vector<Document>& all_docs);
    std::vector<MapResult> gather_map_results(const std::vector<MapResult>& local_results);
    void broadcast_string(std::string& data, int root = 0);
    ReduceResult process_sequential(const std::vector<Document>& docs);
    ReduceResult process_parallel(const std::vector<Document>& all_docs);
};

} // namespace paralex

#endif // PARALEX_ENGINE_H
