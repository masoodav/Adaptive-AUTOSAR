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
    // [SWS_LOG_00174] Copy Constructor deleted
    LogStream(const LogStream&) = delete;
    // [SWS_LOG_00175] Copy Assignment deleted
    LogStream& operator=(const LogStream&) = delete;
    // [SWS_LOG_00176] Move Constructor
    LogStream(LogStream&& other) noexcept;
    // [SWS_LOG_00177] Move Assignment deleted
    LogStream& operator=(LogStream&&) = delete;
    
    // [SWS_LOG_00262] Destructor
    ~LogStream() noexcept;

    LogStream() noexcept;

    // [SWS_LOG_00039] Flush
    void Flush() noexcept;
    
    // [SWS_LOG_00129] WithLocation
    LogStream& WithLocation(core::StringView file, int line) noexcept;
    
    // [SWS_LOG_00132] WithTag
    LogStream& WithTag(core::StringView tag) noexcept;

    // [SWS_LOG_00258] WithPrivacy
    template <typename T>
    LogStream& WithPrivacy(T value) noexcept { 
        static_cast<void>(value); 
        return *this; 
    }

    std::string ToString() const noexcept;

    // Arithmetic Types Template (Covers [SWS_LOG_00040]..[SWS_LOG_00050])
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

    // [SWS_LOG_00062] StringView
    LogStream& operator<<(const core::StringView value) noexcept;
    // [SWS_LOG_00051] char*
    LogStream& operator<<(const char* const value) noexcept;
    // [SWS_LOG_00127] void*
    LogStream& operator<<(const void* value) noexcept; 
    // [SWS_LOG_00063] LogLevel
    LogStream& operator<<(LogLevel value) noexcept; 
    // [SWS_LOG_00124] ErrorCode
    LogStream& operator<<(const core::ErrorCode& ec) noexcept; 
    // [SWS_LOG_00126] InstanceSpecifier
    LogStream& operator<<(const core::InstanceSpecifier& value) noexcept;
    // [SWS_LOG_00128] Span
    LogStream& operator<<(core::Span<const core::Byte> data) noexcept;
    
    // [Non-SWS] std::string support
    LogStream& operator<<(const std::string& value) noexcept;
    // [Non-SWS] LogStream support
    LogStream& operator<<(const LogStream& other) noexcept;

    // [SWS_LOG_00125] Duration
    template <typename Rep, typename Period>
    LogStream& operator<<(const std::chrono::duration<Rep, Period>& value) noexcept {
        try {
            if (!active_) return *this;
            AddSeparator();
            static_cast<void>(buffer_ << value.count() << "ticks");
        } catch (...) {}
        return *this;
    }

    // [SWS_LOG_00203] Argument
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

    std::ostringstream buffer_{};
    LogLevel level_{LogLevel::kInfo};
    std::string ctxId_{""};
    bool active_{false};
    bool first_arg_{true};
    LogHandler logHandler_{nullptr};
};

inline std::ostream& operator<<(std::ostream& os, const LogStream& stream) {
    return os << stream.ToString();
}

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOG_STREAM_H