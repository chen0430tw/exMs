#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <sstream>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace exms::runtime::log {

// Log levels
enum class LogLevel : uint32_t {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
};

// Log output targets
enum class LogTarget : uint32_t {
    CONSOLE = 0x01,
    FILE    = 0x02,
    BOTH    = 0x03
};

// Global configuration
struct LogConfig {
    LogLevel min_level = LogLevel::INFO;
    LogTarget target = LogTarget::CONSOLE;
    std::string log_file = "exms.log";
    bool timestamp = true;
    bool thread_id = false;
    bool source_location = true;
    bool color_output = true;
};

// Global log state (singleton)
class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void set_config(const LogConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;

        if ((config_.target & LogTarget::FILE) != 0 && !config_.log_file.empty()) {
            file_stream_.open(config_.log_file, std::ios::app);
        }
    }

    const LogConfig& config() const { return config_; }

    void log(LogLevel level, const std::string& message,
             const char* file = nullptr, int line = 0) {
        if (level < config_.min_level) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        std::string output = format_message(level, message, file, line);

        if ((config_.target & LogTarget::CONSOLE) != 0) {
            if (config_.color_output) {
                std::cout << color_for_level(level);
            }
            std::cout << output;
            if (config_.color_output) {
                std::cout << "\033[0m";
            }
            std::cout << std::endl;
        }

        if ((config_.target & LogTarget::FILE) != 0 && file_stream_.is_open()) {
            file_stream_ << output << std::endl;
            file_stream_.flush();
        }
    }

private:
    Logger() = default;
    ~Logger() {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
    }

    LogConfig config_;
    std::mutex mutex_;
    std::ofstream file_stream_;

    std::string format_message(LogLevel level, const std::string& message,
                                const char* file, int line) {
        std::ostringstream oss;

        if (config_.timestamp) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            oss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
            oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "]";
        }

        oss << "[" << level_name(level) << "]";

        if (config_.source_location && file) {
            const char* filename = file;
            // Extract just filename from path
            const char* slash = strrchr(file, '/');
            const char* backslash = strrchr(file, '\\');
            if (backslash && (!slash || backslash > slash)) {
                slash = backslash;
            }
            if (slash) {
                filename = slash + 1;
            }
            oss << "[" << filename << ":" << line << "]";
        }

        oss << " " << message;
        return oss.str();
    }

    const char* level_name(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "?????";
        }
    }

    const char* color_for_level(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "\033[90m";    // Gray
            case LogLevel::DEBUG: return "\033[36m";    // Cyan
            case LogLevel::INFO:  return "\033[32m";    // Green
            case LogLevel::WARN:  return "\033[33m";    // Yellow
            case LogLevel::ERROR: return "\033[31m";    // Red
            case LogLevel::FATAL: return "\033[35m";    // Magenta
            default: return "\033[0m";
        }
    }
};

// Convenience macros
#define LOG_TRACE(msg) \
    exms::runtime::log::Logger::instance().log(exms::runtime::log::LogLevel::TRACE, msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg) \
    exms::runtime::log::Logger::instance().log(exms::runtime::log::LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg) \
    exms::runtime::log::Logger::instance().log(exms::runtime::log::LogLevel::INFO, msg, __FILE__, __LINE__)
#define LOG_WARN(msg) \
    exms::runtime::log::Logger::instance().log(exms::runtime::log::LogLevel::WARN, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) \
    exms::runtime::log::Logger::instance().log(exms::runtime::log::LogLevel::ERROR, msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) \
    exms::runtime::log::Logger::instance().log(exms::runtime::log::LogLevel::FATAL, msg, __FILE__, __LINE__)

// Formatted logging helpers
template<typename... Args>
std::string format(const char* fmt, Args... args) {
    // Simple formatting (sprintf-like)
    // For production, use fmtlib or std::format (C++20)
    size_t size = snprintf(nullptr, 0, fmt, args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, fmt, args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

// Module-specific loggers (can be configured per-module)
class ModuleLogger {
public:
    explicit ModuleLogger(const std::string& module_name)
        : module_name_(module_name) {}

    template<typename... Args>
    void trace(const char* fmt, Args... args) {
        log(LogLevel::TRACE, fmt, args...);
    }

    template<typename... Args>
    void debug(const char* fmt, Args... args) {
        log(LogLevel::DEBUG, fmt, args...);
    }

    template<typename... Args>
    void info(const char* fmt, Args... args) {
        log(LogLevel::INFO, fmt, args...);
    }

    template<typename... Args>
    void warn(const char* fmt, Args... args) {
        log(LogLevel::WARN, fmt, args...);
    }

    template<typename... Args>
    void error(const char* fmt, Args... args) {
        log(LogLevel::ERROR, fmt, args...);
    }

    template<typename... Args>
    void fatal(const char* fmt, Args... args) {
        log(LogLevel::FATAL, fmt, args...);
    }

private:
    std::string module_name_;

    template<typename... Args>
    void log(LogLevel level, const char* fmt, Args... args) {
        std::string msg = "[" + module_name_ + "] " + format(fmt, args...);
        Logger::instance().log(level, msg);
    }
};

} // namespace exms::runtime::log

// Debug mode support
#ifdef EXMS_DEBUG
    #define DLOG_TRACE(msg) LOG_TRACE(msg)
    #define DLOG_DEBUG(msg) LOG_DEBUG(msg)
    #define DLOG_INFO(msg) LOG_INFO(msg)
    #define DLOG_WARN(msg) LOG_WARN(msg)
    #define DLOG_ERROR(msg) LOG_ERROR(msg)
#else
    #define DLOG_TRACE(msg) ((void)0)
    #define DLOG_DEBUG(msg) ((void)0)
    #define DLOG_INFO(msg) ((void)0)
    #define DLOG_WARN(msg) ((void)0)
    #define DLOG_ERROR(msg) ((void)0)
#endif
