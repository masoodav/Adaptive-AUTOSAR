#include "./logger.h"
#include <iostream>
#include <mutex>
#include <map>
#include <memory>
#include <atomic>
#include <iomanip>

namespace ara {
namespace log {

// FIX: Removed anonymous namespace to match 'friend class LoggerManager' in header.
// LoggerManager is defined only in this translation unit, effectively hiding it from others
// unless they extern it (which they shouldn't).

class LoggerManager {
public:
    static LoggerManager& Get() {
        return instance_;
    }

    Logger& GetOrCreate(const std::string& id, const std::string& desc, LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = loggers_.find(id);
        if (it != loggers_.end()) 
        {
            return *(it->second);
        }
        // MISRA 21-6-2: make_unique preferred, but constructor is private/friend. 
        // Direct new wrapped immediately in unique_ptr is acceptable here given friendship constraints.
        std::unique_ptr<Logger> logger(new Logger(id, desc, level));
        Logger& ref = *logger;
        // MISRA 0-1-2: Ignored return value of emplace
        static_cast<void>(loggers_.emplace(id, std::move(logger)));
        return ref;
    }

private:
    static LoggerManager instance_;
    std::mutex mutex_{};
    std::map<std::string, std::unique_ptr<Logger>> loggers_{};
};

LoggerManager LoggerManager::instance_;

// --- Logger ---
Logger::Logger(const std::string& ctxId, const std::string& ctxDesc, LogLevel level)
    : contextId_(ctxId), contextDescription_(ctxDesc), currentLimit_(level), logHandler_(nullptr) {}

Logger::Logger(Logger&& other) noexcept
    : contextId_(std::move(other.contextId_)),
      contextDescription_(std::move(other.contextDescription_)),
      currentLimit_(other.currentLimit_.load()),
      logHandler_(std::move(other.logHandler_)) {}

Logger::~Logger() {}

void Logger::SetLogHandler(LogHandler handler) { 
    logHandler_ = handler; 
}

bool Logger::IsEnabled(LogLevel logLevel) const noexcept {
    return static_cast<int>(logLevel) <= static_cast<int>(currentLimit_.load());
}

void Logger::SetThreshold(LogLevel threshold) noexcept { 
    currentLimit_.store(threshold); 
}

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
    try 
    { 
        return LoggerManager::Get().GetOrCreate(ctxId.data(), ctxDescription.data(), ctxDefLogLevel); 
    } 
    catch (...) 
    { 
        // MISRA 18-5-2: Terminate is used here because failure to create a logger 
        // in an AUTOSAR context usually implies critical system instability (OOM).
        std::terminate(); 
    }
}

Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept {
    return CreateLogger(core::StringView(is.ToString().c_str()), "From InstanceSpecifier", LogLevel::kWarn);
}

void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept {
    static_cast<void>(callback); 
}

// --- LogStream Implementation ---

LogStream::LogStream(LogLevel level, const std::string& ctxId, bool active, LogHandler handler) noexcept 
    : level_(level), ctxId_(ctxId), active_(active), first_arg_(true), logHandler_(handler) {}

LogStream::LogStream() noexcept 
    : level_(LogLevel::kOff), ctxId_("INTERNAL"), active_(true), first_arg_(true), logHandler_(nullptr) {}

LogStream::LogStream(LogStream&& other) noexcept 
    : level_(other.level_), ctxId_(std::move(other.ctxId_)), 
      active_(other.active_), first_arg_(other.first_arg_),
      logHandler_(std::move(other.logHandler_)) {
    static_cast<void>(buffer_ << other.buffer_.str());
    other.buffer_.str("");
    other.active_ = false;
}

LogStream::~LogStream() noexcept {
    // MISRA 18-4-1: Destructor must not throw.
    try {
        if (active_ && level_ != LogLevel::kOff) 
        {
            Flush();
        }
    } catch(...) {
        // Swallow exception in destructor
    }
}

void LogStream::Flush() noexcept {
    try {
        if (!active_) 
        {
            return;
        }
        std::string msg = buffer_.str();
        if (msg.empty()) 
        {
            return;
        }

        if (logHandler_) {
            logHandler_(level_, msg);
        } else {
            // MISRA 0-1-2: Cast result to void
            static_cast<void>(std::cout << "[" << ctxId_ << "] " << msg << std::endl);
        }

        buffer_.str("");
        buffer_.clear();
        first_arg_ = true;
    } catch (...) {}
}

std::string LogStream::ToString() const noexcept {
    try 
    { 
        return buffer_.str(); 
    } 
    catch (...) 
    { 
        return ""; 
    }
}

LogStream& LogStream::operator<<(const LogStream& other) noexcept {
    try 
    { 
        if (active_) 
        { 
            AddSeparator(); 
            static_cast<void>(buffer_ << other.ToString()); 
        } 
    } 
    catch (...) {}
    return *this;
}

void LogStream::AddSeparator() { 
    if (!first_arg_) 
    {
        static_cast<void>(buffer_ << " "); 
    }
    first_arg_ = false; 
}

LogStream& LogStream::WithLocation(core::StringView file, int line) noexcept {
    try 
    { 
        if(active_) 
        {
            static_cast<void>(buffer_ << "@" << file.data() << ":" << line << " "); 
        }
    } 
    catch (...) {}
    return *this;
}

LogStream& LogStream::WithTag(core::StringView tag) noexcept {
    try 
    { 
        if(active_) 
        {
            static_cast<void>(buffer_ << "[" << tag.data() << "] "); 
        }
    } 
    catch (...) {}
    return *this;
}

LogStream& LogStream::operator<<(const core::StringView value) noexcept {
    try { if (active_) { AddSeparator(); static_cast<void>(buffer_ << value.data()); } } catch(...) {}
    return *this;
}

LogStream& LogStream::operator<<(const char* const value) noexcept {
    try { if (active_) { AddSeparator(); static_cast<void>(buffer_ << value); } } catch(...) {}
    return *this;
}

LogStream& LogStream::operator<<(const void* value) noexcept {
    try { if (active_) { AddSeparator(); static_cast<void>(buffer_ << value); } } catch(...) {}
    return *this;
}

LogStream& LogStream::operator<<(const std::string& value) noexcept {
    try { if (active_) { AddSeparator(); static_cast<void>(buffer_ << value); } } catch(...) {}
    return *this;
}

LogStream& LogStream::operator<<(LogLevel value) noexcept {
    try { if (active_) { AddSeparator(); static_cast<void>(buffer_ << static_cast<int>(value)); } } catch(...) {}
    return *this;
}

LogStream& LogStream::operator<<(const core::ErrorCode& ec) noexcept {
    try { if (active_) { AddSeparator(); static_cast<void>(buffer_ << "Error:" << ec.Value()); } } catch(...) {}
    return *this;
}

LogStream& LogStream::operator<<(const core::InstanceSpecifier& value) noexcept {
    try { if (active_) { AddSeparator(); static_cast<void>(buffer_ << value.ToString()); } } catch(...) {}
    return *this;
}

LogStream& LogStream::operator<<(core::Span<const core::Byte> data) noexcept {
    try {
        if (active_) {
            AddSeparator();
            auto old_flags = buffer_.flags();
            static_cast<void>(buffer_ << std::hex << std::uppercase);
            
            const size_t size = data.size();
            for (size_t i = 0; i < size; ++i) {
                if (i > 0) 
                {
                    static_cast<void>(buffer_ << "'");
                }
                static_cast<void>(buffer_ << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]));
            }
            buffer_.flags(old_flags);
        }
    } catch(...) {}
    return *this;
}

} // namespace log
} // namespace ara