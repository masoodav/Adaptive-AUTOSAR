#include "./log_stream.h"

#include <bitset>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

#include "backend.h"

namespace ara
{
namespace log
{

namespace
{

std::string LevelToText(LogLevel value) noexcept
{
    switch (value)
    {
    case LogLevel::kFatal:
        return "Fatal";
    case LogLevel::kError:
        return "Error";
    case LogLevel::kWarn:
        return "Warn";
    case LogLevel::kInfo:
        return "Info";
    case LogLevel::kDebug:
        return "Debug";
    case LogLevel::kVerbose:
        return "Verbose";
    case LogLevel::kOff:
    default:
        return "Off";
    }
}

void SubmitSnapshot(const std::shared_ptr<internal::LogStreamState>& state) noexcept
{
    if ((!state) || (!state->logger))
    {
        return;
    }

    try
    {
        internal::MessageRecord record;
        record.ctx_id = state->logger->ctx_id;
        record.ctx_description = state->logger->ctx_description;
        record.level = state->level;
        for (const std::string& argument : state->arguments)
        {
            record.arguments.push_back(internal::RenderedArgument{argument});
        }
        record.file = state->file;
        record.line = state->line;
        record.has_location = state->has_location;
        record.tag = state->tag;
        record.has_tag = state->has_tag;
        record.privacy = state->privacy;
        record.has_privacy = state->has_privacy;
        internal::Backend::Instance().Submit(record);
    }
    catch (...)
    {
    }
}

template <typename IntegerType>
std::string FormatIntegral(IntegerType value, Format format, bool signed_value) noexcept
{
    try
    {
        std::ostringstream stream;
        const std::uint16_t precision = (format.precision == 0U) ? 1U : format.precision;

        switch (format.fmt)
        {
        case Fmt::kBin:
        {
            std::bitset<64U> bits(static_cast<typename std::make_unsigned<IntegerType>::type>(value));
            std::string text = bits.to_string();
            std::size_t position = text.find('1');
            if (position == std::string::npos)
            {
                text = "0";
            }
            else
            {
                text = text.substr(position);
            }
            if (text.size() < precision)
            {
                text.insert(0U, precision - text.size(), '0');
            }
            return text;
        }
        case Fmt::kHex:
            static_cast<void>(stream << std::hex << std::nouppercase << std::setfill('0') << std::setw(precision));
            break;
        case Fmt::kOct:
            static_cast<void>(stream << std::oct << std::setfill('0') << std::setw(precision));
            break;
        case Fmt::kDec:
        case Fmt::kDefault:
        default:
            static_cast<void>(stream << std::dec << std::setfill('0') << std::setw(precision));
            break;
        }

        if (signed_value)
        {
            static_cast<void>(stream << static_cast<long long>(value));
        }
        else
        {
            static_cast<void>(stream << static_cast<unsigned long long>(value));
        }
        return stream.str();
    }
    catch (...)
    {
        return std::string();
    }
}

std::string FormatFloating(double value, Format format) noexcept
{
    try
    {
        std::ostringstream stream;
        switch (format.fmt)
        {
        case Fmt::kDecFloat:
            static_cast<void>(stream << std::fixed << std::setprecision(format.precision));
            break;
        case Fmt::kEngFloat:
            static_cast<void>(stream << std::scientific << std::setprecision(format.precision));
            break;
        case Fmt::kHexFloat:
            static_cast<void>(stream << std::hexfloat);
            break;
        case Fmt::kAutoFloat:
            static_cast<void>(stream << std::setprecision(format.precision));
            break;
        case Fmt::kDefault:
        default:
            break;
        }
        static_cast<void>(stream << value);
        return stream.str();
    }
    catch (...)
    {
        return std::string();
    }
}

std::string FormatBytes(ara::core::Span<const ara::core::Byte> value) noexcept
{
    try
    {
        std::ostringstream stream;
        const std::size_t byte_count = value.size();
        for (std::size_t index = 0U; index < byte_count; ++index)
        {
            if (index != 0U)
            {
                static_cast<void>(stream << '\'');
            }
            static_cast<void>(stream << std::hex << std::nouppercase << std::setfill('0') << std::setw(2)
                                     << static_cast<unsigned int>(value[index]));
        }
        return stream.str();
    }
    catch (...)
    {
        return std::string();
    }
}

}  // namespace

LogStream::LogStream(std::shared_ptr<internal::LogStreamState> state) noexcept : state_(std::move(state)) {}

LogStream::LogStream() noexcept
    : state_(std::make_shared<internal::LogStreamState>(
          std::shared_ptr<internal::LoggerState>(), LogLevel::kOff))
{
}

LogStream::LogStream(LogStream&& other) noexcept : state_(std::move(other.state_)) {}

LogStream::~LogStream() noexcept
{
    if (state_)
    {
        SubmitSnapshot(state_);
    }
}

void LogStream::Flush() noexcept
{
    if (state_ && (!state_->logger))
    {
        state_->arguments.clear();
        state_->file.clear();
        state_->line = 0;
        state_->has_location = false;
        state_->tag.clear();
        state_->has_tag = false;
        state_->privacy = 0U;
        state_->has_privacy = false;
        return;
    }

    SubmitSnapshot(state_);
}

std::string LogStream::ToString() const
{
    if (!state_)
    {
        return std::string();
    }

    std::ostringstream stream;
    bool needs_separator = false;

    if (state_->logger)
    {
        if (!state_->logger->ctx_id.empty())
        {
            static_cast<void>(stream << state_->logger->ctx_id);
            needs_separator = true;
        }

        if (!state_->logger->ctx_description.empty())
        {
            if (needs_separator)
            {
                static_cast<void>(stream << ' ');
            }
            static_cast<void>(stream << state_->logger->ctx_description);
            needs_separator = true;
        }
    }

    for (const std::string& argument : state_->arguments)
    {
        if (needs_separator)
        {
            static_cast<void>(stream << ' ');
        }
        static_cast<void>(stream << argument);
        needs_separator = true;
    }

    if (state_->has_tag)
    {
        if (needs_separator)
        {
            static_cast<void>(stream << ' ');
        }
        static_cast<void>(stream << "tag:" << state_->tag);
        needs_separator = true;
    }

    if (state_->has_location)
    {
        if (needs_separator)
        {
            static_cast<void>(stream << ' ');
        }
        static_cast<void>(stream << state_->file << ':' << state_->line);
        needs_separator = true;
    }

    if (state_->has_privacy)
    {
        if (needs_separator)
        {
            static_cast<void>(stream << ' ');
        }
        static_cast<void>(stream << "privacy:" << static_cast<unsigned int>(state_->privacy));
    }

    return stream.str();
}

LogStream& LogStream::WithLocation(ara::core::StringView file, int line) noexcept
{
    try
    {
        if (state_)
        {
            state_->file = file.ToString();
            state_->line = line;
            state_->has_location = true;
        }
    }
    catch (...)
    {
    }
    return *this;
}

LogStream& LogStream::WithTag(ara::core::StringView tag) noexcept
{
    try
    {
        if (state_)
        {
            state_->tag = tag.ToString();
            state_->has_tag = true;
        }
    }
    catch (...)
    {
    }
    return *this;
}

LogStream& LogStream::operator<<(bool value) noexcept
{
    AppendPayload(value ? "1" : "0");
    return *this;
}

LogStream& LogStream::operator<<(std::uint8_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(std::uint16_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(std::uint32_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(std::uint64_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(std::int8_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(std::int16_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(std::int32_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(std::int64_t value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(float value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(double value) noexcept
{
    AppendPayload(internal::FormatValue(value, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(const std::string& value) noexcept
{
    AppendPayload(value);
    return *this;
}

LogStream& LogStream::operator<<(ara::core::StringView value) noexcept
{
    AppendPayload(value.ToString());
    return *this;
}

LogStream& LogStream::operator<<(const char* const value) noexcept
{
    AppendPayload((value == nullptr) ? std::string() : std::string(value));
    return *this;
}

LogStream& LogStream::operator<<(ara::core::Span<const ara::core::Byte> data) noexcept
{
    AppendPayload(internal::FormatValue(data, Dflt()));
    return *this;
}

LogStream& LogStream::operator<<(const std::vector<std::uint8_t>& data) noexcept
{
    std::ostringstream stream;
    const std::vector<std::uint8_t>::size_type data_size = data.size();
    for (std::vector<std::uint8_t>::size_type index = 0U; index < data_size; ++index)
    {
        static_cast<void>(stream << std::hex << std::nouppercase << std::setfill('0') << std::setw(2)
                                 << static_cast<unsigned int>(data[index]));
    }
    AppendPayload(stream.str());
    return *this;
}

LogStream& LogStream::operator<<(const LogStream& value) noexcept
{
    AppendPayload(value.ToString());
    return *this;
}

void LogStream::AppendPayload(const std::string& value) noexcept
{
    AppendArgumentText(value, nullptr, nullptr);
}

void LogStream::AppendArgumentText(const std::string& value, const char* name, const char* unit) noexcept
{
    if (!state_)
    {
        return;
    }

    try
    {
        std::string text;
        if (name != nullptr)
        {
            text.append(name);
            text.push_back(':');
        }
        text.append(value);
        if (unit != nullptr)
        {
            text.push_back(':');
            text.append(unit);
        }

        state_->arguments.push_back(text);
    }
    catch (...)
    {
    }
}

void LogStream::SetPrivacy(std::uint8_t privacy) noexcept
{
    if (state_)
    {
        state_->privacy = privacy;
        state_->has_privacy = true;
    }
}

LogStream& operator<<(LogStream& out, LogLevel value) noexcept
{
    out.AppendPayload(LevelToText(value));
    return out;
}

LogStream& operator<<(LogStream& out, const ara::core::ErrorCode& ec) noexcept
{
    std::ostringstream stream;
    static_cast<void>(stream << ec.Domain().Name() << ':' << ec.Value());
    out.AppendPayload(stream.str());
    return out;
}

LogStream& operator<<(LogStream& out, const ara::core::InstanceSpecifier& value) noexcept
{
    out.AppendPayload(value.ToString());
    return out;
}

LogStream& operator<<(LogStream& out, const void* value) noexcept
{
    std::ostringstream stream;
    static_cast<void>(stream << value);
    out.AppendPayload(stream.str());
    return out;
}

namespace internal
{

std::string FormatValue(bool value, Format) noexcept
{
    return value ? "1" : "0";
}

std::string FormatValue(std::uint8_t value, Format format) noexcept
{
    return FormatIntegral<std::uint8_t>(value, format, false);
}

std::string FormatValue(std::uint16_t value, Format format) noexcept
{
    return FormatIntegral<std::uint16_t>(value, format, false);
}

std::string FormatValue(std::uint32_t value, Format format) noexcept
{
    return FormatIntegral<std::uint32_t>(value, format, false);
}

std::string FormatValue(std::uint64_t value, Format format) noexcept
{
    return FormatIntegral<std::uint64_t>(value, format, false);
}

std::string FormatValue(std::int8_t value, Format format) noexcept
{
    return FormatIntegral<std::int8_t>(value, format, true);
}

std::string FormatValue(std::int16_t value, Format format) noexcept
{
    return FormatIntegral<std::int16_t>(value, format, true);
}

std::string FormatValue(std::int32_t value, Format format) noexcept
{
    return FormatIntegral<std::int32_t>(value, format, true);
}

std::string FormatValue(std::int64_t value, Format format) noexcept
{
    return FormatIntegral<std::int64_t>(value, format, true);
}

std::string FormatValue(float value, Format format) noexcept
{
    return FormatFloating(static_cast<double>(value), format);
}

std::string FormatValue(double value, Format format) noexcept
{
    return FormatFloating(value, format);
}

std::string FormatValue(ara::core::StringView value, Format) noexcept
{
    return value.ToString();
}

std::string FormatValue(ara::core::Span<const ara::core::Byte> value, Format) noexcept
{
    return FormatBytes(value);
}

std::string FormatValue(const char* value, Format) noexcept
{
    return (value == nullptr) ? std::string() : std::string(value);
}

}  // namespace internal

}  // namespace log
}  // namespace ara
