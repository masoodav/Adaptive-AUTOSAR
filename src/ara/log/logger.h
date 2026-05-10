#ifndef ARA_LOG_LOGGER_H
#define ARA_LOG_LOGGER_H

#include <functional>
#include <tuple>
#include <atomic> 
#include <string>
#include "./common.h"
#include "./log_stream.h"

namespace ara {
namespace log {

class LoggerManager; 

using LogHandler = std::function<void(LogLevel, const std::string&)>;

// [SWS_LOG_00265] Definition of API type ara::log::ConnectionStateHandler
using ConnectionStateHandler = std::function<void(ClientState)>;

// [SWS_LOG_00206] Definition of API enum ara::log::Fmt
enum class Fmt : std::uint16_t { kDefault, kDec, kOct, kHex, kBin, kDecFloat, kEngFloat, kHexFloat, kAutoFloat };

// [SWS_LOG_00207] Definition of API class ara::log::Format
struct Format final {
    Fmt fmt;
    std::uint16_t precision;
};

// [SWS_LOG_00261] Definition of API class ara::log::Argument
template <typename T>
class Argument final {
public:
    Argument(T&& arg, const char* name, const char* unit, Format fmt)
        : value(std::forward<T>(arg)), name_(name), unit_(unit), format_(fmt) {}
    const T& value;
    const char* name_;
    const char* unit_;
    Format format_;
};

// [SWS_LOG_00208] - [SWS_LOG_00224] Helper functions for Format
constexpr Format Dflt() noexcept { return {Fmt::kDefault, 0}; }
constexpr Format Dec(std::uint16_t precision = 0) noexcept { return {Fmt::kDec, precision}; }
constexpr Format Hex(std::uint16_t precision = 0) noexcept { return {Fmt::kHex, precision}; }
constexpr Format Bin(std::uint16_t precision = 0) noexcept { return {Fmt::kBin, precision}; }
constexpr Format Oct(std::uint16_t precision = 0) noexcept { return {Fmt::kOct, precision}; }
constexpr Format AutoFloat(std::uint16_t precision = 6) noexcept { return {Fmt::kAutoFloat, precision}; }

// [SWS_LOG_00201] Definition of API function ara::log::Arg
template <typename T>
Argument<T> Arg(T&& arg, const char* name = nullptr, const char* unit = nullptr, Format format = Dflt()) noexcept {
    return Argument<T>(std::forward<T>(arg), name, unit, format);
}

// [SWS_LOG_00172] Definition of API class ara::log::Logger
class Logger final {
public:
    // [SWS_LOG_00259] Default Constructor deleted
    Logger() = delete;
    Logger(const Logger&) = delete;            
    Logger& operator=(const Logger&) = delete; 
    Logger(Logger&& other) noexcept; 
    
    // [SWS_LOG_00260] Destructor
    ~Logger();

    // [SWS_LOG_00070] IsEnabled
    bool IsEnabled(LogLevel logLevel) const noexcept;

    // [SWS_LOG_00204] Log modeled message
    template <typename MsgId, typename... Params>
    void Log(const MsgId& id, const Params&... args) noexcept {
        static_cast<void>(id);
    }

    // [SWS_LOG_00064] - [SWS_LOG_00069] Factory methods
    LogStream LogFatal() const noexcept;
    LogStream LogError() const noexcept;
    LogStream LogWarn() const noexcept;
    LogStream LogInfo() const noexcept;
    LogStream LogDebug() const noexcept;
    LogStream LogVerbose() const noexcept;
    
    // [SWS_LOG_00131] WithLevel
    LogStream WithLevel(LogLevel logLevel) const noexcept;

    // [SWS_LOG_00133] LogWith
    template <typename... Attrs, typename MsgId, typename... Params>
    void LogWith(const std::tuple<Attrs...>& attrs, const MsgId& msgId, const Params&... params) noexcept {
        static_cast<void>(attrs);
        static_cast<void>(msgId);
    }

    // [SWS_LOG_00255] SetThreshold
    void SetThreshold(LogLevel threshold) noexcept;

    void SetLogHandler(LogHandler handler);

private:
    friend Logger& CreateLogger(core::StringView ctxId, core::StringView ctxDesc, LogLevel level) noexcept;
    friend Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept;
    friend class LoggerManager; 

    Logger(const std::string& ctxId, const std::string& ctxDesc, LogLevel level);

    std::string contextId_{""};
    std::string contextDescription_{""};
    std::atomic<LogLevel> currentLimit_{LogLevel::kWarn}; 
    LogHandler logHandler_{nullptr};
};

// [SWS_LOG_00021] CreateLogger (Strings)
Logger& CreateLogger(core::StringView ctxId, core::StringView ctxDescription, LogLevel ctxDefLogLevel = LogLevel::kWarn) noexcept;

// [SWS_LOG_00256] CreateLogger (InstanceSpecifier)
Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept;

// [SWS_LOG_00205] RegisterConnectionStateHandler
void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept;

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOGGER_H