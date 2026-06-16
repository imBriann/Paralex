/**
 * @file pipeline.cpp
 * @brief Orquestador básico del pipeline
 */

#include "core/pipeline.h"
#include "mapreduce/engine.h"
#include "utils/logger.h"
#include <iostream>

namespace paralex {

int run_pipeline(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Uso: paralex_engine <command> <data_dir> <output_path>\n"
                  << "Comandos disponibles: process\n";
        return 1;
    }

    std::string command = argv[1];
    std::string data_dir = argv[2];
    std::string output_path = argv[3];

    int rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    Logger::instance().init(data_dir + "/../logs", rank);
    
    try {
        MapReduceEngine engine(rank, world_size);
        engine.load_config(data_dir);

        if (command == "process") {
            std::string manifest_path = data_dir + "/manifest.txt";
            engine.process(manifest_path, output_path);
        } else if (command == "tfidf") {
            std::string manifest_path = data_dir + "/manifest.txt";
            engine.compute_tfidf(manifest_path, output_path);
        } else if (command == "search") {
            if (argc < 5) {
                if (rank == 0) std::cerr << "Falta query para busqueda\n";
                return 1;
            }
            std::string manifest_path = data_dir + "/manifest.txt";
            std::string query = argv[4];
            engine.search(query, manifest_path, output_path);
        } else if (command == "similarity") {
            std::string manifest_path = data_dir + "/manifest.txt";
            engine.compute_similarity(manifest_path, output_path);
        } else if (command == "benchmark") {
            std::string manifest_path = data_dir + "/manifest.txt";
            engine.benchmark(manifest_path, output_path);
        } else {
            if (rank == 0) {
                LOG_ERROR("Pipeline", "Comando no soportado: " + command);
            }
            return 1;
        }
    } catch (const std::exception& e) {
        LOG_FATAL("Pipeline", std::string("Excepcion: ") + e.what());
        return 1;
    }

    return 0;
}

} // namespace paralex
