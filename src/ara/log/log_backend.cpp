/**
 * @file log_backend.cpp
 * @brief Implementation of the ara::log internal logging back-end.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00001]  Initialization via ara::core::Initialize
 *   [SWS_LOG_00123]  Shutdown via ara::core::Deinitialize
 *   [SWS_LOG_00002]  Silent discard of internal errors
 *   [SWS_LOG_00005]  CreateLogger stores instance inside framework
 *   [SWS_LOG_00006]  Logger created with ctxId, description, threshold
 *   [SWS_LOG_00228]  ConsoleSink
 *   [SWS_LOG_00229]  FileSink
 *   [SWS_LOG_00231]  NullSink (kOff)
 *   [SWS_LOG_00095]  Ring-buffer for data-loss prevention
 *   [SWS_LOG_00253]  Default log level from manifest (kWarn fallback)
 *
 * Coding standards:
 *   - MISRA C++:2023 Rule 6.2.2   – no global objects with non-trivial init
 *                                    (singleton initialised behind a mutex)
 *   - CERT C++ CON50-CPP          – mutex protection for all shared state
 *   - CERT C++ ERR50-CPP          – no exception propagation out of noexcept
 *   - CWE-362                     – race conditions prevented by locking
 *   - CWE-667                     – proper locking order maintained
 *   - ISO/SAE 21434               – no UB, bounded buffer operations
 */

#include "ara/log/internal/log_backend.h"
#include "ara/log/logger.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace ara {
namespace log {
namespace internal {

// ---------------------------------------------------------------------------
// Helper: timestamp string for console/file output
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief Return an ISO-8601 wall-clock timestamp string.
 *
 * Uses steady monotonic clock as a best-effort fallback.
 * Real implementations would use the ara::tsync time base per
 * [SWS_LOG_00082] / [SWS_LOG_00083].
 *
 * CWE-676: uses only standard library facilities.
 */
std::string GetTimestamp() noexcept
{
    try
    {
        const auto now{std::chrono::system_clock::now()};
        const auto t{std::chrono::system_clock::to_time_t(now)};
        struct tm tmBuf;
#if defined(_WIN32)
        (void)gmtime_s(&tmBuf, &t);
#else
        (void)gmtime_r(&t, &tmBuf);
#endif
        char buf[32U];
        const auto written{
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmBuf)};
        return (written > 0U) ? std::string{buf, written} : std::string{"0"};
    }
    catch (...)
    {
        return std::string{"0"};
    }
}

/**
 * @brief Map LogLevel to a fixed-width DLT-style abbreviation string.
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
        // MISRA C++:2023 Rule 9.5.1 – all enumerators handled; no default.
    }
    return "?????";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ConsoleSink – [SWS_LOG_00228]
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

        // Write atomically as a single string to avoid interleaving.
        // CERT C++ CON54-CPP: std::cout is not thread-safe; in production
        // this would go through a dedicated sink thread.
        std::cout << oss.str();
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard.
    }
}

// ---------------------------------------------------------------------------
// FileSink – [SWS_LOG_00229]
// ---------------------------------------------------------------------------

FileSink::FileSink(ara::core::StringView filePath) noexcept
    : filePath_{filePath}
{}

void FileSink::Write(const LogRecord &record) noexcept
{
    try
    {
        // Open file in append mode for each write to avoid keeping fd open
        // indefinitely. A production impl would hold an open file handle.
        std::ofstream ofs{filePath_, std::ios::app};
        if (!ofs.is_open())
        {
            // [SWS_LOG_00002]: Silent discard if file cannot be opened.
            return;
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
        // [SWS_LOG_00002]: Silent discard.
    }
}

// ---------------------------------------------------------------------------
// LoggingFramework – singleton
// ---------------------------------------------------------------------------

LoggingFramework &LoggingFramework::Instance() noexcept
{
    // MISRA C++:2023 – Meyers singleton; thread-safe by C++11 standard
    // (magic statics). No global constructor with side-effects.
    // CERT C++ DCL56-CPP: static local variable is initialised once.
    static LoggingFramework instance;
    return instance;
}

LoggingFramework::LoggingFramework() noexcept
    : clientState_{ClientState::kUnknown}
{
    try
    {
        ringBuffer_.resize(kRingBufferSize);
    }
    catch (...)
    {
        // [SWS_LOG_00002]: If ring buffer allocation fails, the framework
        // degrades gracefully (dispatch goes directly to sinks without
        // buffering).
    }
}

LoggingFramework::~LoggingFramework()
{
    // Flush residual records on destruction (process shutdown path).
    // [SWS_LOG_00123]
    try
    {
        const std::lock_guard<std::mutex> lock{frameworkMutex_};
        // Drain ring buffer.
        while (ringCount_ > 0U)
        {
            const LogRecord &rec{ringBuffer_[ringTail_]};
            DispatchToSinks(rec);
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
            --ringCount_;
        }
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard.
    }
}

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void LoggingFramework::Initialize(ara::core::StringView appId,
                                  ara::core::StringView appDescription,
                                  LogMode               logMode,
                                  ara::core::StringView logFilePath) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock{frameworkMutex_};

        if (initialized_)
        {
            return; // Idempotent.
        }

        appId_.assign(appId.data(), appId.size());
        appDescription_.assign(appDescription.data(), appDescription.size());

        // Configure sinks based on logMode.
        // logMode is treated as a bitmask; multiple sinks can be active.
        // [SWS_LOG_00228–00231]
        if (logMode == LogMode::kOff)
        {
            sinks_.push_back(std::make_unique<NullSink>());
        }
        else
        {
            const auto modeVal{static_cast<std::uint8_t>(logMode)};
            const auto consoleBit{static_cast<std::uint8_t>(LogMode::kConsole)};
            const auto fileBit   {static_cast<std::uint8_t>(LogMode::kFile)};

            if ((modeVal & consoleBit) != 0U)
            {
                sinks_.push_back(std::make_unique<ConsoleSink>());
            }
            if ((modeVal & fileBit) != 0U)
            {
                sinks_.push_back(
                    std::make_unique<FileSink>(logFilePath));
            }
            if (sinks_.empty())
            {
                // Default to console if no valid mode bit set.
                sinks_.push_back(std::make_unique<ConsoleSink>());
            }
        }

        initialized_ = true;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard.
    }
}

// ---------------------------------------------------------------------------
// Deinitialize – [SWS_LOG_00123]
// ---------------------------------------------------------------------------

void LoggingFramework::Deinitialize() noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock{frameworkMutex_};

        if (!initialized_)
        {
            return;
        }

        // Drain the ring buffer before clearing sinks.
        while (ringCount_ > 0U)
        {
            const LogRecord &rec{ringBuffer_[ringTail_]};
            DispatchToSinks(rec);
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
            --ringCount_;
        }

        sinks_.clear();
        loggers_.clear();
        initialized_ = false;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard.
    }
}

// ---------------------------------------------------------------------------
// GetOrCreateLogger – [SWS_LOG_00005], [SWS_LOG_00006]
// ---------------------------------------------------------------------------

Logger &LoggingFramework::GetOrCreateLogger(
    ara::core::StringView ctxId,
    ara::core::StringView ctxDescription,
    LogLevel              threshold) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock{frameworkMutex_};

        // [SWS_LOG_00005]: Logger instance created internally; reference
        // returned to application.
        const std::string key{ctxId.data(), ctxId.size()};
        auto it{loggers_.find(key)};
        if (it != loggers_.end())
        {
            return *(it->second);
        }

        // Create a new Logger.  Private constructor accessed via friendship.
        // std::make_unique cannot be used with a private constructor, so we
        // use the unique_ptr constructor with new directly.
        // CERT C++ MEM55-CPP: ownership immediately transferred to unique_ptr.
        std::unique_ptr<Logger> newLogger{
            new Logger{ctxId, ctxDescription, threshold}};

        Logger &ref{*newLogger};
        loggers_.emplace(key, std::move(newLogger));
        return ref;
    }
    catch (...)
    {
        // [SWS_LOG_00002]: If allocation fails, return a static emergency
        // logger that silently discards all messages.
        static Logger emergencyLogger{
            ara::core::StringView{"EMRG"},
            ara::core::StringView{"Emergency fallback logger"},
            LogLevel::kOff};
        return emergencyLogger;
    }
}

// ---------------------------------------------------------------------------
// Dispatch – [SWS_LOG_00095]
// ---------------------------------------------------------------------------

void LoggingFramework::Dispatch(const LogRecord &record) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock{frameworkMutex_};

        if (!initialized_ || sinks_.empty())
        {
            // [SWS_LOG_00002]: Silently discard when not initialised.
            return;
        }

        // [SWS_LOG_00095]: Buffer record in the ring buffer.
        // If the ring buffer is full, overwrite the oldest entry (best-effort
        // data-loss prevention – oldest records sacrificed over newest).
        if (ringCount_ < kRingBufferSize)
        {
            ringBuffer_[ringHead_] = record;
            ringHead_ = (ringHead_ + 1U) % kRingBufferSize;
            ++ringCount_;
        }
        else
        {
            // Buffer full: overwrite oldest.
            ringBuffer_[ringHead_] = record;
            ringHead_ = (ringHead_ + 1U) % kRingBufferSize;
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
            // ringCount_ stays at kRingBufferSize.
        }

        // Drain the ring buffer to sinks.
        while (ringCount_ > 0U)
        {
            const LogRecord &rec{ringBuffer_[ringTail_]};
            DispatchToSinks(rec);
            ringTail_ = (ringTail_ + 1U) % kRingBufferSize;
            --ringCount_;
        }
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard.
    }
}

void LoggingFramework::DispatchToSinks(const LogRecord &record) noexcept
{
    // Called under frameworkMutex_. CWE-667: lock already held by caller.
    for (auto &sink : sinks_)
    {
        if (sink)
        {
            sink->Write(record);
        }
    }
}

// ---------------------------------------------------------------------------
// RegisterConnectionStateHandler – [SWS_LOG_00205]
// ---------------------------------------------------------------------------

void LoggingFramework::RegisterConnectionStateHandler(
    std::function<void(ClientState)> callback) noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock{frameworkMutex_};
        connectionHandler_ = std::move(callback);
    }
    catch (...)
    {
        // [SWS_LOG_00002]: Silent discard.
    }
}

// ---------------------------------------------------------------------------
// IsInitialized
// ---------------------------------------------------------------------------

bool LoggingFramework::IsInitialized() const noexcept
{
    try
    {
        const std::lock_guard<std::mutex> lock{frameworkMutex_};
        return initialized_;
    }
    catch (...)
    {
        return false;
    }
}

// ---------------------------------------------------------------------------
// GetManifestLogLevel – [SWS_LOG_00253]
// ---------------------------------------------------------------------------

LogLevel LoggingFramework::GetManifestLogLevel(
    ara::core::StringView /*ctxId*/) const noexcept
{
    // In a full implementation this would consult the parsed machine manifest.
    // Per [SWS_LOG_00253]: if no entry found, return LogLevel::kWarn.
    return LogLevel::kWarn;
}

} // namespace internal
} // namespace log
} // namespace ara
