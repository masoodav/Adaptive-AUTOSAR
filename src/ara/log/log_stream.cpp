#include "./log_stream.h"

#include <bitset>
#include <cstdio>
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

void AppendHexByte(std::ostringstream& stream, unsigned int value)
{
    static_cast<void>(stream << std::hex << std::nouppercase << std::setfill('0') << std::setw(2) << value);
}

std::string TrimBinaryText(const std::string& text)
{
    const std::string::size_type position = text.find('1');
    if (position == std::string::npos)
    {
        return std::string("0");
    }

    return text.substr(position);
}

void ApplyIntegralFormat(std::ostringstream& stream, Format format, std::uint16_t precision)
{
    switch (format.fmt)
    {
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
}

void ApplyFloatingFormat(std::ostringstream& stream, Format format)
{
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
}

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

// Tags: [SWS_LOG_00002] [SWS_LOG_00259] [SWS_LOG_00260]
void SubmitSnapshot(const std::shared_ptr<internal::LogStreamState>& state) noexcept
{
    if ((state == nullptr) || (state->logger == nullptr))
    {
        return;
    }

    try
    {
        internal::MessageRecord record;
        static_cast<void>(record.ctx_id = state->logger->ctx_id);
        static_cast<void>(record.ctx_description = state->logger->ctx_description);
        static_cast<void>(record.level = state->level);
        std::vector<std::string>::size_type argument_index = 0U;
        const std::vector<std::string>::size_type argument_count = state->arguments.size();
        while (argument_index < argument_count)
        {
            record.arguments.push_back(internal::RenderedArgument{state->arguments[argument_index]});
            ++argument_index;
        }
        static_cast<void>(record.file = state->file);
        static_cast<void>(record.line = state->line);
        static_cast<void>(record.has_location = state->has_location);
        static_cast<void>(record.tag = state->tag);
        static_cast<void>(record.has_tag = state->has_tag);
        static_cast<void>(record.privacy = state->privacy);
        static_cast<void>(record.has_privacy = state->has_privacy);
        internal::Backend::Instance().Submit(record);
    }
    catch (...)
    {
    }
}

// Tags: [SWS_LOG_00172] [SWS_LOG_00173] [SWS_LOG_00174] [SWS_LOG_00175]
// [SWS_LOG_00176] [SWS_LOG_00177] [SWS_LOG_00201] [SWS_LOG_00203]
// [SWS_LOG_00204] [SWS_LOG_00205] [SWS_LOG_00206] [SWS_LOG_00207]
// [SWS_LOG_00256]
template <typename IntegerType>
std::string FormatIntegral(IntegerType value, Format format, bool signed_value)
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
            std::string text = TrimBinaryText(bits.to_string());
            if (text.size() < precision)
            {
                static_cast<void>(text.insert(0U, precision - text.size(), '0'));
            }
            return text;
        }
        default:
            ApplyIntegralFormat(stream, format, precision);
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

std::string FormatFloating(double value, Format format)
{
    try
    {
        std::ostringstream stream;
        ApplyFloatingFormat(stream, format);
        static_cast<void>(stream << value);
        return stream.str();
    }
    catch (...)
    {
        return std::string();
    }
}

std::string FormatBytes(ara::core::Span<const ara::core::Byte> value)
{
    try
    {
        std::ostringstream stream;
        const std::size_t byte_count = value.size();
        std::size_t index = 0U;
        while (index < byte_count)
        {
            if (index != 0U)
            {
                static_cast<void>(stream << '\'');
            }
            AppendHexByte(stream, static_cast<unsigned int>(value[index]));
            ++index;
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
    : state_()
{
    try
    {
        state_ = std::make_shared<internal::LogStreamState>(std::shared_ptr<internal::LoggerState>(), LogLevel::kOff);
    }
    catch (...)
    {
    }
}

LogStream::LogStream(LogStream&& other) noexcept : state_(std::move(other.state_)) {}

LogStream::~LogStream() noexcept
{
    if (state_)
    {
        SubmitSnapshot(state_);
    }
}

// Tags: [SWS_LOG_00018] [SWS_LOG_00259] [SWS_LOG_00260]
void LogStream::Flush() noexcept
{
    if ((state_ != nullptr) && (state_->logger == nullptr))
    {
        try
        {
            state_->arguments.clear();
            state_->file.clear();
            state_->line = 0;
            state_->has_location = false;
            state_->tag.clear();
            state_->has_tag = false;
            state_->privacy = 0U;
            state_->has_privacy = false;
        }
        catch (...)
        {
        }
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

    std::vector<std::string>::size_type argument_index = 0U;
    const std::vector<std::string>::size_type argument_count = state_->arguments.size();
    while (argument_index < argument_count)
    {
        if (needs_separator)
        {
            static_cast<void>(stream << ' ');
        }
        static_cast<void>(stream << state_->arguments[argument_index]);
        needs_separator = true;
        ++argument_index;
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

// Tags: [SWS_LOG_00221] [SWS_LOG_00222]
LogStream& LogStream::WithLocation(ara::core::StringView file, int line) noexcept
{
    try
    {
        if (state_)
        {
            static_cast<void>(state_->file = file.ToString());
            static_cast<void>(state_->line = line);
            static_cast<void>(state_->has_location = true);
        }
    }
    catch (...)
    {
    }
    return *this;
}

// Tags: [SWS_LOG_00215] [SWS_LOG_00216]
LogStream& LogStream::WithTag(ara::core::StringView tag) noexcept
{
    try
    {
        if (state_)
        {
            static_cast<void>(state_->tag = tag.ToString());
            static_cast<void>(state_->has_tag = true);
        }
    }
    catch (...)
    {
    }
    return *this;
}

// Tags: [SWS_LOG_00047] [SWS_LOG_00048] [SWS_LOG_00049] [SWS_LOG_00050]
// [SWS_LOG_00051] [SWS_LOG_00062] [SWS_LOG_00064] [SWS_LOG_00065]
// [SWS_LOG_00066] [SWS_LOG_00067] [SWS_LOG_00068] [SWS_LOG_00069]
// [SWS_LOG_00070]
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

// Tags: [SWS_LOG_00209] [SWS_LOG_00210] [SWS_LOG_00211] [SWS_LOG_00212]
// [SWS_LOG_00213] [SWS_LOG_00214]
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
    std::vector<std::uint8_t>::size_type data_index = 0U;
    const std::vector<std::uint8_t>::size_type data_count = data.size();
    while (data_index < data_count)
    {
        AppendHexByte(stream, static_cast<unsigned int>(data[data_index]));
        ++data_index;
    }
    AppendPayload(stream.str());
    return *this;
}

LogStream& LogStream::operator<<(const LogStream& value) noexcept
{
    AppendPayload(value.ToString());
    return *this;
}

void LogStream::AppendPayload(const std::string& value)
{
    AppendArgumentText(value, nullptr, nullptr);
}

void LogStream::AppendArgumentText(const std::string& value, const char* name, const char* unit)
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
            static_cast<void>(text.append(name));
            text.push_back(':');
        }
        static_cast<void>(text.append(value));
        if (unit != nullptr)
        {
            text.push_back(':');
            static_cast<void>(text.append(unit));
        }

        state_->arguments.push_back(text);
    }
    catch (...)
    {
    }
}

// Tags: [SWS_LOG_00217] [SWS_LOG_00218]
void LogStream::SetPrivacy(std::uint8_t privacy)
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

// Tags: [SWS_LOG_00223] [SWS_LOG_00224] [SWS_LOG_00225] [SWS_LOG_00226]
LogStream& operator<<(LogStream& out, const ara::core::ErrorCode& ec) noexcept
{
    try
    {
        std::string error_text(ec.Domain().Name());
        error_text.push_back(':');
        static_cast<void>(error_text.append(std::to_string(ec.Value())));
        out.AppendPayload(error_text);
    }
    catch (...)
    {
    }
    return out;
}

LogStream& operator<<(LogStream& out, const ara::core::InstanceSpecifier& value) noexcept
{
    out.AppendPayload(value.ToString());
    return out;
}

LogStream& operator<<(LogStream& out, const void* value) noexcept
{
    try
    {
        char pointer_text[(sizeof(void*) * 2U) + 3U];
        const int written = std::snprintf(pointer_text, sizeof(pointer_text), "%p", value);
        if (written > 0)
        {
            out.AppendPayload(std::string(pointer_text));
        }
    }
    catch (...)
    {
    }
    return out;
}

namespace internal
{

std::string FormatValue(bool value, Format)
{
    return value ? "1" : "0";
}

std::string FormatValue(std::uint8_t value, Format format)
{
    return FormatIntegral<std::uint8_t>(value, format, false);
}

std::string FormatValue(std::uint16_t value, Format format)
{
    return FormatIntegral<std::uint16_t>(value, format, false);
}

std::string FormatValue(std::uint32_t value, Format format)
{
    return FormatIntegral<std::uint32_t>(value, format, false);
}

std::string FormatValue(std::uint64_t value, Format format)
{
    return FormatIntegral<std::uint64_t>(value, format, false);
}

std::string FormatValue(std::int8_t value, Format format)
{
    return FormatIntegral<std::int8_t>(value, format, true);
}

std::string FormatValue(std::int16_t value, Format format)
{
    return FormatIntegral<std::int16_t>(value, format, true);
}

std::string FormatValue(std::int32_t value, Format format)
{
    return FormatIntegral<std::int32_t>(value, format, true);
}

std::string FormatValue(std::int64_t value, Format format)
{
    return FormatIntegral<std::int64_t>(value, format, true);
}

std::string FormatValue(float value, Format format)
{
    return FormatFloating(static_cast<double>(value), format);
}

std::string FormatValue(double value, Format format)
{
    return FormatFloating(value, format);
}

std::string FormatValue(ara::core::StringView value, Format)
{
    return value.ToString();
}

std::string FormatValue(ara::core::Span<const ara::core::Byte> value, Format)
{
    return FormatBytes(value);
}

std::string FormatValue(const char* value, Format)
{
    return (value == nullptr) ? std::string() : std::string(value);
}

}  // namespace internal

}  // namespace log
}  // namespace ara
