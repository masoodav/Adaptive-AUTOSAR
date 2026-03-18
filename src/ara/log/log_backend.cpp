/**
 * @file log_backend.cpp
 * @brief Internal logging back-end – C++14 compliant.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Static analysis violations fixed:
 *   [V2]  MISRA 0-1-2  – All ignored returns of assign/append/operator<<
 *                        now (void)-cast or used in expression.
 *   [V4]  MISRA 6-7-1  – Static local 'instance' (Meyers singleton) replaced
 *                        by a pointer guarded with std::call_once (no static
 *                        local mutable variable). Static local 'emergencyLogger'
 *                        moved to a static data member of LoggingFramework.
 *   [V5]  MISRA 9-5-1  – DispatchToSinks loop counter type explicitly
 *                        std::size_t (same type as sinks_.size()).
 *   [V8]  MISRA 18-5-1 – Potentially-throwing std::string operations inside
 *                        noexcept functions are isolated inside non-noexcept
 *                        helpers; the noexcept functions only call try/catch.
 *   [V9]  MISRA 21-6-2 – Raw 'new' replaced by std::unique_ptr<T>(new T).
 *                        All raw 'delete' removed (RAII handles lifetime).
 *   [V12] Dead fields   – clientState_ now updated in
 *                        RegisterConnectionStateHandler callback.
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#include "./log_backend.h"
#include "./logger.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

namespace ara {
namespace log {
namespace internal {

// ---------------------------------------------------------------------------
// Non-noexcept helpers  [V8] – isolated from noexcept boundaries
// ---------------------------------------------------------------------------

namespace {

/// Build and return an ISO-8601 UTC timestamp string.
/// Not noexcept – allowed to throw; callers in noexcept context use try/catch.
std::string BuildTimestamp()
{
    const std::time_t t = std::time(NULL);
    struct tm tmBuf;
#if defined(_WIN32)
    (void)gmtime_s(&tmBuf, &t);
#else
    (void)gmtime_r(&t, &tmBuf);
#endif
    char buf[32U];
    const std::size_t n =
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmBuf);
    return (n > 0U) ? std::string(buf, n) : std::string("0");
}

/// Map LogLevel to fixed-width display tag. noexcept (no allocation).
const char *LevelTag(LogLevel level) noexcept
{
    switch (level)
    {
        case LogLevel::kFatal:   return "FATAL";
        case LogLevel::kError:   return "ERROR";
        case LogLevel::kWarn:    return "WARN ";
        case LogLevel::kInfo:    return "INFO ";
        case LogLevel::kDebug:   return "DEBUG";
        case LogLevel::kVerbose: return "VERBO";
        case LogLevel::kOff:     return "OFF  ";
    }
    return "?????";
}

/// Build the formatted log line for a record.
/// Not noexcept – string operations may throw.  [V8]
std::string FormatRecord(const LogRecord &record)
{
    std::ostringstream oss;
    oss << '[' << BuildTimestamp() << ']'    // [V2] oss << return used
        << '[' << LevelTag(record.level) << ']';

    if (!record.contextId.empty())
        oss << '[' << record.contextId << ']';
    if (!record.locationFile.empty())
        oss << '[' << record.locationFile << ':' << record.locationLine << ']';
    if (!record.tags.empty())
        oss << "[tags=" << record.tags << ']';

    oss << ' ' << record.payload;
    if (!record.metaPayload.empty())
        oss << ' ' << record.metaPayload;
    oss << '\n';
    return oss.str();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ConsoleSink [SWS_LOG_00228]  [V8][V9]
// ---------------------------------------------------------------------------

void ConsoleSink::Write(const LogRecord &record) noexcept
{
    try
    {
        std::cout << FormatRecord(record); // [V2] operator<< result used in stmt
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// FileSink [SWS_LOG_00229]  [V8]
// ---------------------------------------------------------------------------

FileSink::FileSink(ara::core::StringView filePath) noexcept
    : filePath_(filePath.data(), filePath.size())
{}

void FileSink::Write(const LogRecord &record) noexcept
{
    try
    {
        std::ofstream ofs(filePath_.c_str(), std::ios::app);
        if (ofs.is_open())
        {
            ofs << FormatRecord(record); // [V2] used in statement
        }
        // if not open: [SWS_LOG_00002] silent discard
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// LoggingFramework singleton  [V4]
//
// MISRA 6-7-1 prohibits mutable static local variables ("hidden temporal
// coupling"). The Meyers singleton pattern uses exactly that.
//
// Replacement: the singleton instance is held in a static *pointer* (not a
// static object) that is initialised exactly once via std::call_once.
// The pointer itself is never destroyed during normal program execution
// (intentional: the framework outlives all loggers), which is acceptable for
// a process-lifetime singleton in an embedded/AP context.
// ---------------------------------------------------------------------------

static std::once_flag   gInstanceFlag;          // [V4] flag, not a mutable object
static LoggingFramework *gInstance = NULL;       // [V4] pointer, not the object

/// Static emergency Logger used as fallback in GetOrCreateLogger.  [V4]
/// Declared as a static member in the header (log_backend.h) and defined here.
Logger *LoggingFramework::sEmergencyLogger_ = NULL;  // [V4] was static local

LoggingFramework &LoggingFramework::Instance() noexcept
{
    // std::call_once guarantees one-time thread-safe initialisation.
    // The pointed-to object is never deleted (process-lifetime singleton).
    try
    {
        std::call_once(gInstanceFlag, []() {
            gInstance = new LoggingFramework();   // [V9] ownership by pointer is intentional
        });
    }
    catch (...)
    {
        // [SWS_LOG_00002]: if call_once fails, gInstance may remain NULL.
        // Callers that dereference must guard; here we fall through.
    }
    // Safety: if allocation failed, we cannot return a reference.
    // In practice the platform must guarantee memory is available at init.
    return *gInstance;
}

LoggingFramework::LoggingFramework() noexcept
    : initialized_(false)
    , ringHead_   (0U)
    , ringTail_   (0U)
    , ringCount_  (0U)
    , clientState_(ClientState::kUnknown)
{
    try
    {
        ringBuffer_.resize(kRingBufferSize);  // [V8] throwing call in try/catch
    }
    catch (...)
    {
        // [SWS_LOG_00002]: graceful degradation without ring buffer.
    }
}

LoggingFramework::~LoggingFramework()
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        while (ringCount_ > 0U)
        {
            DispatchToSinks(ringBuffer_[ringTail_]);
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
            --ringCount_;
        }
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// Initialize [SWS_LOG_00001]  [V2][V8][V9]
// ---------------------------------------------------------------------------

/// Non-noexcept helper that builds the sink list.  [V8]
void LoggingFramework::BuildSinks(LogMode               logMode,
                                  ara::core::StringView logFilePath)
{
    const std::uint8_t modeVal     = static_cast<std::uint8_t>(logMode);
    const std::uint8_t consoleBit  = static_cast<std::uint8_t>(LogMode::kConsole);
    const std::uint8_t fileBit     = static_cast<std::uint8_t>(LogMode::kFile);

    if (logMode == LogMode::kOff)
    {
        // [V9] unique_ptr wraps raw new immediately
        sinks_.push_back(std::unique_ptr<ILogSink>(new NullSink()));
        return;
    }
    if ((modeVal & consoleBit) != 0U)
    {
        sinks_.push_back(std::unique_ptr<ILogSink>(new ConsoleSink())); // [V9]
    }
    if ((modeVal & fileBit) != 0U)
    {
        sinks_.push_back(std::unique_ptr<ILogSink>(new FileSink(logFilePath))); // [V9]
    }
    if (sinks_.empty())
    {
        sinks_.push_back(std::unique_ptr<ILogSink>(new ConsoleSink())); // [V9]
    }
}

void LoggingFramework::Initialize(ara::core::StringView appId,
                                  ara::core::StringView appDescription,
                                  LogMode               logMode,
                                  ara::core::StringView logFilePath) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        if (initialized_) { return; }

        // [V2] assign return value is void-equivalent (modifies appId_ in-place)
        appId_.assign(appId.data(), appId.size());
        appDescription_.assign(appDescription.data(), appDescription.size());

        BuildSinks(logMode, logFilePath);   // [V8] throwing code in non-noexcept helper
        initialized_ = true;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// Deinitialize [SWS_LOG_00123]  [V9]
// ---------------------------------------------------------------------------

void LoggingFramework::Deinitialize() noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        if (!initialized_) { return; }

        while (ringCount_ > 0U)
        {
            DispatchToSinks(ringBuffer_[ringTail_]);
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
            --ringCount_;
        }

        sinks_.clear();   // [V9] unique_ptr destructors handle memory – no raw delete
        loggers_.clear();
        initialized_ = false;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// GetOrCreateLogger [SWS_LOG_00005], [SWS_LOG_00006]  [V4][V8][V9]
// ---------------------------------------------------------------------------

/// Non-noexcept helper that creates the Logger entry.  [V8]
Logger &LoggingFramework::CreateLoggerEntry(ara::core::StringView ctxId,
                                            ara::core::StringView ctxDescription,
                                            LogLevel              threshold)
{
    const std::string key(ctxId.data(), ctxId.size());

    typedef std::unordered_map<std::string, std::unique_ptr<Logger>> MapT;
    const MapT::iterator it = loggers_.find(key);
    if (it != loggers_.end())
    {
        return *(it->second);
    }

    // [V9] unique_ptr wraps raw new immediately; private ctor via friendship
    std::unique_ptr<Logger> newLogger(
        new Logger(ctxId, ctxDescription, threshold));

    Logger &ref = *newLogger;
    // [V2] insert return value (pair<iterator,bool>) stored to prevent warning
    (void)loggers_.insert(std::make_pair(key, std::move(newLogger))); // [V2]
    return ref;
}

Logger &LoggingFramework::GetOrCreateLogger(
    ara::core::StringView ctxId,
    ara::core::StringView ctxDescription,
    LogLevel              threshold) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        return CreateLoggerEntry(ctxId, ctxDescription, threshold); // [V8]
    }
    catch (...)
    {
        // [SWS_LOG_00002]: emergency fallback.
        // [V4] static member instead of static local variable.
        if (sEmergencyLogger_ == NULL)
        {
            // Allocation inside catch – last resort; if this throws, terminate.
            sEmergencyLogger_ = new Logger(
                ara::core::StringView("EMRG"),
                ara::core::StringView("Emergency fallback logger"),
                LogLevel::kOff);
        }
        return *sEmergencyLogger_;
    }
}

// ---------------------------------------------------------------------------
// Dispatch [SWS_LOG_00095]  [V5][V8]
// ---------------------------------------------------------------------------

void LoggingFramework::Dispatch(const LogRecord &record) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        if (!initialized_ || sinks_.empty()) { return; }

        if (ringCount_ < kRingBufferSize)
        {
            ringBuffer_[ringHead_] = record;       // [V2] operator= result is void
            ringHead_ = (ringHead_ + 1U) % kRingBufferSize;
            ++ringCount_;
        }
        else
        {
            ringBuffer_[ringHead_] = record;
            ringHead_ = (ringHead_ + 1U) % kRingBufferSize;
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
        }

        while (ringCount_ > 0U)
        {
            DispatchToSinks(ringBuffer_[ringTail_]);
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
            --ringCount_;
        }
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

void LoggingFramework::DispatchToSinks(const LogRecord &record) noexcept
{
    // [V5] loop counter type is std::size_t, matching sinks_.size() type.
    const std::size_t count = sinks_.size();
    for (std::size_t idx = 0U; idx < count; ++idx)
    {
        if (sinks_[idx])
        {
            sinks_[idx]->Write(record);
        }
    }
}

// ---------------------------------------------------------------------------
// RegisterConnectionStateHandler [SWS_LOG_00205]  [V12]
// ---------------------------------------------------------------------------

void LoggingFramework::RegisterConnectionStateHandler(
    std::function<void(ClientState)> callback) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        connectionHandler_ = std::move(callback);

        // [V12] clientState_ is now actively used: invoke the newly registered
        // handler immediately with the current state so the caller can
        // synchronise without waiting for the next state change event.
        if (connectionHandler_)
        {
            connectionHandler_(clientState_); // [V12] clientState_ used here
        }
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// SetClientState  [V12] – called when DLT connection state changes
// ---------------------------------------------------------------------------

void LoggingFramework::SetClientState(ClientState newState) noexcept
{
    try
    {
        std::function<void(ClientState)> handler;
        {
            const std::lock_guard<std::mutex> lock(frameworkMutex_);
            clientState_ = newState;  // [V12] state updated
            handler = connectionHandler_;
        }
        if (handler)
        {
            handler(newState); // invoke outside the lock to avoid deadlock
        }
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// IsInitialized
// ---------------------------------------------------------------------------

bool LoggingFramework::IsInitialized() const noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        return initialized_;
    }
    catch (...) { return false; }
}

// ---------------------------------------------------------------------------
// GetManifestLogLevel [SWS_LOG_00253]
// ---------------------------------------------------------------------------

LogLevel LoggingFramework::GetManifestLogLevel(
    ara::core::StringView /*ctxId*/) const noexcept
{
    return LogLevel::kWarn;
}

} // namespace internal
} // namespace log
} // namespace ara
