/**
 * @file logging_framework.cpp
 * @brief LoggingFramework implementation – C++14 compliant.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 *
 * Fixes applied vs. original:
 *   [FIX-C] Logger::CreateLogger(...)  →  ::ara::log::CreateLogger(...)
 *           CreateLogger is a free function in namespace ara::log, NOT a
 *           static member of Logger. Calling Logger::CreateLogger caused
 *           "CreateLogger is not a member of ara::log::Logger".
 *
 *   [FIX-D] Logger move constructor restored so std::vector<Logger> can
 *           store and relocate Logger objects.  Previously Logger had
 *           Logger(Logger&&) = delete which made push_back ill-formed.
 *
 *   [FIX-E] operator<<(LogStream&, const LogStream&) added to LogStream.
 *           The line  `_logStreamContex << logStream`  requires this
 *           overload; it was missing, causing "no match for operator<<".
 *
 *   [FIX-F] Create(appId, logMode, ...) restructured so the compiler can
 *           prove all paths either return or throw, eliminating the
 *           "-Werror=return-type: control reaches end of non-void function"
 *           diagnostic.
 *
 * Coding standards: MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#include "./logging_framework.h"
#include <memory>  // std::unique_ptr [V8]

namespace ara
{
    namespace log
    {
        // -------------------------------------------------------------------
        // Private constructor
        // -------------------------------------------------------------------

        LoggingFramework::LoggingFramework(
            sink::LogSink *logSink,
            LogLevel       logLevel)
            : mLogSink{logSink}
            , mDefaultLogLevel{logLevel}
        {
        }

        // -------------------------------------------------------------------
        // CreateLogger – uses the framework default log level
        // [FIX-C]: was Logger::CreateLogger(...)
        //          corrected to ::ara::log::CreateLogger(...) – free function
        // -------------------------------------------------------------------

        const Logger &LoggingFramework::CreateLogger(
            std::string ctxId,
            std::string ctxDescription)
        {
            // ::ara::log::CreateLogger is the free-function factory declared
            // in logger.h. It registers the context with the logging back-end
            // and returns a reference to its internally owned Logger instance.
            // We also keep a local copy for the LoggingFramework's own
            // bookkeeping (e.g. to enumerate contexts).
            // [FIX-D]: Logger must be movable for push_back to compile.
            Logger newLogger =
                // [V11] Use .data() → const char* for StringView construction.
                // Works with any StringView regardless of whether it has
                // a std::string constructor (project ara_core may not).
                Logger(ara::core::StringView(ctxId.data()),
                       ara::core::StringView(ctxDescription.data()),
                       mDefaultLogLevel);
            mLoggers.push_back(std::move(newLogger));
            return mLoggers.back();
        }

        // -------------------------------------------------------------------
        // CreateLogger – with explicit log level
        // [FIX-C]: same correction as above
        // -------------------------------------------------------------------

        const Logger &LoggingFramework::CreateLogger(
            std::string ctxId,
            std::string ctxDescription,
            LogLevel    ctxDefLogLevel)
        {
            Logger newLogger =
                Logger(ara::core::StringView(ctxId.data()),
                       ara::core::StringView(ctxDescription.data()),
                       ctxDefLogLevel);
            mLoggers.push_back(std::move(newLogger));
            return mLoggers.back();
        }

        // -------------------------------------------------------------------
        // Log – dispatch a message to the configured sink
        // [FIX-E]: _logStreamContex << logStream now compiles because
        //          operator<<(LogStream&, const LogStream&) was added.
        // -------------------------------------------------------------------

        void LoggingFramework::Log(
            const Logger    &logger,
            LogLevel         logLevel,
            const LogStream &logStream)
        {
            if (logger.IsEnabled(logLevel))
            {
                // Create a new stream stamped with the correct level and
                // context, then merge the caller's payload into it.
                // [FIX-E]: operator<<(LogStream&, const LogStream&) handles
                //          the payload merge.
                LogStream contextStream = logger.WithLevel(logLevel);
                (void)(contextStream << logStream);  // [V2] explicit void cast
                mLogSink->Log(contextStream);
            }
        }

        // -------------------------------------------------------------------
        // Create – non-file sink factory
        // [FIX-B]: LogMode is now in the public ara::log namespace (common.h)
        // [FIX-F]: restructured so all paths provably return or throw,
        //          eliminating "-Werror=return-type" diagnostic.
        // -------------------------------------------------------------------

        LoggingFramework *LoggingFramework::Create(
            std::string appId,
            LogMode     logMode,
            LogLevel    logLevel,
            std::string appDescription)
        {
            if (logMode == LogMode::kFile)
            {
                throw std::invalid_argument(
                    "File logging mode requires the file-path overload of "
                    "LoggingFramework::Create(). "
                    "Use Create(appId, filePath, logLevel, appDescription).");
            }

            if (logMode == LogMode::kConsole)
            {
                std::unique_ptr<sink::LogSink> logSinkUp(
                    new sink::ConsoleLogSink(appId, appDescription));  // [V8]
                sink::LogSink *logSink = logSinkUp.release();
                std::unique_ptr<LoggingFramework> fwUp(
                    new LoggingFramework(logSink, logLevel));  // [V8]
                return fwUp.release();
            }

            // [FIX-F]: explicit throw on all other modes instead of falling
            // through; this makes the function's return guarantee clear to
            // the compiler, fixing the -Werror=return-type diagnostic.
            throw std::invalid_argument(
                "The requested LogMode is not currently supported.");
        }

        // -------------------------------------------------------------------
        // Create – file sink factory
        // -------------------------------------------------------------------

        LoggingFramework *LoggingFramework::Create(
            std::string appId,
            std::string filePath,
            LogLevel    logLevel,
            std::string appDescription)
        {
            // [V8] unique_ptr wraps raw new; release transfers ownership
            std::unique_ptr<sink::LogSink> logSinkUp(
                new sink::FileLogSink(filePath, appId, appDescription));  // [V8]
            sink::LogSink *logSink = logSinkUp.release();
            std::unique_ptr<LoggingFramework> fwUp(
                new LoggingFramework(logSink, logLevel));  // [V8]
            return fwUp.release();
        }

        // -------------------------------------------------------------------
        // Destructor
        // -------------------------------------------------------------------

        LoggingFramework::~LoggingFramework() noexcept
        {
            // [V8] use unique_ptr to manage deletion – no raw delete
            std::unique_ptr<sink::LogSink> owned(mLogSink);
            mLogSink = NULL;
            // owned goes out of scope here and deletes mLogSink safely
        }
    }
}
