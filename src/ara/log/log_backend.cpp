/**
 * @file log_backend.cpp
 * @brief Internal logging back-end implementation – C++14 compliant.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00001]  Initialize
 *   [SWS_LOG_00123]  Deinitialize
 *   [SWS_LOG_00002]  Silent discard
 *   [SWS_LOG_00005]  Logger created and stored inside framework
 *   [SWS_LOG_00095]  Ring-buffer for data-loss prevention
 *   [SWS_LOG_00228]  ConsoleSink  [SWS_LOG_00229] FileSink  [SWS_LOG_00231] NullSink
 *   [SWS_LOG_00253]  Default kWarn when no manifest entry
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ CON50-CPP | CWE-362 | CWE-667
 */

#include "./log_backend.h"
#include "./logger.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

namespace ara {
namespace log {
namespace internal {

// ---------------------------------------------------------------------------
// Timestamp helper (C++14: no structured bindings, no range-for on pairs)
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief Return a wall-clock timestamp string (ISO-8601 UTC).
 *
 * [SWS_LOG_00082/83]: in production this would use the ara::tsync time base.
 * CWE-676: only standard library facilities used.
 */
std::string GetTimestamp() noexcept
{
    try
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
    catch (...)
    {
        return std::string("0");
    }
}

/**
 * @brief Map LogLevel to a 5-character DLT-style tag.
 */
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
        // MISRA C++:2023 Rule 9.5.1: all enumerators handled; no default.
    }
    return "?????";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ConsoleSink [SWS_LOG_00228]
// ---------------------------------------------------------------------------

void ConsoleSink::Write(const LogRecord &record) noexcept
{
    try
    {
        std::ostringstream oss;
        oss << '[' << GetTimestamp() << ']'
            << '[' << LevelTag(record.level) << ']';

        if (!record.contextId.empty())
        {
            oss << '[' << record.contextId << ']';
        }
        if (!record.locationFile.empty())
        {
            oss << '[' << record.locationFile
                << ':' << record.locationLine << ']';
        }
        if (!record.tags.empty())
        {
            oss << "[tags=" << record.tags << ']';
        }
        oss << ' ' << record.payload;
        if (!record.metaPayload.empty())
        {
            oss << ' ' << record.metaPayload;
        }
        oss << '\n';

        std::cout << oss.str();
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// FileSink [SWS_LOG_00229]
// ---------------------------------------------------------------------------

FileSink::FileSink(ara::core::StringView filePath) noexcept
    : filePath_(filePath.data(), filePath.size())
{}

void FileSink::Write(const LogRecord &record) noexcept
{
    try
    {
        std::ofstream ofs(filePath_.c_str(), std::ios::app);
        if (!ofs.is_open())
        {
            return; // [SWS_LOG_00002]: silent discard.
        }

        ofs << '[' << GetTimestamp() << ']'
            << '[' << LevelTag(record.level) << ']';

        if (!record.contextId.empty())
        {
            ofs << '[' << record.contextId << ']';
        }
        if (!record.locationFile.empty())
        {
            ofs << '[' << record.locationFile
                << ':' << record.locationLine << ']';
        }
        if (!record.tags.empty())
        {
            ofs << "[tags=" << record.tags << ']';
        }

        ofs << ' ' << record.payload;
        if (!record.metaPayload.empty())
        {
            ofs << ' ' << record.metaPayload;
        }
        ofs << '\n';
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// LoggingFramework singleton
// ---------------------------------------------------------------------------

LoggingFramework &LoggingFramework::Instance() noexcept
{
    // C++11/14 magic-static – thread-safe by language standard.
    // CERT C++ DCL56-CPP: static local initialised once.
    static LoggingFramework instance;
    return instance;
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
        ringBuffer_.resize(kRingBufferSize);
    }
    catch (...)
    {
        // [SWS_LOG_00002]: graceful degradation without ring buffer.
    }
}

LoggingFramework::~LoggingFramework()
{
    // Flush residual records on process teardown. [SWS_LOG_00123]
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
// Initialize [SWS_LOG_00001]
// ---------------------------------------------------------------------------

void LoggingFramework::Initialize(ara::core::StringView appId,
                                  ara::core::StringView appDescription,
                                  LogMode               logMode,
                                  ara::core::StringView logFilePath) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);

        if (initialized_) { return; }

        appId_.assign(appId.data(), appId.size());
        appDescription_.assign(appDescription.data(), appDescription.size());

        const std::uint8_t modeVal =
            static_cast<std::uint8_t>(logMode);
        const std::uint8_t consoleBit =
            static_cast<std::uint8_t>(LogMode::kConsole);
        const std::uint8_t fileBit =
            static_cast<std::uint8_t>(LogMode::kFile);

        if (logMode == LogMode::kOff)
        {
            // C++14: unique_ptr from new (make_unique with private ctor
            // requires friendship; use raw new + immediate transfer).
            sinks_.push_back(std::unique_ptr<ILogSink>(new NullSink()));
        }
        else
        {
            if ((modeVal & consoleBit) != 0U)
            {
                sinks_.push_back(
                    std::unique_ptr<ILogSink>(new ConsoleSink()));
            }
            if ((modeVal & fileBit) != 0U)
            {
                sinks_.push_back(
                    std::unique_ptr<ILogSink>(new FileSink(logFilePath)));
            }
            if (sinks_.empty())
            {
                sinks_.push_back(
                    std::unique_ptr<ILogSink>(new ConsoleSink()));
            }
        }

        initialized_ = true;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// Deinitialize [SWS_LOG_00123]
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

        sinks_.clear();
        loggers_.clear();
        initialized_ = false;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: silent discard.
    }
}

// ---------------------------------------------------------------------------
// GetOrCreateLogger [SWS_LOG_00005], [SWS_LOG_00006]
// ---------------------------------------------------------------------------

Logger &LoggingFramework::GetOrCreateLogger(
    ara::core::StringView ctxId,
    ara::core::StringView ctxDescription,
    LogLevel              threshold) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);

        const std::string key(ctxId.data(), ctxId.size());

        typedef std::unordered_map<std::string,
                                   std::unique_ptr<Logger>> MapT;
        const MapT::iterator it = loggers_.find(key);
        if (it != loggers_.end())
        {
            return *(it->second);
        }

        // CERT C++ MEM55-CPP: ownership immediately transferred.
        // Private constructor accessed via friendship with LoggingFramework.
        std::unique_ptr<Logger> newLogger(
            new Logger(ctxId, ctxDescription, threshold));

        Logger &ref = *newLogger;
        loggers_.insert(std::make_pair(key, std::move(newLogger)));
        return ref;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: return static emergency logger on allocation fail.
        static Logger emergencyLogger(
            ara::core::StringView("EMRG"),
            ara::core::StringView("Emergency fallback logger"),
            LogLevel::kOff);
        return emergencyLogger;
    }
}

// ---------------------------------------------------------------------------
// Dispatch [SWS_LOG_00095]
// ---------------------------------------------------------------------------

void LoggingFramework::Dispatch(const LogRecord &record) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);

        if (!initialized_ || sinks_.empty()) { return; }

        // [SWS_LOG_00095]: buffer in ring, overwrite oldest when full.
        if (ringCount_ < kRingBufferSize)
        {
            ringBuffer_[ringHead_] = record;
            ringHead_ = (ringHead_ + 1U) % kRingBufferSize;
            ++ringCount_;
        }
        else
        {
            ringBuffer_[ringHead_] = record;
            ringHead_ = (ringHead_ + 1U) % kRingBufferSize;
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
        }

        // Drain ring to sinks.
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
    // Called under frameworkMutex_. CWE-667: lock held by caller.
    for (std::size_t i = 0U; i < sinks_.size(); ++i)
    {
        if (sinks_[i])
        {
            sinks_[i]->Write(record);
        }
    }
}

// ---------------------------------------------------------------------------
// RegisterConnectionStateHandler [SWS_LOG_00205]
// ---------------------------------------------------------------------------

void LoggingFramework::RegisterConnectionStateHandler(
    std::function<void(ClientState)> callback) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock(frameworkMutex_);
        connectionHandler_ = std::move(callback);
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
    // In a full implementation this consults the parsed machine manifest.
    // Per [SWS_LOG_00253]: fallback is LogLevel::kWarn.
    return LogLevel::kWarn;
}

} // namespace internal
} // namespace log
} // namespace ara
