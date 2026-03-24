#ifndef ARALOG_LOGGER_H
#define ARALOG_LOGGER_H

#include "common.h"
#include "log_stream.h"
#include "../core/instance_specifier.h"

namespace ara::log {

/// \brief Connection state handler type
using ConnectionStateHandler = void(*)(ClientState state);

/// \brief Main logger class
class Logger final {
public:
    Logger() noexcept = default;
    ~Logger() noexcept;

    // Disable copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Allow moving
    Logger(Logger&& other) noexcept;
    Logger& operator=(Logger&& other) noexcept;

    /// \brief Check if logging is enabled for given level
    bool IsEnabled(LogLevel level) const noexcept;

    /// \brief Convenience methods for each log level
    LogStream LogFatal() noexcept;
    LogStream LogError() noexcept;
    LogStream LogWarn() noexcept;
    LogStream LogInfo() noexcept;
    LogStream LogDebug() noexcept;
    LogStream LogVerbose() noexcept;

    /// \brief Create a log stream with specific level
    LogStream WithLevel(LogLevel level) const noexcept;

    /// \brief Create a logger with default log level
    static Logger CreateLogger(std::string ctxId, std::string ctxDescription) noexcept;

    /// \brief Create a logger with specific log level
    static Logger CreateLogger(std::string ctxId, std::string ctxDescription, LogLevel level) noexcept;

    /// \brief Generic logging with explicit level
    void Log(LogLevel level, ara::core::StringView fmt, ...) noexcept;

    /// \brief Set log level threshold
    void SetThreshold(LogLevel level) noexcept;

    /// \brief Register connection state handler
    static void RegisterConnectionStateHandler(ConnectionStateHandler handler) noexcept;

private:
    friend class LogStream;

    explicit Logger(ara::core::InstanceSpecifier spec, LogLevel default_level) noexcept;

    LogStream CreateStream(LogLevel level) const noexcept;

    ara::core::InstanceSpecifier m_spec;
    LogLevel m_threshold = LogLevel::kInfo;
    bool m_initialized = false;
    static ConnectionStateHandler g_connection_handler;
};

} // namespace ara::log

#endif // ARALOG_LOGGER_H
