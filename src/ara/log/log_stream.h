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

// [SWS_LOG_00173] Definition of API class ara::log::LogStream
class LogStream final {
public:
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    LogStream(LogStream&& other) noexcept;
    LogStream& operator=(LogStream&&) = delete;
    ~LogStream() noexcept;

    // Public default constructor for Sinks/Internal use
    LogStream() noexcept;

    void Flush() noexcept;
    
    // [SWS_LOG_00129]
    LogStream& WithLocation(core::StringView file, int line) noexcept;
    
    // [SWS_LOG_00132]
    LogStream& WithTag(core::StringView tag) noexcept;

    // [SWS_LOG_00258]
    template <typename T>
    LogStream& WithPrivacy(T value) noexcept { return *this; }

    std::string ToString() const noexcept;

    // Operator overloads
    LogStream& operator<<(bool value) noexcept;
    LogStream& operator<<(std::uint8_t value) noexcept;
    LogStream& operator<<(std::uint16_t value) noexcept;
    LogStream& operator<<(std::uint32_t value) noexcept;
    LogStream& operator<<(std::uint64_t value) noexcept;
    LogStream& operator<<(std::int8_t value) noexcept;
    LogStream& operator<<(std::int16_t value) noexcept;
    LogStream& operator<<(std::int32_t value) noexcept;
    LogStream& operator<<(std::int64_t value) noexcept;
    LogStream& operator<<(float value) noexcept;
    LogStream& operator<<(double value) noexcept;
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
            buffer_ << value.count() << "ticks"; 
        } catch (...) {}
        return *this;
    }

    template <typename T>
    LogStream& operator<<(const Argument<T>& arg) noexcept {
        try {
            if (!active_) return *this;
            AddSeparator();
            if (arg.name_) buffer_ << arg.name_ << ":";
            buffer_ << arg.value;
            if (arg.unit_) buffer_ << ":" << arg.unit_;
        } catch (...) {}
        return *this;
    }

private:
    friend class Logger;
    friend class sink::LogSink;

    // Internal constructor used by Logger
    LogStream(LogLevel level, const std::string& ctxId, bool active, LogHandler handler) noexcept;
    
    void AddSeparator();

    std::ostringstream buffer_;
    LogLevel level_;
    std::string ctxId_;
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