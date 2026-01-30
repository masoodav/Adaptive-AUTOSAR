#ifndef LOGGING_FRAMEWORK_H
#define LOGGING_FRAMEWORK_H

#include <stdexcept>
#include <vector>
#include <string>

#include "./logger.h"
// Assumes sink headers exist in your project structure
#include "./sink/log_sink.h"
#include "./sink/console_log_sink.h"
#include "./sink/file_log_sink.h"

namespace ara
{
    namespace log
    {
        /// @brief Logging framework which links loggers to a log sink
        class LoggingFramework
        {
        private:
            sink::LogSink *mLogSink;
            LogLevel mDefaultLogLevel;
            // FIX: Store pointers because Logger is non-copyable and managed by ara::log runtime
            std::vector<const Logger*> mLoggers;

            LoggingFramework(sink::LogSink *logSink, LogLevel logLevel);

        public:
            LoggingFramework() = delete;
            ~LoggingFramework() noexcept;

            const Logger &CreateLogger(
                std::string ctxId,
                std::string ctxDescription);

            const Logger &CreateLogger(
                std::string ctxId,
                std::string ctxDescription,
                LogLevel ctxDefLogLevel);

            void Log(
                const Logger &logger,
                LogLevel logLevel,
                const LogStream &logStream);

            static LoggingFramework *Create(
                std::string appId,
                LogMode logMode,
                LogLevel logLevel = LogLevel::kWarn,
                std::string appDescription = "");

            static LoggingFramework *Create(
                std::string appId,
                std::string filePath,
                LogLevel logLevel = LogLevel::kWarn,
                std::string appDescription = "");
        };
    }
}

#endif