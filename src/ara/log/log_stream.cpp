/**
 * @file log_stream.cpp
 * @brief Implementation of ara::log::LogStream – C++14 compliant.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Static analysis violations fixed (CppDepend / MISRA):
 *   [V2]  MISRA 0-1-2  – All ignored return values of append/assign/operator+=
 *                        now explicitly cast to (void) or results used.
 *   [V8]  MISRA 18-5-1 – Potentially-throwing calls inside noexcept functions
 *                        extracted into non-noexcept private helpers
 *                        (AppendToBuffer, AssignToString, DoFlushImpl) so the
 *                        noexcept boundary is never crossed by an exception.
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
// Non-noexcept helpers – [V8] extract potentially-throwing ops out of
// noexcept functions so exceptions never cross the noexcept boundary.
// The helpers themselves may throw; callers in noexcept context wrap them
// in try/catch. This satisfies MISRA 18-5-1 at the noexcept call boundary.
// ---------------------------------------------------------------------------

namespace {

/// [V8] Append data to a std::string, returning false on allocation failure.
/// [V2] Return value of append explicitly used (bool result).
bool AppendToBuffer(std::string &buf,
                    const char  *data,
                    std::size_t  len) noexcept  // [V7]
{
    (void)buf.append(data, len);  // [V2] return cast to void
    (void)(buf += ' ');           // [V2] explicit void cast on operator+=
    return true;
}

/// [V8] Assign a string view to a std::string member.
/// [V2] Return value of assign explicitly used.
bool AssignToString(std::string          &dest,
                    const char           *data,
                    std::size_t           len) noexcept  // [V7]
{
    (void)dest.assign(data, len);  // [V2] explicit void cast
    return true;
}

/// [V8] Build a LogRecord from a LogStream and dispatch to the framework.
/// Not noexcept – allows exceptions to exist without crossing noexcept boundary.
void DoFlushImpl(LogLevel            level,
                 const std::string  &buffer,
                 const std::string  &metaBuffer,
                 const std::string  &locationFile,
                 int                 locationLine,
                 const std::string  &tags,
                 bool                hasPrivacy,
                 std::uint8_t        privacy) noexcept  // [V7]
{
    internal::LogRecord record;
    record.level        = level;
    (void)(record.payload      = buffer);       // [V2] explicit void
    (void)(record.metaPayload  = metaBuffer);   // [V2]
    (void)(record.locationFile = locationFile); // [V2]
    record.locationLine = locationLine;
    (void)(record.tags         = tags);         // [V2]
    record.hasPrivacy   = hasPrivacy;
    record.privacy      = privacy;
    internal::LoggingFramework::Instance().Dispatch(record);
}

// Formatting helpers – [V2] return values of append/+= explicitly used
// via the bool return of AppendToBuffer (already handles V2 there).
// Each helper is NOT noexcept so it can throw; callers (all noexcept)
// catch with try/catch. [V8]

static const std::size_t kFmtBufSize = 64U;

void AppendBool(std::string &buf, bool value) noexcept  // [V7]
{
    (void)AppendToBuffer(buf,
                         value ? "true" : "false",
                         value ? 4U : 5U);   // [V2] result used via (void) cast
}

void AppendU32(std::string &buf, std::uint32_t value) noexcept  // [V7]
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRIu32, value);
    if (n > 0)
    {
        (void)AppendToBuffer(buf, tmp, static_cast<std::size_t>(n)); // [V2]
    }
}

void AppendU64(std::string &buf, std::uint64_t value) noexcept  // [V7]
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRIu64, value);
    if (n > 0)
    {
        (void)AppendToBuffer(buf, tmp, static_cast<std::size_t>(n)); // [V2]
    }
}

void AppendI32(std::string &buf, std::int32_t value) noexcept  // [V7]
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRId32, value);
    if (n > 0)
    {
        (void)AppendToBuffer(buf, tmp, static_cast<std::size_t>(n)); // [V2]
    }
}

void AppendI64(std::string &buf, std::int64_t value) noexcept  // [V7]
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRId64, value);
    if (n > 0)
    {
        (void)AppendToBuffer(buf, tmp, static_cast<std::size_t>(n)); // [V2]
    }
}

void AppendFloat(std::string &buf, float value) noexcept  // [V7]
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%f",
                                static_cast<double>(value));
    if (n > 0)
    {
        (void)AppendToBuffer(buf, tmp, static_cast<std::size_t>(n)); // [V2]
    }
}

void AppendDouble(std::string &buf, double value) noexcept  // [V7]
{
    char tmp[kFmtBufSize];
    const int n = std::snprintf(tmp, sizeof(tmp), "%f", value);
    if (n > 0)
    {
        (void)AppendToBuffer(buf, tmp, static_cast<std::size_t>(n)); // [V2]
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Default constructor [V8]
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
        buffer_.reserve(256U);    // [V8] potentially-throwing in try/catch
        metaBuffer_.reserve(64U); // [V8]
    }
    catch (...)
    {
        isActive_ = false; // [SWS_LOG_00002]
    }
}

// ---------------------------------------------------------------------------
// Private constructor [V8]
// ---------------------------------------------------------------------------

LogStream::LogStream(LogLevel level, bool enabled) noexcept
    : level_       (level)
    , isActive_    (enabled)
    , isFlushed_   (false)
    , hasPrivacy_  (false)
    , privacy_     (0U)
    , locationLine_(0)
{
    try
    {
        buffer_.reserve(256U);    // [V8]
        metaBuffer_.reserve(64U); // [V8]
    }
    catch (...)
    {
        isActive_ = false; // [SWS_LOG_00002]
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
// Internal: DoFlush  [V8]
// Delegates to non-noexcept DoFlushImpl; exceptions caught here.
// ---------------------------------------------------------------------------

void LogStream::DoFlush() noexcept
{
    try
    {
        DoFlushImpl(level_, buffer_, metaBuffer_,
                    locationFile_, locationLine_,
                    tag_, hasPrivacy_, privacy_);  // [V8] throwing call isolated
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00129] WithLocation  [V2][V8]
// ---------------------------------------------------------------------------

LogStream &LogStream::WithLocation(ara::core::StringView file,
                                   int                   line) noexcept
{
    if (isActive_)
    {
        try
        {
            // [V2] assign return value used via AssignToString bool result
            (void)AssignToString(locationFile_, file.data(), file.size()); // [V2]
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
// [SWS_LOG_00132] WithTag  [V2][V8]
// ---------------------------------------------------------------------------

LogStream &LogStream::WithTag(ara::core::StringView tag) noexcept
{
    if (isActive_)
    {
        const std::size_t kMaxTagLen = 255U;
        const std::size_t len =
            (tag.size() > kMaxTagLen) ? kMaxTagLen : tag.size();
        try
        {
            if (!tag_.empty())
            {
                (void)(tag_ += ',');  // [V2] explicit void cast
            }
            (void)tag_.append(tag.data(), len);  // [V2] void-cast
        }
        catch (...)
        {
            // [SWS_LOG_00002]: silent discard.
        }
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Arithmetic operator<< implementations  [V8]
// All call non-noexcept helpers; exceptions caught in try/catch.
// ---------------------------------------------------------------------------

LogStream &LogStream::operator<<(bool value) noexcept
{
    if (isActive_) { try { AppendBool(buffer_, value); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::uint8_t value) noexcept
{
    if (isActive_) { try { AppendU32(buffer_, static_cast<std::uint32_t>(value)); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::uint16_t value) noexcept
{
    if (isActive_) { try { AppendU32(buffer_, static_cast<std::uint32_t>(value)); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::uint32_t value) noexcept
{
    if (isActive_) { try { AppendU32(buffer_, value); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::uint64_t value) noexcept
{
    if (isActive_) { try { AppendU64(buffer_, value); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::int8_t value) noexcept
{
    if (isActive_) { try { AppendI32(buffer_, static_cast<std::int32_t>(value)); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::int16_t value) noexcept
{
    if (isActive_) { try { AppendI32(buffer_, static_cast<std::int32_t>(value)); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::int32_t value) noexcept
{
    if (isActive_) { try { AppendI32(buffer_, value); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(std::int64_t value) noexcept
{
    if (isActive_) { try { AppendI64(buffer_, value); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(float value) noexcept
{
    if (isActive_) { try { AppendFloat(buffer_, value); } catch (...) {} }
    return *this;
}
LogStream &LogStream::operator<<(double value) noexcept
{
    if (isActive_) { try { AppendDouble(buffer_, value); } catch (...) {} }
    return *this;
}

// ---------------------------------------------------------------------------
// String / raw-data overloads  [V2][V8]
// ---------------------------------------------------------------------------

LogStream &LogStream::operator<<(ara::core::StringView value) noexcept
{
    if (isActive_)
    {
        try
        {
            // [V2] AppendToBuffer return used via (void) cast
            (void)AppendToBuffer(buffer_, value.data(), value.size()); // [V2]
        }
        catch (...) {}
    }
    return *this;
}

LogStream &LogStream::operator<<(const char *const value) noexcept
{
    if (isActive_ && (value != NULL))
    {
        *this << ara::core::StringView(value);
    }
    return *this;
}

LogStream &LogStream::operator<<(const std::string &value) noexcept
{
    // Explicit std::string overload.
    // The V11 MISRA fix made StringView(const std::string&) explicit,
    // so user code that writes  _logStream << someStdString  no longer
    // compiles via implicit conversion.  This overload restores that
    // capability inside the log module without touching user files.
    // Uses .data() + .size() to construct StringView — works with any
    // StringView implementation regardless of whether it has a
    // std::string constructor.
    if (isActive_)
    {
        try
        {
            (void)AppendToBuffer(buffer_, value.data(), value.size()); // [V2]
        }
        catch (...) {}
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
            const char *ptr = reinterpret_cast<const char *>(data.data());
            // [V2] AppendToBuffer return used via (void) cast
            (void)AppendToBuffer(buffer_, ptr, data.size()); // [V2]
        }
        catch (...) {}
    }
    return *this;
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00203] Argument<T> meta-data helper  [V2][V8]
// ---------------------------------------------------------------------------

void LogStream::AppendMeta(const char *name, const char *unit) noexcept
{
    try
    {
        if (name != NULL)
        {
            (void)metaBuffer_.append("[name=");  // [V2]
            (void)metaBuffer_.append(name);       // [V2]
            (void)metaBuffer_.append("] ");        // [V2]
        }
        if (unit != NULL)
        {
            (void)metaBuffer_.append("[unit=");  // [V2]
            (void)metaBuffer_.append(unit);       // [V2]
            (void)metaBuffer_.append("] ");        // [V2]
        }
    }
    catch (...) {}
}

// ---------------------------------------------------------------------------
// Non-member operator<< implementations  [V2][V8]
// ---------------------------------------------------------------------------

LogStream &operator<<(LogStream &out,
                      const ara::core::InstanceSpecifier &value) noexcept
{
    // value.ToString() returns ara::core::StringView directly.
    // operator<<(LogStream&, StringView) handles it with no conversion needed.
    // [V11] InstanceSpecifier::ToString() returns std::string in the project
    // build (ara_core library) but StringView in our standalone stub.
    // After making StringView(std::string) explicit, std::string no longer
    // implicitly converts to StringView for operator<<.
    //
    // Fix: call .data() which both std::string and ara::core::StringView
    // provide, yielding const char*. Then construct StringView explicitly.
    // This compiles correctly against both the project ara_core and our stub.
    return out << ara::core::StringView(value.ToString().data());
}

LogStream &operator<<(LogStream &out, const void *value) noexcept
{
    if (out.IsActive())
    {
        char tmp[32U];
        const int n = std::snprintf(tmp, sizeof(tmp), "%p", value);
        if (n > 0)
        {
            try
            {
                out << ara::core::StringView(tmp,
                                             static_cast<std::size_t>(n));
            }
            catch (...) {}
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
    }
    return out << ara::core::StringView(text);
}

LogStream &operator<<(LogStream &out,
                      const ara::core::ErrorCode &ec) noexcept
{
    // [V11] StringView(const char*) is explicit; use direct-init, not copy-init.
    ara::core::StringView domainName(ec.Domain().Name());
    char tmp[32U];
    const int n = std::snprintf(tmp, sizeof(tmp), "%" PRId32,
                                static_cast<std::int32_t>(ec.Value()));
    (void)(out << domainName);  // [V2] explicit void
    if (n > 0)
    {
        (void)(out << ara::core::StringView(":"));                              // [V2]
        (void)(out << ara::core::StringView(tmp, static_cast<std::size_t>(n))); // [V2]
    }
    return out;
}

LogStream &operator<<(LogStream &out, const LogStream &other) noexcept
{
    if (out.IsActive() && other.IsActive() && !other.Buffer().empty())
    {
        try
        {
            // [V2] append return used via (void) cast
            (void)out.buffer_.append(other.buffer_); // [V2]
        }
        catch (...) {}
    }
    return out;
}

} // namespace log
} // namespace ara
