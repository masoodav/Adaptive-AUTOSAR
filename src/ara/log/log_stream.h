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

namespace internal
{
std::string FormatValue(bool value, Format format);
std::string FormatValue(std::uint8_t value, Format format);
std::string FormatValue(std::uint16_t value, Format format);
std::string FormatValue(std::uint32_t value, Format format);
std::string FormatValue(std::uint64_t value, Format format);
std::string FormatValue(std::int8_t value, Format format);
std::string FormatValue(std::int16_t value, Format format);
std::string FormatValue(std::int32_t value, Format format);
std::string FormatValue(std::int64_t value, Format format);
std::string FormatValue(float value, Format format);
std::string FormatValue(double value, Format format);
std::string FormatValue(ara::core::StringView value, Format format);
std::string FormatValue(ara::core::Span<const ara::core::Byte> value, Format format);
std::string FormatValue(const char* value, Format format);
}

// Tags: [SWS_LOG_00018] [SWS_LOG_00021]
class LogStream final
{
public:
    LogStream() noexcept;
    LogStream(const LogStream&) = delete;
    LogStream(LogStream&& other) noexcept;
    LogStream& operator=(const LogStream&) = delete;
    LogStream& operator=(LogStream&&) = delete;
    ~LogStream() noexcept;

    // Tags: [SWS_LOG_00018] [SWS_LOG_00259] [SWS_LOG_00260]
    void Flush() noexcept;
    std::string ToString() const;
    // Tags: [SWS_LOG_00221] [SWS_LOG_00222]
    LogStream& WithLocation(ara::core::StringView file, int line) noexcept;

    // Tags: [SWS_LOG_00217] [SWS_LOG_00218]
    template <typename T>
    LogStream& WithPrivacy(T value) noexcept
    {
        static_assert(std::is_integral<T>::value || std::is_enum<T>::value,
                      "Privacy values must be integral or enum.");
        SetPrivacy(static_cast<std::uint8_t>(value));
        return *this;
    }

    // Tags: [SWS_LOG_00215] [SWS_LOG_00216]
    LogStream& WithTag(ara::core::StringView tag) noexcept;

    // Tags: [SWS_LOG_00047] [SWS_LOG_00048] [SWS_LOG_00049]
    // [SWS_LOG_00050] [SWS_LOG_00051] [SWS_LOG_00062]
    // [SWS_LOG_00064] [SWS_LOG_00065] [SWS_LOG_00066]
    // [SWS_LOG_00067] [SWS_LOG_00068] [SWS_LOG_00069]
    // [SWS_LOG_00070]
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
    // Tags: [SWS_LOG_00209] [SWS_LOG_00210] [SWS_LOG_00211]
    // [SWS_LOG_00212] [SWS_LOG_00213] [SWS_LOG_00214]
    LogStream& operator<<(const std::string& value) noexcept;
    LogStream& operator<<(ara::core::StringView value) noexcept;
    LogStream& operator<<(const char* const value) noexcept;
    LogStream& operator<<(ara::core::Span<const ara::core::Byte> data) noexcept;
    LogStream& operator<<(const std::vector<std::uint8_t>& data) noexcept;
    LogStream& operator<<(const LogStream& value) noexcept;

    template <typename T>
    typename std::enable_if<std::is_enum<T>::value, LogStream&>::type operator<<(T value) noexcept
    {
        typedef typename std::underlying_type<T>::type UnderlyingType;
        return (*this) << static_cast<UnderlyingType>(value);
    }

    template <typename T>
    LogStream& operator<<(const Argument<T>& arg) noexcept
    {
        // Tags: [SWS_LOG_00172] [SWS_LOG_00173] [SWS_LOG_00174]
        // [SWS_LOG_00175] [SWS_LOG_00176] [SWS_LOG_00177]
        // [SWS_LOG_00201] [SWS_LOG_00203] [SWS_LOG_00204]
        // [SWS_LOG_00205] [SWS_LOG_00206] [SWS_LOG_00207] [SWS_LOG_00256]
        AppendArgumentText(
            internal::FormatValue(arg.Value(), arg.GetFormat()), arg.Name(), arg.Unit());
        return *this;
    }

private:
    explicit LogStream(std::shared_ptr<internal::LogStreamState> state) noexcept;

    void AppendPayload(const std::string& value);
    void AppendArgumentText(const std::string& value, const char* name, const char* unit);
    void SetPrivacy(std::uint8_t privacy);

    std::shared_ptr<internal::LogStreamState> state_;

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
namespace internal
{

template <typename T>
std::string FormatValue(const T& value, Format)
{
    return std::to_string(value);
}

}  // namespace internal

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
