/**
 * @file pipeline.h
 * @brief Orquestador del pipeline completo de procesamiento
 */

#ifndef PARALEX_PIPELINE_H
#define PARALEX_PIPELINE_H

#include <string>

namespace paralex {

/**
 * Punto de entrada principal que despacha comandos al motor MapReduce.
 * @param argc Argumentos de línea de comando
 * @param argv Valores de argumentos
 * @return Código de salida (0 = éxito)
 * 
 * Comandos soportados:
 *   process    <data_dir> <output_path>           Pipeline completo
 *   tfidf      <data_dir> <output_path>           Cálculo TF-IDF
 *   search     <data_dir> <output_path> <query>   Búsqueda
 *   similarity <data_dir> <output_path>           Similitud coseno
 *   benchmark  <data_dir> <output_path>           Benchmark seq vs par
 */
int run_pipeline(int argc, char** argv);

} // namespace paralex

#endif // PARALEX_PIPELINE_H
