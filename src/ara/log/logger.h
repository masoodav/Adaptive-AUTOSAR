#ifndef ARA_LOG_LOGGER_H_
#define ARA_LOG_LOGGER_H_

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../core/instance_specifier.h"
#include "../core/span.h"
#include "../core/string_view.h"
#include "common.h"
#include "log_fwd.h"

namespace ara
{
namespace log
{

enum class Fmt : std::uint16_t
{
    kDefault = 0U,
    kDec = 1U,
    kOct = 2U,
    kHex = 3U,
    kBin = 4U,
    kDecFloat = 5U,
    kEngFloat = 6U,
    kHexFloat = 7U,
    kAutoFloat = 8U
};

struct Format final
{
    Fmt fmt;
    std::uint16_t precision;
};

constexpr Format Dflt() noexcept
{
    return Format{Fmt::kDefault, 0U};
}

constexpr Format AutoFloat(std::uint16_t precision = 6U) noexcept
{
    return Format{Fmt::kAutoFloat, precision};
}

constexpr Format AutoFloatMax() noexcept
{
    return Format{Fmt::kAutoFloat, static_cast<std::uint16_t>(std::numeric_limits<double>::max_digits10)};
}

constexpr Format Bin() noexcept
{
    return Format{Fmt::kBin, 1U};
}

constexpr Format Bin(std::uint16_t precision) noexcept
{
    return Format{Fmt::kBin, precision};
}

constexpr Format Dec() noexcept
{
    return Format{Fmt::kDec, 1U};
}

constexpr Format Dec(std::uint16_t precision) noexcept
{
    return Format{Fmt::kDec, precision};
}

constexpr Format DecFloat(std::uint16_t precision = 6U) noexcept
{
    return Format{Fmt::kDecFloat, precision};
}

constexpr Format DecFloatMax() noexcept
{
    return Format{Fmt::kDecFloat, static_cast<std::uint16_t>(std::numeric_limits<double>::max_digits10)};
}

constexpr Format Hex() noexcept
{
    return Format{Fmt::kHex, 1U};
}

constexpr Format Hex(std::uint16_t precision) noexcept
{
    return Format{Fmt::kHex, precision};
}

constexpr Format HexFloat(std::uint16_t precision) noexcept
{
    return Format{Fmt::kHexFloat, precision};
}

constexpr Format HexFloatMax() noexcept
{
    return Format{Fmt::kHexFloat, static_cast<std::uint16_t>(std::numeric_limits<double>::max_digits10)};
}

constexpr Format Oct() noexcept
{
    return Format{Fmt::kOct, 1U};
}

constexpr Format Oct(std::uint16_t precision) noexcept
{
    return Format{Fmt::kOct, precision};
}

template <typename T>
class Argument final
{
public:
    Argument(T value, const char* name, const char* unit, Format format) noexcept
        : value_(value),
          name_(name),
          unit_(unit),
          format_(format)
    {
    }

    const T& Value() const noexcept
    {
        return value_;
    }

    const char* Name() const noexcept
    {
        return name_;
    }

    const char* Unit() const noexcept
    {
        return unit_;
    }

    Format GetFormat() const noexcept
    {
        return format_;
    }

private:
    T value_;
    const char* name_;
    const char* unit_;
    Format format_;
};

namespace internal
{

template <typename T>
struct IsStringLike : std::is_convertible<T, ara::core::StringView>
{
};

template <typename T>
struct IsByteSpanLike : std::is_convertible<T, ara::core::Span<const ara::core::Byte> >
{
};

template <typename T>
struct IsAllowedArgumentType
{
    static const bool value = std::is_arithmetic<typename std::decay<T>::type>::value ||
                              std::is_same<typename std::decay<T>::type, bool>::value ||
                              IsStringLike<typename std::decay<T>::type>::value ||
                              IsByteSpanLike<typename std::decay<T>::type>::value;
};

template <typename T>
struct AllowsUnit
{
    static const bool value = !std::is_same<typename std::decay<T>::type, bool>::value &&
                              !IsStringLike<typename std::decay<T>::type>::value &&
                              !IsByteSpanLike<typename std::decay<T>::type>::value;
};

}  // namespace internal

template <typename T>
Argument<typename std::decay<T>::type> Arg(
    T&& arg,
    const char* name = nullptr,
    const char* unit = nullptr,
    Format format = Dflt()) noexcept
{
    typedef typename std::decay<T>::type ValueType;
    static_assert(internal::IsAllowedArgumentType<ValueType>::value, "Unsupported argument type for ara::log::Arg");
    static_assert(internal::AllowsUnit<ValueType>::value || (std::is_same<decltype(unit), const char*>::value),
                  "Unit validation is performed at runtime.");

    if ((!internal::AllowsUnit<ValueType>::value) && (unit != nullptr))
    {
        return Argument<ValueType>(ValueType(), nullptr, nullptr, format);
    }

    return Argument<ValueType>(static_cast<ValueType>(arg), name, unit, format);
}

class Logger;

Logger& CreateLogger(const ara::core::InstanceSpecifier& is) noexcept;
Logger& CreateLogger(ara::core::StringView ctxId, ara::core::StringView ctxDescription) noexcept;
Logger& CreateLogger(
    ara::core::StringView ctxId,
    ara::core::StringView ctxDescription,
    LogLevel ctxDefLogLevel) noexcept;

class Logger final
{
public:
    struct State
    {
        State(std::string ctx, std::string description, LogLevel threshold_value) noexcept
            : ctx_id(std::move(ctx)),
              ctx_description(std::move(description)),
              threshold(threshold_value)
        {
        }

        std::string ctx_id;
        std::string ctx_description;
        LogLevel threshold;
    };

    Logger() = delete;
    ~Logger();

    static Logger& CreateLogger(const ara::core::InstanceSpecifier& is) noexcept
    {
        return ara::log::CreateLogger(is);
    }

    static Logger& CreateLogger(ara::core::StringView ctxId, ara::core::StringView ctxDescription) noexcept
    {
        return ara::log::CreateLogger(ctxId, ctxDescription);
    }

    static Logger& CreateLogger(
        ara::core::StringView ctxId,
        ara::core::StringView ctxDescription,
        LogLevel ctxDefLogLevel) noexcept
    {
        return ara::log::CreateLogger(ctxId, ctxDescription, ctxDefLogLevel);
    }

    bool IsEnabled(LogLevel logLevel) const noexcept;

    template <typename MsgId, typename... Params>
    void Log(const MsgId& id, const Params&... args) noexcept;

    LogStream LogDebug() const noexcept;
    LogStream LogError() const noexcept;
    LogStream LogFatal() const noexcept;
    LogStream LogInfo() const noexcept;
    LogStream LogVerbose() const noexcept;
    LogStream LogWarn() const noexcept;

    template <typename... Attrs, typename MsgId, typename... Params>
    void LogWith(const std::tuple<Attrs...>& attrs, const MsgId& msgId, const Params&... params) noexcept;

    void SetThreshold(LogLevel threshold) noexcept;
    LogStream WithLevel(LogLevel logLevel) const noexcept;

private:
    explicit Logger(std::shared_ptr<State> state) noexcept;

    std::shared_ptr<State> state_;

    friend Logger& CreateLogger(const ara::core::InstanceSpecifier& is) noexcept;
    friend Logger& CreateLogger(
        ara::core::StringView ctxId,
        ara::core::StringView ctxDescription,
        LogLevel ctxDefLogLevel) noexcept;
    friend Logger& CreateLogger(ara::core::StringView ctxId, ara::core::StringView ctxDescription) noexcept;
    friend class LogStream;
};

void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept;

}  // namespace log
}  // namespace ara

#include "log_stream.h"

namespace ara
{
namespace log
{

template <typename MsgId, typename... Params>
void Logger::Log(const MsgId& id, const Params&... args) noexcept
{
    LogStream stream = WithLevel(LogLevel::kInfo);
    stream << id;
    int dummy[] = {0, ((stream << args), 0)...};
    static_cast<void>(dummy);
}

namespace internal
{

template <std::size_t Index, typename... Attrs>
struct TupleAppender final
{
    static void Append(LogStream& stream, const std::tuple<Attrs...>& attrs) noexcept
    {
        TupleAppender<Index - 1U, Attrs...>::Append(stream, attrs);
        stream << std::get<Index - 1U>(attrs);
    }
};

template <typename... Attrs>
struct TupleAppender<0U, Attrs...> final
{
    static void Append(LogStream&, const std::tuple<Attrs...>&) noexcept {}
};

}  // namespace internal

template <typename... Attrs, typename MsgId, typename... Params>
void Logger::LogWith(const std::tuple<Attrs...>& attrs, const MsgId& msgId, const Params&... params) noexcept
{
    LogStream stream = WithLevel(LogLevel::kInfo);
    internal::TupleAppender<sizeof...(Attrs), Attrs...>::Append(stream, attrs);
    stream << msgId;
    int dummy[] = {0, ((stream << params), 0)...};
    static_cast<void>(dummy);
}

}  // namespace log
}  // namespace ara

#endif  // ARA_LOG_LOGGER_H_
