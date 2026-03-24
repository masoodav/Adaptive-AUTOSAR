#ifndef ARALOG_LOG_STREAM_H
#define ARALOG_LOG_STREAM_H

#include "common.h"
#include "../core/instance_specifier.h"
#include <chrono>
#include <cstdint>
#include <string>

namespace ara::log {
class Logger; // Forward declaration

// Forward declare sink classes
namespace sink {
    class ConsoleLogSink;
    class FileLogSink;
}

/// \brief Log message stream builder
class LogStream final {
public:
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    LogStream& operator=(LogStream&&) = delete;

    LogStream(LogStream&& other) noexcept;
    ~LogStream() noexcept;

    /// \brief Send the current message buffer
    void Flush() noexcept;

    /// \brief Add source location to message
    LogStream& WithLocation(ara::core::StringView file, int line) noexcept;

    /// \brief Get the message buffer as a string
    std::string GetBuffer() const noexcept { return m_buffer; }

    // Stream operators for various types
    LogStream& operator<<(std::uint8_t value) noexcept;
    LogStream& operator<<(std::uint16_t value) noexcept;
    LogStream& operator<<(std::uint32_t value) noexcept;
    LogStream& operator<<(std::uint64_t value) noexcept;
    LogStream& operator<<(std::int8_t value) noexcept;
    LogStream& operator<<(bool value) noexcept;
    LogStream& operator<<(std::int64_t value) noexcept;
    LogStream& operator<<(std::int32_t value) noexcept;
    LogStream& operator<<(std::int16_t value) noexcept;
    LogStream& operator<<(float value) noexcept;
    LogStream& operator<<(double value) noexcept;
    LogStream& operator<<(ara::core::StringView str) noexcept;
    LogStream& operator<<(const char* str) noexcept;
    LogStream& operator<<(const void* ptr) noexcept;
    LogStream& operator<<(LogLevel level) noexcept;
    LogStream& operator<<(const ara::core::ErrorCode& ec) noexcept;
    LogStream& operator<<(const ara::core::InstanceSpecifier& spec) noexcept;
    LogStream& operator<<(const std::string& str) noexcept;

    /// \brief Create a new LogStream with a specific logger and level
    static LogStream Create(Logger& logger, LogLevel level) noexcept;

private:
    friend class Logger;
    friend class sink::ConsoleLogSink;
    friend class sink::FileLogSink;

    explicit LogStream(Logger& logger, LogLevel level) noexcept;
    void InternalSend() noexcept;

    Logger* m_logger;
    LogLevel m_level;
    std::string m_buffer;
    bool m_flushed = false;
};

} // namespace ara::log

#endif // ARALOG_LOG_STREAM_H
