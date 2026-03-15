/**
 * @file log_stream.cpp
 * @brief Implementation of ara::log::LogStream – C++14 compliant.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00173]  LogStream class
 *   [SWS_LOG_00176]  Move constructor
 *   [SWS_LOG_00262]  Destructor
 *   [SWS_LOG_00039]  Flush()
 *   [SWS_LOG_00129]  WithLocation()
 *   [SWS_LOG_00132]  WithTag()
 *   [SWS_LOG_00040-00051] arithmetic/string operator<<
 *   [SWS_LOG_00062]  operator<<(StringView)
 *   [SWS_LOG_00128]  operator<<(Span<const Byte>)
 *   [SWS_LOG_00126]  operator<<(InstanceSpecifier)
 *   [SWS_LOG_00127]  operator<<(const void*)
 *   [SWS_LOG_00063]  operator<<(LogLevel)
 *   [SWS_LOG_00124]  operator<<(ErrorCode)
 *   [SWS_LOG_00002]  Silent error discard
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#include "./log_stream.h"
#include "./log_backend.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// Default constructor
// Creates an active LogStream at LogLevel::kInfo with an empty buffer.
// Required by log_sink.cpp:
//   LogStream _result;   (GetAppstamp line 16, GetTimestamp line 32)
// ---------------------------------------------------------------------------

LogStream::LogStream() noexcept
    : level_       (LogLevel::kInfo)
    , isActive_    (true)
    , isFlushed_   (false)
    , hasPrivacy_  (false)
    , privacy_     (0U)
    , locationLine_(0)
{
    try
    {
        buffer_.reserve(256U);
        metaBuffer_.reserve(64U);
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard on allocation failure.
        isActive_ = false;
    }
}

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

LogStream::LogStream(LogLevel level, bool enabled) noexcept
    : level_       (level)
    , isActive_    (enabled)
    , isFlushed_   (false)
    , hasPrivacy_  (false)
    , privacy_     (0U)
    , locationLine_(0)
{
    // CWE-770: limited reservation prevents memory exhaustion.
    // [SWS_LOG_00002]: silently discard any allocation failure.
    try
    {
        buffer_.reserve(256U);
        metaBuffer_.reserve(64U);
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
        isActive_ = false;
    }
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00176] Move constructor
// ---------------------------------------------------------------------------

LogStream::LogStream(LogStream &&other) noexcept
    : level_       (other.level_)
    , isActive_    (other.isActive_)
    , isFlushed_   (other.isFlushed_)
    , hasPrivacy_  (other.hasPrivacy_)
    , privacy_     (other.privacy_)
    , locationLine_(other.locationLine_)
    , buffer_      (std::move(other.buffer_))
    , metaBuffer_  (std::move(other.metaBuffer_))
    , locationFile_(std::move(other.locationFile_))
    , tag_         (std::move(other.tag_))
{
    // Invalidate moved-from object.
    other.isActive_  = false;
    other.isFlushed_ = true;
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00262] Destructor
// ---------------------------------------------------------------------------

LogStream::~LogStream() noexcept
{
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
        // [SWS_LOG_00002]: silent discard.
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
        try
        {
            locationFile_.assign(file.data(), file.size());
            locationLine_ = line;
        }
        catch (...)
        {
            // [SWS_LOG_00002]: silent discard.
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
        // Spec: max 255 chars, ASCII only. Truncate silently.
        const std::size_t kMaxTagLen = 255U;
        const std::size_t len =
            (tag.size() > kMaxTagLen) ? kMaxTagLen : tag.size();
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
            // [SWS_LOG_00002]: silent discard.
        }
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Arithmetic operator<< helpers (C++14: no if constexpr; use overloads)
// ---------------------------------------------------------------------------

namespace {

// CWE-120: all snprintf calls use explicit buffer size.
static const std::size_t kFmtBufSize = 64U;

// Each type gets its own helper to avoid if-constexpr (C++17).

void AppendBool(std::string &buf, bool value) noexcept
{
    try
    {
        buf.append(value ? "true" : "false");
        buf += ' ';
    }
    catch (...) {}
}

void AppendU32(std::string &buf, std::uint32_t value) noexcept
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRIu32, value);
    if (n > 0)
    {
        try { buf.append(tmp, static_cast<std::size_t>(n)); buf += ' '; }
        catch (...) {}
    }
}

void AppendU64(std::string &buf, std::uint64_t value) noexcept
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRIu64, value);
    if (n > 0)
    {
        try { buf.append(tmp, static_cast<std::size_t>(n)); buf += ' '; }
        catch (...) {}
    }
}

void AppendI32(std::string &buf, std::int32_t value) noexcept
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRId32, value);
    if (n > 0)
    {
        try { buf.append(tmp, static_cast<std::size_t>(n)); buf += ' '; }
        catch (...) {}
    }
}

void AppendI64(std::string &buf, std::int64_t value) noexcept
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRId64, value);
    if (n > 0)
    {
        try { buf.append(tmp, static_cast<std::size_t>(n)); buf += ' '; }
        catch (...) {}
    }
}

void AppendFloat(std::string &buf, float value) noexcept
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%f",
                                static_cast<double>(value));
    if (n > 0)
    {
        try { buf.append(tmp, static_cast<std::size_t>(n)); buf += ' '; }
        catch (...) {}
    }
}

void AppendDouble(std::string &buf, double value) noexcept
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%f", value);
    if (n > 0)
    {
        try { buf.append(tmp, static_cast<std::size_t>(n)); buf += ' '; }
        catch (...) {}
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Arithmetic operator<< implementations
// ---------------------------------------------------------------------------

LogStream &LogStream::operator<<(bool value) noexcept
{
    if (isActive_) { AppendBool(buffer_, value); }
    return *this;
}

LogStream &LogStream::operator<<(std::uint8_t value) noexcept
{
    if (isActive_) { AppendU32(buffer_, static_cast<std::uint32_t>(value)); }
    return *this;
}

LogStream &LogStream::operator<<(std::uint16_t value) noexcept
{
    if (isActive_) { AppendU32(buffer_, static_cast<std::uint32_t>(value)); }
    return *this;
}

LogStream &LogStream::operator<<(std::uint32_t value) noexcept
{
    if (isActive_) { AppendU32(buffer_, value); }
    return *this;
}

LogStream &LogStream::operator<<(std::uint64_t value) noexcept
{
    if (isActive_) { AppendU64(buffer_, value); }
    return *this;
}

LogStream &LogStream::operator<<(std::int8_t value) noexcept
{
    if (isActive_) { AppendI32(buffer_, static_cast<std::int32_t>(value)); }
    return *this;
}

LogStream &LogStream::operator<<(std::int16_t value) noexcept
{
    if (isActive_) { AppendI32(buffer_, static_cast<std::int32_t>(value)); }
    return *this;
}

LogStream &LogStream::operator<<(std::int32_t value) noexcept
{
    if (isActive_) { AppendI32(buffer_, value); }
    return *this;
}

LogStream &LogStream::operator<<(std::int64_t value) noexcept
{
    if (isActive_) { AppendI64(buffer_, value); }
    return *this;
}

LogStream &LogStream::operator<<(float value) noexcept
{
    if (isActive_) { AppendFloat(buffer_, value); }
    return *this;
}

LogStream &LogStream::operator<<(double value) noexcept
{
    if (isActive_) { AppendDouble(buffer_, value); }
    return *this;
}

// ---------------------------------------------------------------------------
// String / raw-data overloads
// ---------------------------------------------------------------------------

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
            // [SWS_LOG_00002]: silent discard.
        }
    }
    return *this;
}

LogStream &LogStream::operator<<(const char *const value) noexcept
{
    if (isActive_ && (value != NULL))
    {
        // CWE-125: bounded via StringView (computes length internally).
        *this << ara::core::StringView(value);
    }
    return *this;
}

LogStream &LogStream::operator<<(
    ara::core::Span<const ara::core::Byte> data) noexcept
{
    if (isActive_ && !data.empty())
    {
        try
        {
            // Copy raw bytes as-is per spec. CERT C++ MEM57-CPP: bounded.
            const char *ptr =
                reinterpret_cast<const char *>(data.data());
            buffer_.append(ptr, data.size());
            buffer_ += ' ';
        }
        catch (...)
        {
            // [SWS_LOG_00002]: silent discard.
        }
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Non-member operator<< implementations
// ---------------------------------------------------------------------------

LogStream &operator<<(LogStream &out,
                      const ara::core::InstanceSpecifier &value) noexcept
{
    return out << value.ToString();
}

LogStream &operator<<(LogStream &out, const void *value) noexcept
{
    if (out.IsActive())
    {
        char tmp[32U];
        // CWE-120: explicit buffer size.
        const int n = std::snprintf(tmp, sizeof(tmp), "%p", value);
        if (n > 0)
        {
            try
            {
                out << ara::core::StringView(tmp,
                                             static_cast<std::size_t>(n));
            }
            catch (...)
            {
                // [SWS_LOG_00002]: silent discard.
            }
        }
    }
    return out;
}

LogStream &operator<<(LogStream &out, LogLevel value) noexcept
{
    const char *text = "kUnknown";
    switch (value)
    {
        case LogLevel::kOff:     text = "kOff";     break;
        case LogLevel::kFatal:   text = "kFatal";   break;
        case LogLevel::kError:   text = "kError";   break;
        case LogLevel::kWarn:    text = "kWarn";    break;
        case LogLevel::kInfo:    text = "kInfo";    break;
        case LogLevel::kDebug:   text = "kDebug";   break;
        case LogLevel::kVerbose: text = "kVerbose"; break;
        // MISRA C++:2023 Rule 9.5.1: all enumerators covered; no default.
    }
    return out << ara::core::StringView(text);
}

LogStream &operator<<(LogStream &out,
                      const ara::core::ErrorCode &ec) noexcept
{
    // [SWS_LOG_00124]: show Domain().Name() + numeric code.
    ara::core::StringView domainName = ec.Domain().Name();
    char tmp[32U];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRId32,
                                static_cast<std::int32_t>(ec.Value()));
    out << domainName;
    if (n > 0)
    {
        out << ara::core::StringView(":");
        out << ara::core::StringView(tmp, static_cast<std::size_t>(n));
    }
    return out;
}


// ---------------------------------------------------------------------------
// operator<<(LogStream&, const LogStream&)
// Appends the payload buffer of 'other' into 'out'.
// Used by LoggingFramework::Log() to merge the context-level LogStream
// (created by Logger::WithLevel) with the caller-supplied message stream.
// ---------------------------------------------------------------------------

LogStream &operator<<(LogStream &out, const LogStream &other) noexcept
{
    // Only forward data when both streams are active and the source has data.
    if (out.IsActive() && other.IsActive() && !other.Buffer().empty())
    {
        try
        {
            // Append the source payload directly into the destination buffer.
            // CWE-120: std::string::append is bounded by the source size.
            out.buffer_.append(other.buffer_);
        }
        catch (...)
        {
            // [SWS_LOG_00002]: silent discard on allocation failure.
        }
    }
    return out;
}

} // namespace log
} // namespace ara
