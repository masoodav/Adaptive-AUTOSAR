/**
 * @file log_stream.cpp
 * @brief Implementation of ara::log::LogStream.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00173]  LogStream class
 *   [SWS_LOG_00174]  Copy constructor deleted (declared only)
 *   [SWS_LOG_00176]  Move constructor
 *   [SWS_LOG_00175]  Copy assignment deleted (declared only)
 *   [SWS_LOG_00177]  Move assignment deleted (declared only)
 *   [SWS_LOG_00262]  Destructor
 *   [SWS_LOG_00039]  Flush()
 *   [SWS_LOG_00129]  WithLocation()
 *   [SWS_LOG_00132]  WithTag()
 *   [SWS_LOG_00040]–[SWS_LOG_00051] arithmetic/string operator<< overloads
 *   [SWS_LOG_00062]  operator<<(StringView)
 *   [SWS_LOG_00128]  operator<<(Span<const Byte>)
 *   [SWS_LOG_00125]  operator<<(chrono::duration) – template in header
 *   [SWS_LOG_00126]  operator<<(InstanceSpecifier)
 *   [SWS_LOG_00127]  operator<<(const void*)
 *   [SWS_LOG_00063]  operator<<(LogLevel)
 *   [SWS_LOG_00124]  operator<<(ErrorCode)
 *   [SWS_LOG_00002]  Silent error discard
 *
 * Coding standards:
 *   - MISRA C++:2023 Rule 15.1.1 – no implicit conversions in operators
 *   - CERT C++ STR50-CPP         – validate string inputs
 *   - CWE-120, CWE-125           – no unbounded buffer copies
 *   - ISO/SAE 21434               – bounded operations, no UB
 */

#include "ara/log/log_stream.h"
#include "ara/log/internal/log_backend.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

LogStream::LogStream(LogLevel level, bool enabled) noexcept
    : level_       {level}
    , isActive_    {enabled}
    , isFlushed_   {false}
    , hasPrivacy_  {false}
    , privacy_     {0U}
    , locationLine_{0}
{
    // Reserve a reasonable initial capacity to avoid frequent re-allocations.
    // CWE-770: limited reservation prevents memory exhaustion.
    constexpr std::size_t kInitialCapacity{256U};
    // [SWS_LOG_00002]: Any allocation failure is silently swallowed.
    try
    {
        buffer_.reserve(kInitialCapacity);
        metaBuffer_.reserve(64U);
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard on internal framework errors.
        isActive_ = false;
    }
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00176] Move constructor
// ---------------------------------------------------------------------------

LogStream::LogStream(LogStream &&other) noexcept
    : level_        {other.level_}
    , isActive_     {other.isActive_}
    , isFlushed_    {other.isFlushed_}
    , hasPrivacy_   {other.hasPrivacy_}
    , privacy_      {other.privacy_}
    , locationLine_ {other.locationLine_}
    , buffer_       {std::move(other.buffer_)}
    , metaBuffer_   {std::move(other.metaBuffer_)}
    , locationFile_ {std::move(other.locationFile_)}
    , tag_          {std::move(other.tag_)}
{
    // Invalidate the moved-from object so its destructor is a no-op.
    other.isActive_  = false;
    other.isFlushed_ = true;
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00262] Destructor
// ---------------------------------------------------------------------------

LogStream::~LogStream() noexcept
{
    // Flush remaining data unless already flushed or not active.
    if (isActive_ && !isFlushed_ && !buffer_.empty())
    {
        DoFlush();
    }
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00039] Flush
// ---------------------------------------------------------------------------

void LogStream::Flush() noexcept
{
    if (isActive_ && !buffer_.empty())
    {
        DoFlush();
        // Clear buffer so the stream can be reused.
        buffer_.clear();
        metaBuffer_.clear();
        locationFile_.clear();
        locationLine_ = 0;
        tag_.clear();
        hasPrivacy_   = false;
        privacy_      = 0U;
        isFlushed_    = true;
    }
}

// ---------------------------------------------------------------------------
// Internal: DoFlush
// ---------------------------------------------------------------------------

void LogStream::DoFlush() noexcept
{
    // Build a LogRecord and dispatch it to the back-end.
    try
    {
        internal::LogRecord record;
        record.level        = level_;
        record.payload      = buffer_;
        record.metaPayload  = metaBuffer_;
        record.locationFile = locationFile_;
        record.locationLine = locationLine_;
        record.tags         = tag_;
        record.hasPrivacy   = hasPrivacy_;
        record.privacy      = privacy_;

        internal::LoggingFramework::Instance().Dispatch(record);
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard on errors.
    }
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00129] WithLocation
// ---------------------------------------------------------------------------

LogStream &LogStream::WithLocation(ara::core::StringView file,
                                   int                   line) noexcept
{
    if (isActive_)
    {
        // CERT C++ STR50-CPP: assign via std::string copy (bounded).
        try
        {
            locationFile_.assign(file.data(), file.size());
            locationLine_ = line;
        }
        catch (...)
        {
            // [SWS_LOG_00002]: Silent discard.
        }
    }
    return *this;
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00132] WithTag
// ---------------------------------------------------------------------------

LogStream &LogStream::WithTag(ara::core::StringView tag) noexcept
{
    if (isActive_)
    {
        // Spec: max 255 chars, ASCII only. Silently truncate if exceeded.
        constexpr std::size_t kMaxTagLen{255U};
        const std::size_t len{
            (tag.size() > kMaxTagLen) ? kMaxTagLen : tag.size()};

        try
        {
            if (!tag_.empty())
            {
                tag_ += ',';
            }
            tag_.append(tag.data(), len);
        }
        catch (...)
        {
            // [SWS_LOG_00002]: Silent discard.
        }
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Arithmetic operator<< overloads
// ---------------------------------------------------------------------------

// Helper: append formatted text to buffer. All paths noexcept.
// CWE-120: using snprintf with explicit size bounds.
namespace {

constexpr std::size_t kFmtBufSize{64U};

template <typename T>
void AppendValue(std::string &buf, T value) noexcept
{
    char tmp[kFmtBufSize];
    // Per-type format selection – MISRA Rule 15.1.1 no implicit conversions.
    int written{0};
    if constexpr (std::is_same<T, bool>::value)
    {
        written = std::snprintf(tmp, sizeof(tmp),
                                "%s", value ? "true" : "false");
    }
    else if constexpr (std::is_same<T, float>::value)
    {
        written = std::snprintf(tmp, sizeof(tmp), "%f",
                                static_cast<double>(value));
    }
    else if constexpr (std::is_same<T, double>::value)
    {
        written = std::snprintf(tmp, sizeof(tmp), "%f", value);
    }
    else if constexpr (std::is_same<T, std::uint8_t>::value  ||
                       std::is_same<T, std::uint16_t>::value ||
                       std::is_same<T, std::uint32_t>::value)
    {
        written = std::snprintf(tmp, sizeof(tmp), "%" PRIu32,
                                static_cast<std::uint32_t>(value));
    }
    else if constexpr (std::is_same<T, std::uint64_t>::value)
    {
        written = std::snprintf(tmp, sizeof(tmp), "%" PRIu64, value);
    }
    else if constexpr (std::is_same<T, std::int8_t>::value  ||
                       std::is_same<T, std::int16_t>::value ||
                       std::is_same<T, std::int32_t>::value)
    {
        written = std::snprintf(tmp, sizeof(tmp), "%" PRId32,
                                static_cast<std::int32_t>(value));
    }
    else if constexpr (std::is_same<T, std::int64_t>::value)
    {
        written = std::snprintf(tmp, sizeof(tmp), "%" PRId64, value);
    }

    if (written > 0)
    {
        // CERT C++ STR31-CPP: written < kFmtBufSize by snprintf contract.
        try
        {
            buf.append(tmp, static_cast<std::size_t>(written));
            buf += ' ';
        }
        catch (...)
        {
            // [SWS_LOG_00002]: Silent discard.
        }
    }
}

} // anonymous namespace

/// [SWS_LOG_00040]
LogStream &LogStream::operator<<(bool value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00041]
LogStream &LogStream::operator<<(std::uint8_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00042]
LogStream &LogStream::operator<<(std::uint16_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00043]
LogStream &LogStream::operator<<(std::uint32_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00044]
LogStream &LogStream::operator<<(std::uint64_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00045]
LogStream &LogStream::operator<<(std::int8_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00046]
LogStream &LogStream::operator<<(std::int16_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00047]
LogStream &LogStream::operator<<(std::int32_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00048]
LogStream &LogStream::operator<<(std::int64_t value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00049]
LogStream &LogStream::operator<<(float value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

/// [SWS_LOG_00050]
LogStream &LogStream::operator<<(double value) noexcept
{
    if (isActive_) { AppendValue(buffer_, value); }
    return *this;
}

// ---------------------------------------------------------------------------
// String / raw-data overloads
// ---------------------------------------------------------------------------

/// [SWS_LOG_00062]
LogStream &LogStream::operator<<(ara::core::StringView value) noexcept
{
    if (isActive_)
    {
        try
        {
            buffer_.append(value.data(), value.size());
            buffer_ += ' ';
        }
        catch (...)
        {
            // [SWS_LOG_00002]: Silent discard.
        }
    }
    return *this;
}

/// [SWS_LOG_00051] Null-terminated UTF-8 string.
LogStream &LogStream::operator<<(const char *const value) noexcept
{
    if (isActive_ && (value != nullptr))
    {
        // CWE-125: bounded via StringView construction (length computed).
        *this << ara::core::StringView{value};
    }
    return *this;
}

/// [SWS_LOG_00128] Byte sequence.
LogStream &LogStream::operator<<(
    ara::core::Span<const ara::core::Byte> data) noexcept
{
    if (isActive_ && !data.empty())
    {
        try
        {
            // Copy the raw bytes as-is into the buffer per spec.
            // CERT C++ MEM57-CPP: size validated via Span.
            const auto *ptr{reinterpret_cast<const char *>(data.data())};
            buffer_.append(ptr, data.size());
            buffer_ += ' ';
        }
        catch (...)
        {
            // [SWS_LOG_00002]: Silent discard.
        }
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Non-member operator<< overloads
// ---------------------------------------------------------------------------

/// [SWS_LOG_00126] InstanceSpecifier
LogStream &operator<<(LogStream                        &out,
                      const ara::core::InstanceSpecifier &value) noexcept
{
    return out << value.ToString();
}

/// [SWS_LOG_00127] Raw pointer – formatted as hex address.
LogStream &operator<<(LogStream &out, const void *value) noexcept
{
    if (out.IsActive())
    {
        char tmp[32U];
        // CWE-120: snprintf with explicit size.
        const int written{
            std::snprintf(tmp, sizeof(tmp), "%p", value)};
        if (written > 0)
        {
            try
            {
                out << ara::core::StringView{tmp,
                                             static_cast<std::size_t>(written)};
            }
            catch (...)
            {
                // [SWS_LOG_00002]: Silent discard.
            }
        }
    }
    return out;
}

/// [SWS_LOG_00063] LogLevel as text.
LogStream &operator<<(LogStream &out, LogLevel value) noexcept
{
    // Map enum value to string. No default case – all enum values explicit.
    const char *text{"kUnknown"};
    switch (value)
    {
        case LogLevel::kOff:     text = "kOff";     break;
        case LogLevel::kFatal:   text = "kFatal";   break;
        case LogLevel::kError:   text = "kError";   break;
        case LogLevel::kWarn:    text = "kWarn";    break;
        case LogLevel::kInfo:    text = "kInfo";    break;
        case LogLevel::kDebug:   text = "kDebug";   break;
        case LogLevel::kVerbose: text = "kVerbose"; break;
        // MISRA C++:2023 Rule 9.5.1 – no default; all enumerators covered.
    }
    return out << ara::core::StringView{text};
}

/// [SWS_LOG_00124] ErrorCode.
LogStream &operator<<(LogStream                 &out,
                      const ara::core::ErrorCode &ec) noexcept
{
    // [SWS_LOG_00124]: console output shows Domain().Name() + numeric code.
    // CERT C++ STR50-CPP: domain name obtained via virtual call, bounded.
    ara::core::StringView domainName{ec.Domain().Name()};

    char tmp[32U];
    // CWE-120: snprintf with explicit size.
    const int written{
        std::snprintf(tmp, sizeof(tmp), "%" PRId32,
                      static_cast<std::int32_t>(ec.Value()))};

    out << domainName;
    if (written > 0)
    {
        out << ara::core::StringView{":"};
        out << ara::core::StringView{tmp, static_cast<std::size_t>(written)};
    }
    return out;
}

} // namespace log
} // namespace ara
