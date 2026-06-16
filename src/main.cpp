/**
 * @file main.cpp
 * @brief Punto de entrada de la aplicación MPI
 */

#include <mpi.h>
#include <iostream>
#include "core/pipeline.h"

int main(int argc, char** argv) {
    int provided;
    // Usamos MPI_Init_thread para mayor seguridad si es posible
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int result = 0;
    try {
        result = paralex::run_pipeline(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] " << e.what() << std::endl;
        result = 1;
    }

    MPI_Finalize();
    return result;
}
