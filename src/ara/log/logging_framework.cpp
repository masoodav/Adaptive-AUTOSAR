#include "./logging_framework.h"

namespace ara
{
    namespace log
    {
        LoggingFramework::LoggingFramework(
            sink::LogSink *logSink,
            LogLevel logLevel) : mLogSink{logSink},
                                 mDefaultLogLevel{logLevel}
        {
        }

        const Logger &LoggingFramework::CreateLogger(
            const std::string& ctxId,
            const std::string& ctxDescription)
        {
            Logger& loggerRef = 
                const_cast<Logger&>(ara::log::CreateLogger(core::StringView(ctxId.c_str()), core::StringView(ctxDescription.c_str()), mDefaultLogLevel));
            
            sink::LogSink* sinkPtr = mLogSink;
            
            loggerRef.SetLogHandler([sinkPtr](LogLevel /*level*/, const std::string& msg) {
                LogStream tmp; 
                static_cast<void>(tmp << msg);
                sinkPtr->Log(tmp);
            });

            mLoggers.push_back(&loggerRef);
            return loggerRef;
        }

        const Logger &LoggingFramework::CreateLogger(
            const std::string& ctxId,
            const std::string& ctxDescription,
            LogLevel ctxDefLogLevel)
        {
            Logger& loggerRef =
                const_cast<Logger&>(ara::log::CreateLogger(core::StringView(ctxId.c_str()), core::StringView(ctxDescription.c_str()), ctxDefLogLevel));
            
            sink::LogSink* sinkPtr = mLogSink;
            
            loggerRef.SetLogHandler([sinkPtr](LogLevel /*level*/, const std::string& msg) {
                LogStream tmp; 
                static_cast<void>(tmp << msg);
                sinkPtr->Log(tmp);
            });

            mLoggers.push_back(&loggerRef);
            return loggerRef;
        }

        void LoggingFramework::Log(const Logger &logger, LogLevel logLevel, const LogStream &logStream) {
             bool levelEnabled = logger.IsEnabled(logLevel); 
             if (levelEnabled) {
                 LogStream logStreamContext = logger.WithLevel(logLevel); 
                 static_cast<void>(logStreamContext << logStream);
             }
        }

        LoggingFramework *LoggingFramework::Create(const std::string& appId, LogMode logMode, LogLevel logLevel, const std::string& appDescription) {
            if (logMode == LogMode::kFile) throw std::invalid_argument("File logging mode is not feasible within this constructor override.");
            if (logMode == LogMode::kConsole) {
                sink::LogSink *logSink = new sink::ConsoleLogSink(appId, appDescription); 
                return new LoggingFramework(logSink, logLevel);
            }
            throw std::invalid_argument("The log mode is not currently supported.");
        }

        LoggingFramework *LoggingFramework::Create(const std::string& appId, const std::string& filePath, LogLevel logLevel, const std::string& appDescription) {
            sink::LogSink *logSink = new sink::FileLogSink(filePath, appId, appDescription); 
            return new LoggingFramework(logSink, logLevel);
        }

        LoggingFramework::~LoggingFramework() noexcept {
            if (mLogSink) delete mLogSink;
        }
    }
}