#include "./logger.h"
#include <iostream>
#include <mutex>
#include <map>
#include <memory>
#include <atomic>
#include <iomanip>

namespace ara {
namespace log {

// ... [LoggerManager, Logger, Global Functions omitted - assume same as before] ...

// --- Logger Manager ---
class LoggerManager {
public:
    static LoggerManager& Get() {
        static LoggerManager instance;
        return instance;
    }
    Logger& GetOrCreate(const std::string& id, const std::string& desc, LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = loggers_.find(id);
        if (it != loggers_.end()) return *(it->second);
        auto logger = new Logger(id, desc, level); 
        loggers_[id] = std::unique_ptr<Logger>(logger);
        return *logger;
    }
private:
    std::mutex mutex_;
    std::map<std::string, std::unique_ptr<Logger>> loggers_;
};

// --- Logger ---
Logger::Logger(const std::string& ctxId, const std::string& ctxDesc, LogLevel level)
    : contextId_(ctxId), contextDescription_(ctxDesc), currentLimit_(level) {}

Logger::Logger(Logger&& other) noexcept
    : contextId_(std::move(other.contextId_)),
      contextDescription_(std::move(other.contextDescription_)),
      currentLimit_(other.currentLimit_.load()),
      logHandler_(std::move(other.logHandler_)) {}

Logger::~Logger() {}

void Logger::SetLogHandler(LogHandler handler) { logHandler_ = handler; }

bool Logger::IsEnabled(LogLevel logLevel) const noexcept {
    return static_cast<int>(logLevel) <= static_cast<int>(currentLimit_.load());
}

void Logger::SetThreshold(LogLevel threshold) noexcept { currentLimit_.store(threshold); }

LogStream Logger::WithLevel(LogLevel logLevel) const noexcept {
    bool active = IsEnabled(logLevel);
    return LogStream(logLevel, contextId_, active, logHandler_);
}

LogStream Logger::LogFatal() const noexcept   { return WithLevel(LogLevel::kFatal); }
LogStream Logger::LogError() const noexcept   { return WithLevel(LogLevel::kError); }
LogStream Logger::LogWarn() const noexcept    { return WithLevel(LogLevel::kWarn); }
LogStream Logger::LogInfo() const noexcept    { return WithLevel(LogLevel::kInfo); }
LogStream Logger::LogDebug() const noexcept   { return WithLevel(LogLevel::kDebug); }
LogStream Logger::LogVerbose() const noexcept { return WithLevel(LogLevel::kVerbose); }

Logger& CreateLogger(core::StringView ctxId, core::StringView ctxDescription, LogLevel ctxDefLogLevel) noexcept {
    try { return LoggerManager::Get().GetOrCreate(ctxId.data(), ctxDescription.data(), ctxDefLogLevel); } catch (...) { std::terminate(); }
}
Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept {
    return CreateLogger(core::StringView(is.ToString().c_str()), "From InstanceSpecifier", LogLevel::kWarn);
}
void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept {}

// --- LogStream Implementation ---

LogStream::LogStream(LogLevel level, const std::string& ctxId, bool active, LogHandler handler) noexcept 
    : level_(level), ctxId_(ctxId), active_(active), first_arg_(true), logHandler_(handler) {}

LogStream::LogStream() noexcept 
    : level_(LogLevel::kOff), ctxId_("INTERNAL"), active_(true), first_arg_(true) {}

LogStream::LogStream(LogStream&& other) noexcept 
    : level_(other.level_), ctxId_(std::move(other.ctxId_)), 
      active_(other.active_), first_arg_(other.first_arg_),
      logHandler_(std::move(other.logHandler_)) {
    buffer_ << other.buffer_.str();
    other.buffer_.str("");
    other.active_ = false;
}

LogStream::~LogStream() noexcept {
    if (active_ && level_ != LogLevel::kOff) Flush();
}

void LogStream::Flush() noexcept {
    try {
        if (!active_) return;
        std::string msg = buffer_.str();
        if (msg.empty()) return;

        if (logHandler_) {
            logHandler_(level_, msg);
        } else {
            std::cout << "[" << ctxId_ << "] " << msg << std::endl;
        }

        buffer_.str("");
        buffer_.clear();
        first_arg_ = true;
    } catch (...) {}
}

std::string LogStream::ToString() const noexcept {
    try { return buffer_.str(); } catch (...) { return ""; }
}

LogStream& LogStream::operator<<(const LogStream& other) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << other.ToString(); } } catch (...) {}
    return *this;
}

void LogStream::AddSeparator() { if (!first_arg_) buffer_ << " "; first_arg_ = false; }

// Correctly matched implementation
LogStream& LogStream::WithLocation(core::StringView file, int line) noexcept {
    try { if(active_) buffer_ << "@" << file.data() << ":" << line << " "; } catch (...) {}
    return *this;
}

// Correctly matched implementation
LogStream& LogStream::WithTag(core::StringView tag) noexcept {
    try { if(active_) buffer_ << "[" << tag.data() << "] "; } catch (...) {}
    return *this;
}

#define IMPL_LOG_OP(Type) \
    LogStream& LogStream::operator<<(Type value) noexcept { \
        try { if (active_) { AddSeparator(); buffer_ << value; } } catch (...) {} \
        return *this; \
    }

IMPL_LOG_OP(std::uint16_t)
IMPL_LOG_OP(std::uint32_t)
IMPL_LOG_OP(std::uint64_t)
IMPL_LOG_OP(std::int8_t)
IMPL_LOG_OP(std::int16_t)
IMPL_LOG_OP(std::int32_t)
IMPL_LOG_OP(std::int64_t)
IMPL_LOG_OP(float)
IMPL_LOG_OP(double)
IMPL_LOG_OP(const char* const)
IMPL_LOG_OP(const void*)

LogStream& LogStream::operator<<(std::uint8_t value) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << static_cast<int>(value); } } catch(...) {}
    return *this;
}
LogStream& LogStream::operator<<(bool value) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << (value ? "1" : "0"); } } catch(...) {}
    return *this;
}
LogStream& LogStream::operator<<(const core::StringView value) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << value.data(); } } catch(...) {}
    return *this;
}
LogStream& LogStream::operator<<(const std::string& value) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << value; } } catch(...) {}
    return *this;
}
LogStream& LogStream::operator<<(LogLevel value) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << static_cast<int>(value); } } catch(...) {}
    return *this;
}
LogStream& LogStream::operator<<(const core::ErrorCode& ec) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << "Error:" << ec.Value(); } } catch(...) {}
    return *this;
}
LogStream& LogStream::operator<<(const core::InstanceSpecifier& value) noexcept {
    try { if (active_) { AddSeparator(); buffer_ << value.ToString(); } } catch(...) {}
    return *this;
}
LogStream& LogStream::operator<<(core::Span<const core::Byte> data) noexcept {
    try {
        if (active_) {
            AddSeparator();
            auto old_flags = buffer_.flags();
            buffer_ << std::hex << std::uppercase;
            for (size_t i = 0; i < data.size(); ++i) {
                if (i > 0) buffer_ << "'";
                buffer_ << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
            }
            buffer_.flags(old_flags);
        }
    } catch(...) {}
    return *this;
}

} // namespace log
} // namespace ara