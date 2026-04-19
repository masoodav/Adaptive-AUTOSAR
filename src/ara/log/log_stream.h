#ifndef ARA_LOG_LOG_STREAM_H_
#define ARA_LOG_LOG_STREAM_H_

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../core/error_code.h"
#include "../core/instance_specifier.h"
#include "../core/span.h"
#include "../core/string_view.h"
#include "common.h"
#include "log_fwd.h"
#include "logger.h"

namespace ara
{
namespace log
{

class Logger;

class LogStream final
{
public:
    struct State;

    LogStream(const LogStream&) = delete;
    LogStream(LogStream&& other) noexcept;
    LogStream& operator=(const LogStream&) = delete;
    LogStream& operator=(LogStream&&) = delete;
    ~LogStream() noexcept;

    void Flush() noexcept;
    LogStream& WithLocation(ara::core::StringView file, int line) noexcept;

    template <typename T>
    LogStream& WithPrivacy(T value) noexcept
    {
        static_assert(std::is_integral<T>::value || std::is_enum<T>::value,
                      "Privacy values must be integral or enum.");
        SetPrivacy(static_cast<std::uint8_t>(value));
        return *this;
    }

    LogStream& WithTag(ara::core::StringView tag) noexcept;

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
    LogStream& operator<<(ara::core::StringView value) noexcept;
    LogStream& operator<<(const char* const value) noexcept;
    LogStream& operator<<(ara::core::Span<const ara::core::Byte> data) noexcept;

    template <typename T>
    LogStream& operator<<(const Argument<T>& arg) noexcept
    {
        AppendArgument(arg);
        return *this;
    }

private:
    explicit LogStream(std::shared_ptr<State> state) noexcept;

    void AppendPayload(const std::string& value) noexcept;
    void AppendArgumentText(const std::string& value, const char* name, const char* unit) noexcept;
    void SetPrivacy(std::uint8_t privacy) noexcept;

    template <typename T>
    void AppendArgument(const Argument<T>& arg) noexcept;

    std::shared_ptr<State> state_;

    friend class Logger;
    friend LogStream& operator<<(LogStream& out, LogLevel value) noexcept;
    friend LogStream& operator<<(LogStream& out, const ara::core::ErrorCode& ec) noexcept;
    friend LogStream& operator<<(LogStream& out, const ara::core::InstanceSpecifier& value) noexcept;
    friend LogStream& operator<<(LogStream& out, const void* value) noexcept;
    template <typename Rep, typename Period>
    friend LogStream& operator<<(LogStream& out, const std::chrono::duration<Rep, Period>& value) noexcept;
};

LogStream& operator<<(LogStream& out, LogLevel value) noexcept;
LogStream& operator<<(LogStream& out, const ara::core::ErrorCode& ec) noexcept;
LogStream& operator<<(LogStream& out, const ara::core::InstanceSpecifier& value) noexcept;
LogStream& operator<<(LogStream& out, const void* value) noexcept;

template <typename Rep, typename Period>
LogStream& operator<<(LogStream& out, const std::chrono::duration<Rep, Period>& value) noexcept;

}  // namespace log
}  // namespace ara



namespace ara
{
namespace log
{

struct LogStream::State
{
    State(std::shared_ptr<Logger::State> logger_state, LogLevel severity) noexcept
        : logger(std::move(logger_state)),
          level(severity),
          line(0),
          has_location(false),
          has_tag(false),
          privacy(0U),
          has_privacy(false)
    {
    }

    std::shared_ptr<Logger::State> logger;
    LogLevel level;
    std::vector<std::string> arguments;
    std::string file;
    int line;
    bool has_location;
    std::string tag;
    bool has_tag;
    std::uint8_t privacy;
    bool has_privacy;
};

namespace internal
{

std::string FormatValue(bool value, Format format) noexcept;
std::string FormatValue(std::uint8_t value, Format format) noexcept;
std::string FormatValue(std::uint16_t value, Format format) noexcept;
std::string FormatValue(std::uint32_t value, Format format) noexcept;
std::string FormatValue(std::uint64_t value, Format format) noexcept;
std::string FormatValue(std::int8_t value, Format format) noexcept;
std::string FormatValue(std::int16_t value, Format format) noexcept;
std::string FormatValue(std::int32_t value, Format format) noexcept;
std::string FormatValue(std::int64_t value, Format format) noexcept;
std::string FormatValue(float value, Format format) noexcept;
std::string FormatValue(double value, Format format) noexcept;
std::string FormatValue(ara::core::StringView value, Format format) noexcept;
std::string FormatValue(ara::core::Span<const ara::core::Byte> value, Format format) noexcept;
std::string FormatValue(const char* value, Format format) noexcept;

template <typename T>
std::string FormatValue(const T& value, Format) noexcept
{
    return std::to_string(value);
}

}  // namespace internal

template <typename T>
void LogStream::AppendArgument(const Argument<T>& arg) noexcept
{
    AppendArgumentText(
        internal::FormatValue(arg.Value(), arg.GetFormat()), arg.Name(), arg.Unit());
}

template <typename Rep, typename Period>
LogStream& operator<<(LogStream& out, const std::chrono::duration<Rep, Period>& value) noexcept
{
    std::string suffix("s");
    if (std::ratio_equal<Period, std::nano>::value)
    {
        suffix = "ns";
    }
    else if (std::ratio_equal<Period, std::micro>::value)
    {
        suffix = "us";
    }
    else if (std::ratio_equal<Period, std::milli>::value)
    {
        suffix = "ms";
    }
    else if (std::ratio_equal<Period, std::centi>::value)
    {
        suffix = "cs";
    }
    else if (std::ratio_equal<Period, std::deci>::value)
    {
        suffix = "ds";
    }
    else if (std::ratio_equal<Period, std::deca>::value)
    {
        suffix = "das";
    }
    else if (std::ratio_equal<Period, std::hecto>::value)
    {
        suffix = "hs";
    }
    else if (std::ratio_equal<Period, std::kilo>::value)
    {
        suffix = "ks";
    }
    else if (std::ratio_equal<Period, std::mega>::value)
    {
        suffix = "Ms";
    }
    else if (std::ratio_equal<Period, std::giga>::value)
    {
        suffix = "Gs";
    }

    out.AppendPayload(std::to_string(value.count()) + suffix);
    return out;
}

}  // namespace log
}  // namespace ara

#endif  // ARA_LOG_LOG_STREAM_H_
