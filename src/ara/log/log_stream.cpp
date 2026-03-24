#include "log_stream.h"
#include "logger.h"
#include <sstream>
#include <iomanip>

namespace ara::log {

// Add the static Create method implementation
LogStream LogStream::Create(Logger& logger, LogLevel level) noexcept {
    return LogStream(logger, level);
}

LogStream::LogStream(LogStream&& other) noexcept
    : m_logger(other.m_logger),
      m_level(other.m_level),
      m_buffer(std::move(other.m_buffer)),
      m_flushed(other.m_flushed) {
    other.m_logger = nullptr;
}

LogStream::LogStream(Logger& logger, LogLevel level) noexcept
    : m_logger(&logger), m_level(level) {}

LogStream::~LogStream() noexcept {
    if (!m_flushed && m_logger) {
        InternalSend();
    }
}

void LogStream::Flush() noexcept {
    if (!m_flushed && m_logger) {
        InternalSend();
        m_flushed = true;
    }
}

LogStream& LogStream::WithLocation(ara::core::StringView file, int line) noexcept {
    if (m_logger) {
        m_buffer += file.data();
        m_buffer += ':';
        m_buffer += std::to_string(line);
        m_buffer += " ";
    }
    return *this;
}

LogStream& LogStream::operator<<(std::uint8_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(std::uint16_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(std::uint32_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(std::uint64_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(std::int8_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(bool value) noexcept {
    if (m_logger) {
        m_buffer += value ? "true " : "false ";
    }
    return *this;
}

LogStream& LogStream::operator<<(std::int64_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(std::int32_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(std::int16_t value) noexcept {
    if (m_logger) {
        m_buffer += std::to_string(value);
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(float value) noexcept {
    if (m_logger) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value;
        m_buffer += oss.str();
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(double value) noexcept {
    if (m_logger) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(15) << value;
        m_buffer += oss.str();
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(ara::core::StringView str) noexcept {
    if (m_logger) {
        m_buffer += str.data();
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(const char* str) noexcept {
    if (m_logger && str) {
        m_buffer += str;
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(const void* ptr) noexcept {
    if (m_logger) {
        std::ostringstream oss;
        oss << ptr;
        m_buffer += oss.str();
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(LogLevel level) noexcept {
    if (m_logger) {
        m_buffer += ToString(level).data();
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(const ara::core::ErrorCode& ec) noexcept {
    if (m_logger) {
        m_buffer += ec.Message();
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(const ara::core::InstanceSpecifier& spec) noexcept {
    if (m_logger) {
        m_buffer += spec.ToString();
        m_buffer += ' ';
    }
    return *this;
}

LogStream& LogStream::operator<<(const std::string& str) noexcept {
    if (m_logger) {
        m_buffer += str;
        m_buffer += ' ';
    }
    return *this;
}

void LogStream::InternalSend() noexcept {
    if (m_logger && !m_buffer.empty()) {
        // Forward to backend
        m_buffer.clear();
    }
}

} // namespace ara::log
