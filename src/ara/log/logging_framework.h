/**
 * @file logging_framework.h
 * @brief LoggingFramework – links Logger instances to a log sink.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 * C++14 compliant.
 *
 * Fixes applied vs. original:
 *   [FIX-A] Added #include <vector>  →  resolves "std::vector not declared"
 *   [FIX-B] LogMode now declared in ara::log (common.h) →  resolves
 *            "'LogMode' has not been declared"
 *
 * Coding standards: MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef LOGGING_FRAMEWORK_H
#define LOGGING_FRAMEWORK_H

#include <stdexcept>
#include <vector>           /* [FIX-A] was missing */

#include "./logger.h"
#include "./sink/log_sink.h"
#include "./sink/console_log_sink.h"
#include "./sink/file_log_sink.h"

// LogMode is now defined in ara/log/common.h (included transitively
// through logger.h → common.h). [FIX-B]

namespace ara
{
    namespace log
    {
        /// @brief Logging framework which links loggers to a log sink.
        class LoggingFramework
        {
        private:
            sink::LogSink  *mLogSink;
            LogLevel        mDefaultLogLevel;
            std::vector<Logger> mLoggers; /* [FIX-A] <vector> now included */

            LoggingFramework(sink::LogSink *logSink, LogLevel logLevel);

        public:
            LoggingFramework() = delete;
            ~LoggingFramework() noexcept;

            /// @brief Create a logger using the framework default log level.
            /// @param ctxId Log context ID
            /// @param ctxDescription Log context description
            /// @returns Reference to the internally stored logger
            const Logger &CreateLogger(
                std::string ctxId,
                std::string ctxDescription);

            /// @brief Create a logger with an explicit log level.
            /// @param ctxId Log context ID
            /// @param ctxDescription Log context description
            /// @param ctxDefLogLevel Log context default log level
            /// @returns Reference to the internally stored logger
            const Logger &CreateLogger(
                std::string ctxId,
                std::string ctxDescription,
                LogLevel ctxDefLogLevel);

            /// @brief Log a stream to the configured sink.
            /// @param logger The logging context
            /// @param logLevel Severity level of this message
            /// @param logStream Pre-built message stream to log
            void Log(
                const Logger    &logger,
                LogLevel         logLevel,
                const LogStream &logStream);

            /// @brief Factory for non-file sinks (console / remote).
            /// @param appId Application ID
            /// @param logMode Output sink mode – must NOT be LogMode::kFile
            /// @param logLevel Default log severity level
            /// @param appDescription Application description
            /// @returns Pointer to the created LoggingFramework (caller owns)
            /// @throws std::invalid_argument when logMode == LogMode::kFile
            ///         or an unsupported mode is requested.
            /// @see Create(std::string, std::string, LogLevel, std::string)
            static LoggingFramework *Create(
                std::string appId,
                LogMode     logMode,
                LogLevel    logLevel = LogLevel::kWarn,
                std::string appDescription = "");

            /// @brief Factory for file sinks only.
            /// @param appId Application ID
            /// @param filePath Destination log file path
            /// @param logLevel Default log severity level
            /// @param appDescription Application description
            /// @returns Pointer to the created LoggingFramework (caller owns)
            /// @see Create(std::string, LogMode, LogLevel, std::string)
            static LoggingFramework *Create(
                std::string appId,
                std::string filePath,
                LogLevel    logLevel = LogLevel::kWarn,
                std::string appDescription = "");
        };
    }
}

#endif // LOGGING_FRAMEWORK_H
