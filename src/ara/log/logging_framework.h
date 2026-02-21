#ifndef LOGGING_FRAMEWORK_H
#define LOGGING_FRAMEWORK_H

#include <stdexcept>
#include <vector>
#include <string>

#include "./logger.h"
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
            // Logger is non-copyable, so we store pointers
            std::vector<const Logger*> mLoggers;

            LoggingFramework(sink::LogSink *logSink, LogLevel logLevel);

        public:
            LoggingFramework() = delete;
            ~LoggingFramework() noexcept;

            const Logger &CreateLogger(
                const std::string& ctxId,
                const std::string& ctxDescription);

            const Logger &CreateLogger(
                const std::string& ctxId,
                const std::string& ctxDescription,
                LogLevel ctxDefLogLevel);

            void Log(
                const Logger &logger,
                LogLevel logLevel,
                const LogStream &logStream);

            static LoggingFramework *Create(
                const std::string& appId,
                LogMode logMode,
                LogLevel logLevel = LogLevel::kWarn,
                const std::string& appDescription = "");

            static LoggingFramework *Create(
                const std::string& appId,
                const std::string& filePath,
                LogLevel logLevel = LogLevel::kWarn,
                const std::string& appDescription = "");
        };
    }
}

#endif