/**
 * @file log_backend.h
 * @brief Internal logging back-end: sink interface and logger registry.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00001]  Logging framework initialization via ara::core::Initialize
 *   [SWS_LOG_00123]  Logging framework shutdown via ara::core::Deinitialize
 *   [SWS_LOG_00002]  Silent discard on errors inside framework
 *   [SWS_LOG_00228]  Console sink
 *   [SWS_LOG_00229]  File sink
 *   [SWS_LOG_00230]  Remote (DLT) sink
 *   [SWS_LOG_00231]  Off (null) sink
 *   [SWS_LOG_00095]  Internal buffering to prevent data loss
 *
 * Coding standards:
 *   - MISRA C++:2023 Rule 6.2.2   – no global objects with non-trivial init
 *   - CERT C++ CON50-CPP          – mutex-protected shared state
 *   - CWE-667                     – improper locking avoided
 *   - ISO/SAE 21434               – no undefined behavior in shared data paths
 */

#ifndef ARA_LOG_INTERNAL_LOG_BACKEND_H_
#define ARA_LOG_INTERNAL_LOG_BACKEND_H_

#include "ara/log/common.h"
#include "ara/core/string_view.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace ara {
namespace log {

// Forward declaration
class Logger;

namespace internal {

// ---------------------------------------------------------------------------
// LogMode enumeration
// Matches manifest DltLogSink.category.
// [SWS_LOG_00228..00231]
// ---------------------------------------------------------------------------

/**
 * @brief Destination of log message output.
 */
enum class LogMode : std::uint8_t
{
    kRemote  = 0x01U, ///< [SWS_LOG_00230] DLT network/remote.
    kFile    = 0x02U, ///< [SWS_LOG_00229] Local file system.
    kConsole = 0x04U, ///< [SWS_LOG_00228] Console/stdout.
    kOff     = 0x00U  ///< [SWS_LOG_00231] Logging disabled.
};

// ---------------------------------------------------------------------------
// LogRecord – complete message payload passed to sinks
// ---------------------------------------------------------------------------

/**
 * @brief Encapsulates all data of a single log message.
 */
struct LogRecord final
{
    LogLevel    level;
    std::string contextId;
    std::string payload;
    std::string metaPayload;
    std::string locationFile;
    int         locationLine {0};
    std::string tags;
    bool        hasPrivacy {false};
    std::uint8_t privacy   {0U};
};

// ---------------------------------------------------------------------------
// ILogSink – abstract sink interface
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for a logging output sink.
 *
 * [SWS_LOG_00002]: Implementations SHALL NOT throw or signal errors
 * to the caller.
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

    /**
     * @brief Dispatch a log record to this sink.
     *
     * @param record  The completed log message.
     */
    virtual void Write(const LogRecord &record) noexcept = 0;
};

// ---------------------------------------------------------------------------
// ConsoleSink – [SWS_LOG_00228]
// ---------------------------------------------------------------------------

/**
 * @brief Writes log records to stdout.
 */
class ConsoleSink final : public ILogSink
{
public:
    ConsoleSink()  = default;
    ~ConsoleSink() = default;

    void Write(const LogRecord &record) noexcept override;

private:
    static const char *LogLevelToString(LogLevel level) noexcept;
};

// ---------------------------------------------------------------------------
// FileSink – [SWS_LOG_00229]
// ---------------------------------------------------------------------------

/**
 * @brief Writes log records to a local file.
 */
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
// NullSink – [SWS_LOG_00231] (kOff)
// ---------------------------------------------------------------------------

/**
 * @brief Discards all log records silently.
 */
class NullSink final : public ILogSink
{
public:
    NullSink()  = default;
    ~NullSink() = default;

    void Write(const LogRecord & /*record*/) noexcept override {}
};

// ---------------------------------------------------------------------------
// LoggingFramework – singleton registry
// ---------------------------------------------------------------------------

/**
 * @brief Central singleton managing Logger instances and sinks.
 *
 * Provides:
 *   - Logger creation and look-up.
 *   - Sink registration and message dispatch.
 *   - Connection-state handler management.
 *   - Internal ring-buffer for data-loss prevention [SWS_LOG_00095].
 *
 * Thread-safety: All public methods are protected by an internal mutex
 * to comply with CERT C++ CON50-CPP.
 */
class LoggingFramework final
{
public:
    // ------------------------------------------------------------------------
    // Singleton access
    // ------------------------------------------------------------------------

    /**
     * @brief Return the global LoggingFramework instance.
     *
     * [SWS_LOG_00001]: Initialization is triggered by ara::core::Initialize().
     * The singleton uses lazy initialization protected by a mutex.
     */
    static LoggingFramework &Instance() noexcept;

    // Not copyable/movable.
    LoggingFramework(const LoggingFramework &)            = delete;
    LoggingFramework &operator=(const LoggingFramework &) = delete;
    LoggingFramework(LoggingFramework &&)                 = delete;
    LoggingFramework &operator=(LoggingFramework &&)      = delete;

    // ------------------------------------------------------------------------
    // Initialization / shutdown
    // ------------------------------------------------------------------------

    /**
     * @brief Initialize the framework with application identity.
     *
     * [SWS_LOG_00001]
     *
     * @param appId          Application ID string.
     * @param appDescription Human-readable application description.
     * @param logMode        Default log output mode.
     * @param logFilePath    Path for file logging (relevant when kFile is set).
     */
    void Initialize(ara::core::StringView appId,
                    ara::core::StringView appDescription,
                    LogMode               logMode,
                    ara::core::StringView logFilePath) noexcept;

    /**
     * @brief Shut down the framework, flushing all pending messages.
     *
     * [SWS_LOG_00123]
     */
    void Deinitialize() noexcept;

    // ------------------------------------------------------------------------
    // Logger registry
    // ------------------------------------------------------------------------

    /**
     * @brief Retrieve or create a Logger.
     *
     * If a Logger for ctxId already exists, the existing instance is returned.
     * [SWS_LOG_00005], [SWS_LOG_00006]
     *
     * @param ctxId          Context ID.
     * @param ctxDescription Context description.
     * @param threshold      Initial log level threshold.
     * @return Reference to the Logger instance owned by the framework.
     */
    Logger &GetOrCreateLogger(ara::core::StringView ctxId,
                              ara::core::StringView ctxDescription,
                              LogLevel              threshold) noexcept;

    // ------------------------------------------------------------------------
    // Message dispatch
    // ------------------------------------------------------------------------

    /**
     * @brief Dispatch a completed log record to all registered sinks.
     *
     * [SWS_LOG_00095]: Records are queued in an internal ring buffer before
     * dispatch to prevent data loss under high load.
     *
     * @param record  The log message to dispatch.
     */
    void Dispatch(const LogRecord &record) noexcept;

    // ------------------------------------------------------------------------
    // Connection state handler
    // ------------------------------------------------------------------------

    /**
     * @brief Register a connection-state change callback.
     *
     * [SWS_LOG_00205]
     */
    void RegisterConnectionStateHandler(
        std::function<void(ClientState)> callback) noexcept;

    /**
     * @brief Query whether the framework has been initialized.
     */
    bool IsInitialized() const noexcept;

    /**
     * @brief Return the default log level from the manifest for a context.
     *
     * [SWS_LOG_00253]: Returns kWarn if no manifest entry exists.
     *
     * @param ctxId  Context ID to look up.
     * @return LogLevel from manifest, or kWarn if not found.
     */
    LogLevel GetManifestLogLevel(ara::core::StringView ctxId) const noexcept;

private:
    // Private constructor for singleton pattern.
    LoggingFramework() noexcept;
    ~LoggingFramework();

    // Internal sink list and dispatch.
    void DispatchToSinks(const LogRecord &record) noexcept;

    // -----------------------------------------------------------------------
    // Member data
    // -----------------------------------------------------------------------

    mutable std::mutex frameworkMutex_;

    std::string appId_;
    std::string appDescription_;
    bool        initialized_ {false};

    /// Map from contextId to owned Logger instances.
    std::unordered_map<std::string, std::unique_ptr<Logger>> loggers_;

    /// Registered output sinks.
    std::vector<std::unique_ptr<ILogSink>> sinks_;

    /// [SWS_LOG_00095] Internal ring-buffer for data-loss prevention.
    static constexpr std::size_t kRingBufferSize{256U};
    std::vector<LogRecord>       ringBuffer_;
    std::size_t                  ringHead_  {0U};
    std::size_t                  ringTail_  {0U};
    std::size_t                  ringCount_ {0U};

    /// Connection state callback.
    std::function<void(ClientState)> connectionHandler_;

    /// Current remote client state.
    ClientState clientState_ {ClientState::kUnknown};
};

} // namespace internal
} // namespace log
} // namespace ara

#endif // ARA_LOG_INTERNAL_LOG_BACKEND_H_
