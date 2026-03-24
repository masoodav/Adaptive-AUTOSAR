#include "logger.h"
#include <unordered_map>
#include <mutex>

namespace ara::log {

// Define the static member variable
// Logger::ConnectionStateHandler Logger::g_connection_handler = nullptr;

namespace {
std::unordered_map<std::string, Logger> g_loggers;
std::mutex g_logger_mutex;
} // namespace

Logger::Logger(Logger&& other) noexcept
    : m_spec(std::move(other.m_spec)), m_threshold(other.m_threshold),
      m_initialized(other.m_initialized) {
    other.m_initialized = false;
}

Logger& Logger::operator=(Logger&& other) noexcept {
    if (this != &other) {
        m_spec = std::move(other.m_spec);
        m_threshold = other.m_threshold;
        m_initialized = other.m_initialized;
        other.m_initialized = false;
    }
    return *this;
}

Logger::~Logger() noexcept {
    // Cleanup if needed
}

Logger::Logger(ara::core::InstanceSpecifier spec, LogLevel default_level) noexcept
    : m_spec(std::move(spec)), m_threshold(default_level), m_initialized(true) {}

bool Logger::IsEnabled(LogLevel level) const noexcept {
    return static_cast<std::uint8_t>(level) <= static_cast<std::uint8_t>(m_threshold);
}

LogStream Logger::LogFatal() noexcept { return CreateStream(LogLevel::kFatal); }
LogStream Logger::LogError() noexcept { return CreateStream(LogLevel::kError); }
LogStream Logger::LogWarn() noexcept { return CreateStream(LogLevel::kWarn); }
LogStream Logger::LogInfo() noexcept { return CreateStream(LogLevel::kInfo); }
LogStream Logger::LogDebug() noexcept { return CreateStream(LogLevel::kDebug); }
LogStream Logger::LogVerbose() noexcept { return CreateStream(LogLevel::kVerbose); }

LogStream Logger::WithLevel(LogLevel level) const noexcept {
    return CreateStream(level);
}

Logger Logger::CreateLogger(std::string ctxId, std::string ctxDescription) noexcept {
    return CreateLogger(ctxId, ctxDescription, LogLevel::kInfo);
}

Logger Logger::CreateLogger(std::string ctxId, std::string ctxDescription, LogLevel level) noexcept {
    ara::core::InstanceSpecifier spec{ctxId + "/" + ctxDescription};
    return Logger(spec, level);
}

void Logger::Log(LogLevel level, ara::core::StringView fmt, ...) noexcept {
    if (!IsEnabled(level)) return;
    // Forward to backend
}

void Logger::SetThreshold(LogLevel level) noexcept { m_threshold = level; }

void Logger::RegisterConnectionStateHandler(ConnectionStateHandler handler) noexcept {
    // std::lock_guard<std::mutex> lock(g_logger_mutex);
    // g_connection_handler = handler;
}

LogStream Logger::CreateStream(LogLevel level) const noexcept {
    if (!m_initialized || !IsEnabled(level)) {
        return LogStream(const_cast<Logger&>(*this), LogLevel::kOff);
    }
    return LogStream(const_cast<Logger&>(*this), level);
}

} // namespace ara::log
