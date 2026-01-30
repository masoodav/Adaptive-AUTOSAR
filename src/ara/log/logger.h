#ifndef ARA_LOG_LOGGER_H
#define ARA_LOG_LOGGER_H

#include <functional>
#include <tuple>
#include <atomic> 
#include "./common.h"
#include "./log_stream.h"

namespace ara {
namespace log {

class LoggerManager; 

using ConnectionStateHandler = std::function<void(ClientState)>;

enum class Fmt : std::uint16_t {
    kDefault = 0,
    kDec = 1,
    kOct = 2,
    kHex = 3,
    kBin = 4,
    kDecFloat = 5,
    kEngFloat = 6,
    kHexFloat = 7,
    kAutoFloat = 8
};

struct Format final {
    Fmt fmt;
    std::uint16_t precision;
};

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

constexpr Format Dflt() noexcept { return {Fmt::kDefault, 0}; }
constexpr Format Dec(std::uint16_t precision = 0) noexcept { return {Fmt::kDec, precision}; }
constexpr Format Hex(std::uint16_t precision = 0) noexcept { return {Fmt::kHex, precision}; }
constexpr Format Bin(std::uint16_t precision = 0) noexcept { return {Fmt::kBin, precision}; }
constexpr Format Oct(std::uint16_t precision = 0) noexcept { return {Fmt::kOct, precision}; }
constexpr Format AutoFloat(std::uint16_t precision = 6) noexcept { return {Fmt::kAutoFloat, precision}; }

template <typename T>
Argument<T> Arg(T&& arg, const char* name = nullptr, const char* unit = nullptr, Format format = Dflt()) noexcept {
    return Argument<T>(std::forward<T>(arg), name, unit, format);
}

// [SWS_LOG_00172] Definition of API class ara::log::Logger
class Logger final {
public:
    Logger() = delete;
    Logger(const Logger&) = delete;            // Non-copyable
    Logger& operator=(const Logger&) = delete; // Non-assignable

    // FIX: Move Constructor is required for std::vector<Logger> usage in LoggingFramework
    Logger(Logger&& other) noexcept; 

    ~Logger();

    bool IsEnabled(LogLevel logLevel) const noexcept;

    template <typename MsgId, typename... Params>
    void Log(const MsgId& id, const Params&... args) noexcept {
    }

    LogStream LogFatal() const noexcept;
    LogStream LogError() const noexcept;
    LogStream LogWarn() const noexcept;
    LogStream LogInfo() const noexcept;
    LogStream LogDebug() const noexcept;
    LogStream LogVerbose() const noexcept;
    
    LogStream WithLevel(LogLevel logLevel) const noexcept;

    template <typename... Attrs, typename MsgId, typename... Params>
    void LogWith(const std::tuple<Attrs...>& attrs, const MsgId& msgId, const Params&... params) noexcept {
    }

    void SetThreshold(LogLevel threshold) noexcept;

private:
    friend Logger& CreateLogger(core::StringView ctxId, core::StringView ctxDesc, LogLevel level) noexcept;
    friend Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept;
    friend class LoggerManager; 

    Logger(const std::string& ctxId, const std::string& ctxDesc, LogLevel level);

    std::string contextId_;
    std::string contextDescription_;
    std::atomic<LogLevel> currentLimit_; 
};

// Global API Functions
Logger& CreateLogger(core::StringView ctxId, core::StringView ctxDescription, LogLevel ctxDefLogLevel = LogLevel::kWarn) noexcept;
Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept;
void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept;

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOGGER_H