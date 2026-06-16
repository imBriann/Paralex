/**
 * @file engine.cpp
 * @brief Motor MapReduce distribuido con MPI (Versión Estabilizada)
 */

#include "mapreduce/engine.h"
#include "indexing/tfidf.h"
#include "indexing/inverted_index.h"
#include "search/search_engine.h"
#include "similarity/cosine.h"
#include "analytics/analyzer.h"
#include "database/sqlite_manager.h"
#include "utils/json_output.h"
#include "utils/logger.h"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <numeric>

namespace paralex {

MapReduceEngine::MapReduceEngine(int rank, int world_size)
    : rank_(rank), world_size_(world_size), mapper_(processor_) {
    LOG_INFO("Engine", "Inicializado rank " + std::to_string(rank_) + 
             " de " + std::to_string(world_size_) + " procesos");
}

void MapReduceEngine::load_config(const std::string& data_dir) {
    std::string stopwords_data;

    if (rank_ == 0) {
        std::string es_path = data_dir + "/stopwords/spanish.txt";
        std::string en_path = data_dir + "/stopwords/english.txt";
        processor_.load_stopwords(es_path);
        processor_.load_stopwords(en_path);
        stopwords_data = processor_.serialize_stopwords();
        LOG_INFO("Engine", "Stopwords cargadas: " + 
                 std::to_string(processor_.get_stopwords().size()));
    }

    broadcast_string(stopwords_data, 0);

    if (rank_ != 0) {
        processor_.deserialize_stopwords(stopwords_data);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
}

void MapReduceEngine::broadcast_string(std::string& data, int root) {
    int length = static_cast<int>(data.size());
    MPI_Bcast(&length, 1, MPI_INT, root, MPI_COMM_WORLD);
    
    if (rank_ != root) {
        data.resize(length);
    }
    if (length > 0) {
        MPI_Bcast(&data[0], length, MPI_CHAR, root, MPI_COMM_WORLD);
    }
}

std::vector<Document> MapReduceEngine::distribute_documents(const std::vector<Document>& all_docs) {
    std::vector<Document> local_docs;
    int total_docs = static_cast<int>(all_docs.size());

    MPI_Bcast(&total_docs, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (total_docs == 0) {
        return local_docs; // Manejo de deadlock
    }

    if (rank_ == 0) {
        for (int i = 0; i < total_docs; ++i) {
            int target_rank = i % world_size_;
            if (target_rank == 0) {
                local_docs.push_back(all_docs[i]);
            } else {
                std::string serialized = all_docs[i].serialize();
                int data_size = static_cast<int>(serialized.size());
                MPI_Send(&data_size, 1, MPI_INT, target_rank, 0, MPI_COMM_WORLD);
                MPI_Send(serialized.c_str(), data_size, MPI_CHAR, target_rank, 1, MPI_COMM_WORLD);
            }
        }

        // Send termination signal
        for (int r = 1; r < world_size_; ++r) {
            int end_signal = -1;
            MPI_Send(&end_signal, 1, MPI_INT, r, 0, MPI_COMM_WORLD);
        }
        LOG_INFO("Engine", "Distribuidos " + std::to_string(total_docs) + " docs");
    } else {
        while (true) {
            int data_size;
            MPI_Recv(&data_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (data_size == -1) break;

            std::string buffer(data_size, '\0');
            MPI_Recv(&buffer[0], data_size, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            local_docs.push_back(Document::deserialize(buffer));
        }
    }
    return local_docs;
}

std::vector<MapResult> MapReduceEngine::gather_map_results(const std::vector<MapResult>& local_results) {
    std::vector<MapResult> all_results;
    std::string local_data = Mapper::serialize_batch(local_results);
    int local_size = static_cast<int>(local_data.size());

    if (rank_ == 0) {
        all_results = local_results;
        for (int r = 1; r < world_size_; ++r) {
            int remote_size;
            MPI_Recv(&remote_size, 1, MPI_INT, r, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            if (remote_size > 0) {
                std::string remote_data(remote_size, '\0');
                MPI_Recv(&remote_data[0], remote_size, MPI_CHAR, r, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                auto remote_results = Mapper::deserialize_batch(remote_data);
                all_results.insert(all_results.end(), remote_results.begin(), remote_results.end());
            }
        }
    } else {
        MPI_Send(&local_size, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        if (local_size > 0) {
            MPI_Send(local_data.c_str(), local_size, MPI_CHAR, 0, 3, MPI_COMM_WORLD);
        }
    }
    return all_results;
}

ReduceResult MapReduceEngine::process_parallel(const std::vector<Document>& all_docs) {
    auto local_docs = distribute_documents(all_docs);
    auto local_results = mapper_.map_batch(local_docs);
    auto all_results = gather_map_results(local_results);

    ReduceResult reduce_result;
    if (rank_ == 0) {
        reduce_result = reducer_.reduce(all_results);
    }
    return reduce_result;
}

void MapReduceEngine::process(const std::string& manifest_path, const std::string& output_path) {
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<Document> all_docs;
    if (rank_ == 0) {
        all_docs = load_manifest(manifest_path);
        if (all_docs.empty()) {
            LOG_ERROR("Engine", "No se cargaron documentos del manifiesto");
        }
    }

    auto result = process_parallel(all_docs);

    if (rank_ == 0 && !all_docs.empty()) {
        auto end = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        auto top_words = Reducer::get_top_words(result.global_word_counts, 50);

        SQLiteManager db_manager("paralex.db");
        if (db_manager.initialize()) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
            std::string current_date = ss.str();

            for (const auto& doc : all_docs) {
                db_manager.save_document(doc, result.doc_total_words[doc.id], result.doc_unique_words[doc.id], current_date);
            }
            db_manager.save_postings(result.per_doc_counts);
            db_manager.save_execution_history(current_date, result.total_documents, time_ms, world_size_, 0.0, 0.0, 0.0);
            LOG_INFO("Engine", "Índice guardado en SQLite exitosamente");
        }

        write_results_json(output_path, "process", world_size_, time_ms,
                          result.total_documents, result.total_words, 
                          result.total_unique_words,
                          result.global_word_counts, result.per_doc_counts,
                          result.inverted_index, top_words);

        std::cout << "{\"status\":\"success\","
                  << "\"command\":\"process\","
                  << "\"total_documents\":" << result.total_documents << ","
                  << "\"total_words\":" << result.total_words << ","
                  << "\"unique_words\":" << result.total_unique_words << ","
                  << "\"processing_time_ms\":" << std::fixed << std::setprecision(2) << time_ms << ","
                  << "\"num_processes\":" << world_size_ << "}" << std::endl;
    }
}

ReduceResult MapReduceEngine::process_sequential(const std::vector<Document>& docs) {
    Mapper seq_mapper(processor_);
    Reducer seq_reducer;
    std::vector<MapResult> map_results;
    for (const auto& doc : docs) {
        map_results.push_back(seq_mapper.map(doc));
    }
    return seq_reducer.reduce(map_results);
}

void MapReduceEngine::compute_tfidf(const std::string& manifest_path, const std::string& output_path) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Document> all_docs;
    if (rank_ == 0) all_docs = load_manifest(manifest_path);
    auto result = process_parallel(all_docs);

    if (rank_ == 0 && !all_docs.empty()) {
        auto doc_freq = TfIdf::compute_doc_frequencies(result.per_doc_counts);
        auto tfidf_scores = TfIdf::compute_all(result.per_doc_counts, doc_freq, result.total_documents);

        JsonWriter jw;
        jw.start_object();
        jw.kv_string("command", "tfidf"); jw.comma();
        jw.kv_int("num_processes", world_size_); jw.comma();

        jw.key("tfidf_scores"); jw.start_object();
        bool first_doc = true;
        for (const auto& [doc_id, scores] : tfidf_scores) {
            if (!first_doc) jw.comma();
            auto top_kw = TfIdf::extract_keywords(scores, 20);
            jw.key(std::to_string(doc_id)); jw.start_object();
            bool first = true;
            for (const auto& [w, s] : top_kw) {
                if (!first) jw.comma();
                jw.kv_double(w, s, 6);
                first = false;
            }
            jw.end_object();
            first_doc = false;
        }
        jw.end_object(); jw.comma();

        jw.key("keywords"); jw.start_object();
        first_doc = true;
        for (const auto& [doc_id, scores] : tfidf_scores) {
            if (!first_doc) jw.comma();
            auto keywords = TfIdf::extract_keywords(scores, 10);
            jw.key(std::to_string(doc_id)); jw.start_array();
            bool first_kw = true;
            for (const auto& [word, score] : keywords) {
                if (!first_kw) jw.comma();
                jw.start_object();
                jw.kv_string("word", word); jw.comma();
                jw.kv_double("score", score, 6); jw.comma();
                std::string doc_name = result.doc_names.count(doc_id) ? result.doc_names.at(doc_id) : "";
                jw.kv_string("doc_name", doc_name);
                jw.end_object();
                first_kw = false;
            }
            jw.end_array();
            first_doc = false;
        }
        jw.end_object(); jw.comma();

        std::map<std::string, int> sorted_df(doc_freq.begin(), doc_freq.end());
        jw.write_map_string_int("document_frequencies", sorted_df);

        auto end_total = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end_total - start).count();
        jw.comma(); jw.kv_double("processing_time_ms", time_ms, 2);
        jw.end_object();
        jw.write_to_file(output_path);

        std::cout << "{\"status\":\"success\",\"command\":\"tfidf\","
                  << "\"processing_time_ms\":" << std::fixed << std::setprecision(2) << time_ms << "}" << std::endl;
    }
}

void MapReduceEngine::search(const std::string& query, const std::string& manifest_path, const std::string& output_path) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Document> all_docs;
    if (rank_ == 0) all_docs = load_manifest(manifest_path);
    auto result = process_parallel(all_docs);

    if (rank_ == 0 && !all_docs.empty()) {
        InvertedIndex inv_index;
        inv_index.build(result.per_doc_counts);
        auto doc_freq = TfIdf::compute_doc_frequencies(result.per_doc_counts);
        auto tfidf_scores = TfIdf::compute_all(result.per_doc_counts, doc_freq, result.total_documents);

        SearchEngine search_eng(processor_);
        search_eng.configure(inv_index, tfidf_scores, result.doc_names, result.total_documents);
        auto search_results = search_eng.search(query, 20);

        auto end = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        JsonWriter jw;
        jw.start_object();
        jw.kv_string("command", "search"); jw.comma();
        jw.kv_string("query", query); jw.comma();
        jw.kv_int("num_results", static_cast<int>(search_results.size())); jw.comma();
        jw.kv_double("processing_time_ms", time_ms, 2); jw.comma();
        jw.kv_int("num_processes", world_size_); jw.comma();

        jw.key("results"); jw.start_array();
        for (size_t i = 0; i < search_results.size(); ++i) {
            if (i > 0) jw.comma();
            jw.start_object();
            jw.kv_int("doc_id", search_results[i].doc_id); jw.comma();
            jw.kv_string("doc_name", search_results[i].doc_name); jw.comma();
            jw.kv_double("score", search_results[i].score, 6); jw.comma();
            jw.kv_int("match_count", search_results[i].match_count);
            jw.end_object();
        }
        jw.end_array();
        jw.end_object();
        jw.write_to_file(output_path);

        std::cout << "{\"status\":\"success\",\"command\":\"search\","
                  << "\"query\":\"" << JsonWriter::escape(query) << "\","
                  << "\"num_results\":" << search_results.size() << ","
                  << "\"processing_time_ms\":" << std::fixed << std::setprecision(2) << time_ms
                  << "}" << std::endl;
    }
}

void MapReduceEngine::compute_similarity(const std::string& manifest_path, const std::string& output_path) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Document> all_docs;
    if (rank_ == 0) all_docs = load_manifest(manifest_path);
    auto result = process_parallel(all_docs);

    if (rank_ == 0 && !all_docs.empty()) {
        auto doc_freq = TfIdf::compute_doc_frequencies(result.per_doc_counts);
        auto tfidf_scores = TfIdf::compute_all(result.per_doc_counts, doc_freq, result.total_documents);

        std::vector<std::string> vocabulary;
        for (const auto& [term, _] : result.global_word_counts) vocabulary.push_back(term);
        std::sort(vocabulary.begin(), vocabulary.end());

        std::map<int, std::vector<double>> vectors;
        std::vector<int> doc_ids;
        for (const auto& [doc_id, scores] : tfidf_scores) {
            vectors[doc_id] = TfIdf::to_vector(scores, vocabulary);
            doc_ids.push_back(doc_id);
        }

        std::vector<std::pair<int, int>> all_pairs;
        for (size_t i = 0; i < doc_ids.size(); ++i) {
            for (size_t j = i + 1; j < doc_ids.size(); ++j) {
                all_pairs.emplace_back(doc_ids[i], doc_ids[j]);
            }
        }

        auto similarity_pairs = CosineSimilarity::compute_subset(vectors, all_pairs, result.doc_names);
        auto top_similar = CosineSimilarity::get_top_similar(similarity_pairs, 10);
        auto duplicates = CosineSimilarity::detect_duplicates(similarity_pairs, 0.90);
        auto matrix_2d = CosineSimilarity::to_matrix_2d(similarity_pairs, doc_ids);

        auto end = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        JsonWriter jw;
        jw.start_object();
        jw.kv_string("command", "similarity"); jw.comma();
        jw.kv_int("num_processes", world_size_); jw.comma();
        jw.kv_double("processing_time_ms", time_ms, 2); jw.comma();
        jw.kv_int("num_documents", result.total_documents); jw.comma();

        jw.key("doc_ids"); jw.start_array();
        for (size_t i = 0; i < doc_ids.size(); ++i) {
            if (i > 0) jw.comma();
            jw.value_int(doc_ids[i]);
        }
        jw.end_array(); jw.comma();

        jw.key("doc_names"); jw.start_object();
        bool first = true;
        for (const auto& [id, name] : result.doc_names) {
            if (!first) jw.comma();
            jw.kv_string(std::to_string(id), name);
            first = false;
        }
        jw.end_object(); jw.comma();

        jw.key("matrix"); jw.start_array();
        for (size_t i = 0; i < matrix_2d.size(); ++i) {
            if (i > 0) jw.comma();
            jw.start_array();
            for (size_t j = 0; j < matrix_2d[i].size(); ++j) {
                if (j > 0) jw.comma();
                jw.value_double(matrix_2d[i][j], 4);
            }
            jw.end_array();
        }
        jw.end_array(); jw.comma();

        jw.key("top_similar"); jw.start_array();
        for (size_t i = 0; i < top_similar.size(); ++i) {
            if (i > 0) jw.comma();
            jw.start_object();
            jw.kv_int("doc_id_1", top_similar[i].doc_id_1); jw.comma();
            jw.kv_int("doc_id_2", top_similar[i].doc_id_2); jw.comma();
            jw.kv_string("name_1", top_similar[i].name_1); jw.comma();
            jw.kv_string("name_2", top_similar[i].name_2); jw.comma();
            jw.kv_double("similarity", top_similar[i].similarity, 4);
            jw.end_object();
        }
        jw.end_array(); jw.comma();

        jw.key("duplicates"); jw.start_array();
        for (size_t i = 0; i < duplicates.size(); ++i) {
            if (i > 0) jw.comma();
            jw.start_object();
            jw.kv_string("name_1", duplicates[i].name_1); jw.comma();
            jw.kv_string("name_2", duplicates[i].name_2); jw.comma();
            jw.kv_double("similarity", duplicates[i].similarity, 4);
            jw.end_object();
        }
        jw.end_array();
        jw.end_object();
        jw.write_to_file(output_path);

        std::cout << "{\"status\":\"success\",\"command\":\"similarity\","
                  << "\"num_documents\":" << result.total_documents << ","
                  << "\"num_pairs\":" << similarity_pairs.size() << ","
                  << "\"num_duplicates\":" << duplicates.size() << ","
                  << "\"processing_time_ms\":" << std::fixed << std::setprecision(2) << time_ms
                  << "}" << std::endl;
    }
}

void MapReduceEngine::benchmark(const std::string& manifest_path, const std::string& output_path) {
    std::vector<Document> all_docs;
    if (rank_ == 0) all_docs = load_manifest(manifest_path);

    double seq_time_ms = 0;
    if (rank_ == 0 && !all_docs.empty()) {
        auto start_seq = std::chrono::high_resolution_clock::now();
        process_sequential(all_docs);
        auto end_seq = std::chrono::high_resolution_clock::now();
        seq_time_ms = std::chrono::duration<double, std::milli>(end_seq - start_seq).count();
    }

    MPI_Barrier(MPI_COMM_WORLD);
    auto start_par = std::chrono::high_resolution_clock::now();
    auto par_result = process_parallel(all_docs);
    MPI_Barrier(MPI_COMM_WORLD);
    auto end_par = std::chrono::high_resolution_clock::now();
    double par_time_ms = std::chrono::duration<double, std::milli>(end_par - start_par).count();

    double max_par_time = 0;
    MPI_Reduce(&par_time_ms, &max_par_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank_ == 0 && !all_docs.empty()) {
        double speedup = seq_time_ms / max_par_time;
        double efficiency = speedup / world_size_;

        JsonWriter jw;
        jw.start_object();
        jw.kv_string("command", "benchmark"); jw.comma();
        jw.kv_int("num_processes", world_size_); jw.comma();
        jw.kv_int("num_documents", par_result.total_documents); jw.comma();
        jw.kv_int("total_words", par_result.total_words); jw.comma();
        jw.kv_double("time_sequential_ms", seq_time_ms, 2); jw.comma();
        jw.kv_double("time_parallel_ms", max_par_time, 2); jw.comma();
        jw.kv_double("speedup", speedup, 4); jw.comma();
        jw.kv_double("efficiency", efficiency, 4); jw.comma();
        jw.kv_double("overhead_ms", max_par_time * world_size_ - seq_time_ms, 2);
        jw.end_object();
        jw.write_to_file(output_path);

        std::cout << "{\"status\":\"success\",\"command\":\"benchmark\","
                  << "\"time_sequential_ms\":" << std::fixed << std::setprecision(2) << seq_time_ms << ","
                  << "\"time_parallel_ms\":" << std::fixed << std::setprecision(2) << max_par_time << ","
                  << "\"speedup\":" << std::fixed << std::setprecision(4) << speedup << ","
                  << "\"efficiency\":" << std::fixed << std::setprecision(4) << efficiency << ","
                  << "\"num_processes\":" << world_size_
                  << "}" << std::endl;
    }
}

} // namespace paralex
