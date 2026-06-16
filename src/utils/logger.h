/**
 * @file logger.h
 * @brief Sistema de logging con soporte multi-rank MPI
 * 
 * Proporciona logging thread-safe con niveles INFO, WARN, ERROR, FATAL.
 * Cada proceso MPI registra su rank en los mensajes.
 */

#ifndef PARALEX_LOGGER_H
#define PARALEX_LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <vector>
#include <tuple>

namespace paralex {

enum class LogLevel { INFO, WARN, ERROR, FATAL };

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void init(const std::string& log_dir, int mpi_rank = 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        rank_ = mpi_rank;
        std::string filename = log_dir + "/paralex_rank_" + std::to_string(rank_) + ".log";
        file_.open(filename, std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[Logger] Could not open log file: " << filename << std::endl;
        }
    }

    void log(LogLevel level, const std::string& source, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string timestamp = get_timestamp();
        std::string level_str = level_to_string(level);
        
        std::ostringstream oss;
        oss << "[" << timestamp << "] "
            << "[" << level_str << "] "
            << "[Rank " << rank_ << "] "
            << "[" << source << "] "
            << message;
        
        std::string line = oss.str();
        
        if (file_.is_open()) {
            file_ << line << std::endl;
            file_.flush();
        }
        
        if (level == LogLevel::ERROR || level == LogLevel::FATAL) {
            std::cerr << line << std::endl;
        }

        log_entries_.push_back({level_str, source, message, timestamp});
    }

    void info(const std::string& source, const std::string& msg) {
        log(LogLevel::INFO, source, msg);
    }

    void warn(const std::string& source, const std::string& msg) {
        log(LogLevel::WARN, source, msg);
    }

    void error(const std::string& source, const std::string& msg) {
        log(LogLevel::ERROR, source, msg);
    }

    void fatal(const std::string& source, const std::string& msg) {
        log(LogLevel::FATAL, source, msg);
    }

    struct LogEntry {
        std::string level;
        std::string source;
        std::string message;
        std::string timestamp;
    };

    const std::vector<LogEntry>& get_entries() const { return log_entries_; }
    void clear_entries() { log_entries_.clear(); }

    int get_rank() const { return rank_; }

    ~Logger() {
        if (file_.is_open()) file_.close();
    }

private:
    Logger() : rank_(0) {}
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    int rank_;
    std::ofstream file_;
    std::mutex mutex_;
    std::vector<LogEntry> log_entries_;
};

#define LOG_INFO(src, msg)  paralex::Logger::instance().info(src, msg)
#define LOG_WARN(src, msg)  paralex::Logger::instance().warn(src, msg)
#define LOG_ERROR(src, msg) paralex::Logger::instance().error(src, msg)
#define LOG_FATAL(src, msg) paralex::Logger::instance().fatal(src, msg)

} // namespace paralex

#endif // PARALEX_LOGGER_H
