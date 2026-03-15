/**
 * @file log_backend.h
 * @brief Internal logging back-end: sink interface and logger registry.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 * C++14 compliant.
 *
 * Traceability:
 *   [SWS_LOG_00001]  Framework initialization
 *   [SWS_LOG_00123]  Framework shutdown
 *   [SWS_LOG_00002]  Silent discard on errors
 *   [SWS_LOG_00228]  Console sink
 *   [SWS_LOG_00229]  File sink
 *   [SWS_LOG_00230]  Remote (DLT) sink
 *   [SWS_LOG_00231]  Off (null) sink
 *   [SWS_LOG_00095]  Internal buffering to prevent data loss
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_LOG_INTERNAL_LOG_BACKEND_H_
#define ARA_LOG_INTERNAL_LOG_BACKEND_H_

#include "./common.h"
#include "./string_view.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ara {
namespace log {

class Logger;

namespace internal {

// LogMode is defined in ara/log/common.h (public namespace).
// Internal code uses ara::log::LogMode directly.

// ---------------------------------------------------------------------------
// LogRecord
// ---------------------------------------------------------------------------

/**
 * @brief Complete payload of one log message passed to sinks.
 */
struct LogRecord final
{
    LogLevel     level;
    std::string  contextId;
    std::string  payload;
    std::string  metaPayload;
    std::string  locationFile;
    int          locationLine;
    std::string  tags;
    bool         hasPrivacy;
    std::uint8_t privacy;

    LogRecord()
        : level(LogLevel::kOff)
        , locationLine(0)
        , hasPrivacy(false)
        , privacy(0U)
    {}
};

// ---------------------------------------------------------------------------
// ILogSink
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for a logging output sink.
 * [SWS_LOG_00002]: implementations SHALL NOT throw.
 */
class ILogSink
{
public:
    ILogSink()                            = default;
    virtual ~ILogSink()                   = default;
    ILogSink(const ILogSink &)            = delete;
    ILogSink &operator=(const ILogSink &) = delete;
    ILogSink(ILogSink &&)                 = delete;
    ILogSink &operator=(ILogSink &&)      = delete;

    virtual void Write(const LogRecord &record) noexcept = 0;
};

// ---------------------------------------------------------------------------
// ConsoleSink [SWS_LOG_00228]
// ---------------------------------------------------------------------------
class ConsoleSink final : public ILogSink
{
public:
    ConsoleSink()  = default;
    ~ConsoleSink() = default;
    void Write(const LogRecord &record) noexcept override;
};

// ---------------------------------------------------------------------------
// FileSink [SWS_LOG_00229]
// ---------------------------------------------------------------------------
class FileSink final : public ILogSink
{
public:
    explicit FileSink(ara::core::StringView filePath) noexcept;
    ~FileSink() = default;
    void Write(const LogRecord &record) noexcept override;
private:
    std::string filePath_;
};

// ---------------------------------------------------------------------------
// NullSink [SWS_LOG_00231]
// ---------------------------------------------------------------------------
class NullSink final : public ILogSink
{
public:
    NullSink()  = default;
    ~NullSink() = default;
    void Write(const LogRecord & /*record*/) noexcept override {}
};

// ---------------------------------------------------------------------------
// LoggingFramework – singleton
// ---------------------------------------------------------------------------

/**
 * @brief Central singleton managing Logger instances, sinks, and dispatch.
 *
 * Thread-safety: all public methods protected by internal mutex.
 * CERT C++ CON50-CPP | CWE-362 | CWE-667
 */
class LoggingFramework final
{
public:
    static LoggingFramework &Instance() noexcept;

    LoggingFramework(const LoggingFramework &)            = delete;
    LoggingFramework &operator=(const LoggingFramework &) = delete;
    LoggingFramework(LoggingFramework &&)                 = delete;
    LoggingFramework &operator=(LoggingFramework &&)      = delete;

    // [SWS_LOG_00001]
    void Initialize(ara::core::StringView appId,
                    ara::core::StringView appDescription,
                    LogMode               logMode,
                    ara::core::StringView logFilePath) noexcept;

    // [SWS_LOG_00123]
    void Deinitialize() noexcept;

    // [SWS_LOG_00005], [SWS_LOG_00006]
    Logger &GetOrCreateLogger(ara::core::StringView ctxId,
                              ara::core::StringView ctxDescription,
                              LogLevel              threshold) noexcept;

    // [SWS_LOG_00095]
    void Dispatch(const LogRecord &record) noexcept;

    // [SWS_LOG_00205]
    void RegisterConnectionStateHandler(
        std::function<void(ClientState)> callback) noexcept;

    bool    IsInitialized() const noexcept;

    // [SWS_LOG_00253]
    LogLevel GetManifestLogLevel(ara::core::StringView ctxId) const noexcept;

private:
    LoggingFramework() noexcept;
    ~LoggingFramework();

    void DispatchToSinks(const LogRecord &record) noexcept;

    mutable std::mutex frameworkMutex_;

    std::string appId_;
    std::string appDescription_;
    bool        initialized_;

    std::unordered_map<std::string,
                       std::unique_ptr<Logger>> loggers_;

    std::vector<std::unique_ptr<ILogSink>> sinks_;

    // Ring-buffer for [SWS_LOG_00095] data-loss prevention.
    static const std::size_t kRingBufferSize = 256U;
    std::vector<LogRecord>   ringBuffer_;
    std::size_t              ringHead_;
    std::size_t              ringTail_;
    std::size_t              ringCount_;

    std::function<void(ClientState)> connectionHandler_;
    ClientState                      clientState_;
};

} // namespace internal
} // namespace log
} // namespace ara

#endif // ARA_LOG_INTERNAL_LOG_BACKEND_H_
