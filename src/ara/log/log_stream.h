#ifndef ARA_LOG_LOG_STREAM_H
#define ARA_LOG_LOG_STREAM_H

#include <chrono>
#include <sstream>
#include <string>
#include <functional>
#include "../core/core_types.h"
#include "./common.h"
#include "./log_fwd.h"

namespace ara {
namespace log {

using LogHandler = std::function<void(LogLevel, const std::string&)>;

namespace sink { class LogSink; }

class LogStream final {
public:
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    LogStream(LogStream&& other) noexcept;
    LogStream& operator=(LogStream&&) = delete;
    ~LogStream() noexcept;

    LogStream() noexcept;

    void Flush() noexcept;
    
    LogStream& WithLocation(core::StringView file, int line) noexcept;
    LogStream& WithTag(core::StringView tag) noexcept;

    template <typename T>
    LogStream& WithPrivacy(T value) noexcept { 
        static_cast<void>(value); 
        return *this; 
    }

    std::string ToString() const noexcept;

    template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    LogStream& operator<<(T value) noexcept {
        try {
            if (active_) {
                AddSeparator();
                static_cast<void>(buffer_ << (+value));
            }
        } catch (...) {}
        return *this;
    }

    LogStream& operator<<(const core::StringView value) noexcept;
    LogStream& operator<<(const char* const value) noexcept;
    LogStream& operator<<(const void* value) noexcept; 
    LogStream& operator<<(LogLevel value) noexcept; 
    LogStream& operator<<(const core::ErrorCode& ec) noexcept; 
    LogStream& operator<<(const core::InstanceSpecifier& value) noexcept;
    LogStream& operator<<(core::Span<const core::Byte> data) noexcept;
    LogStream& operator<<(const std::string& value) noexcept;
    LogStream& operator<<(const LogStream& other) noexcept;

    template <typename Rep, typename Period>
    LogStream& operator<<(const std::chrono::duration<Rep, Period>& value) noexcept {
        try {
            if (!active_) return *this;
            AddSeparator();
            static_cast<void>(buffer_ << value.count() << "ticks");
        } catch (...) {}
        return *this;
    }

    template <typename T>
    LogStream& operator<<(const Argument<T>& arg) noexcept {
        try {
            if (!active_) return *this;
            AddSeparator();
            if (arg.name_) static_cast<void>(buffer_ << arg.name_ << ":");
            static_cast<void>(buffer_ << arg.value);
            if (arg.unit_) static_cast<void>(buffer_ << ":" << arg.unit_);
        } catch (...) {}
        return *this;
    }

private:
    friend class Logger;
    friend class sink::LogSink;

    LogStream(LogLevel level, const std::string& ctxId, bool active, LogHandler handler) noexcept;
    
    void AddSeparator();

    std::ostringstream buffer_;
    LogLevel level_;
    std::string ctxId_;
    
    // FIX MISRA 10-0-1: Split boolean declarations
    bool active_;
    bool first_arg_;
    
    LogHandler logHandler_;
};

inline std::ostream& operator<<(std::ostream& os, const LogStream& stream) {
    return os << stream.ToString();
}

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOG_STREAM_H