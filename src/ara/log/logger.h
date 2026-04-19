#ifndef LOGGER_H
#define LOGGER_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <algorithm>

namespace ara::log {

enum class LogLevel { Fatal, Error, Warn, Info, Debug, Verbose };

class LoggerRegistry {
public:
    static LoggerRegistry& Instance() {
        static LoggerRegistry instance;
        return instance;
    }

    void AddLogger(class Logger* logger) {
        std::lock_guard<std::mutex> lock(mutex_);
        loggers_.push_back(logger);
    }

    void RemoveLogger(class Logger* logger) {
        std::lock_guard<std::mutex> lock(mutex_);
        loggers_.erase(
            std::remove(loggers_.begin(), loggers_.end(), logger),
            loggers_.end()
        );
    }

private:
    std::vector<Logger*> loggers_;
    std::mutex mutex_;
    LoggerRegistry() = default;
    ~LoggerRegistry() = default;
    LoggerRegistry(const LoggerRegistry&) = delete;
    LoggerRegistry& operator=(const LoggerRegistry&) = delete;
};

class Logger {
public:
    explicit Logger(const std::string& appId, const std::string& appDescription)
        : appId_(appId), appDescription_(appDescription) {}

    ~Logger() {
        LoggerRegistry::Instance().RemoveLogger(this);
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    Logger(Logger&&) = default;
    Logger& operator=(Logger&& other) noexcept {
        if (this != &other) {
            appId_ = std::move(other.appId_);
            appDescription_ = std::move(other.appDescription_);
        }
        return *this;
    }

    template <typename... Args>
    bool Log(LogLevel level, const std::string& fmt, Args&&... args) {
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        // Log the message (implementation depends on your logging backend)
        return true;
    }

private:
    std::string appId_;
    std::string appDescription_;
};

} // namespace ara::log

#endif // LOGGER_H
