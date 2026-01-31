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

// FIX: Define a generic handler for log messages (Level + Message Content)
using LogHandler = std::function<void(LogLevel, const std::string&)>;

using ConnectionStateHandler = std::function<void(ClientState)>;

enum class Fmt : std::uint16_t { kDefault, kDec, kOct, kHex, kBin, kDecFloat, kEngFloat, kHexFloat, kAutoFloat };

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
// ... [Other helper functions Dflt/Dec/Hex... same as before] ...
template <typename T>
Argument<T> Arg(T&& arg, const char* name = nullptr, const char* unit = nullptr, Format format = Dflt()) noexcept {
    return Argument<T>(std::forward<T>(arg), name, unit, format);
}

class Logger final {
public:
    Logger() = delete;
    Logger(const Logger&) = delete;            
    Logger& operator=(const Logger&) = delete; 
    Logger(Logger&& other) noexcept; 
    ~Logger();

    bool IsEnabled(LogLevel logLevel) const noexcept;

    // [Modeled Log method stub...]
    template <typename MsgId, typename... Params>
    void Log(const MsgId& id, const Params&... args) noexcept {}

    LogStream LogFatal() const noexcept;
    LogStream LogError() const noexcept;
    LogStream LogWarn() const noexcept;
    LogStream LogInfo() const noexcept;
    LogStream LogDebug() const noexcept;
    LogStream LogVerbose() const noexcept;
    
    LogStream WithLevel(LogLevel logLevel) const noexcept;

    // [LogWith stub...]
    template <typename... Attrs, typename MsgId, typename... Params>
    void LogWith(const std::tuple<Attrs...>& attrs, const MsgId& msgId, const Params&... params) noexcept {}

    void SetThreshold(LogLevel threshold) noexcept;

    // FIX: Method for the Framework to inject the Sink logic
    void SetLogHandler(LogHandler handler);

private:
    friend Logger& CreateLogger(core::StringView ctxId, core::StringView ctxDesc, LogLevel level) noexcept;
    friend Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept;
    friend class LoggerManager; 

    Logger(const std::string& ctxId, const std::string& ctxDesc, LogLevel level);

    std::string contextId_;
    std::string contextDescription_;
    std::atomic<LogLevel> currentLimit_; 
    
    // FIX: The callback that connects to the Sink
    LogHandler logHandler_;
};

// Global API
Logger& CreateLogger(core::StringView ctxId, core::StringView ctxDescription, LogLevel ctxDefLogLevel = LogLevel::kWarn) noexcept;
Logger& CreateLogger(const core::InstanceSpecifier& is) noexcept;
void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept;

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOGGER_H