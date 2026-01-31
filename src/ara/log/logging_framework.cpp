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
            std::string ctxId,
            std::string ctxDescription)
        {
            // Get the managed Logger instance
            Logger& loggerRef = 
                const_cast<Logger&>(ara::log::CreateLogger(core::StringView(ctxId.c_str()), core::StringView(ctxDescription.c_str()), mDefaultLogLevel));
            
            // FIX: Inject the Sink logic into the Logger
            // This is the bridge: When Logger flushes, it calls this lambda, which calls the Sink
            sink::LogSink* sinkPtr = mLogSink;
            loggerRef.SetLogHandler([sinkPtr](LogLevel level, const std::string& msg) {
                // We create a temporary stream to satisfy the Sink's API
                // This is a zero-cost wrapper around the string
                LogStream tmp; 
                tmp << msg;
                sinkPtr->Log(tmp);
            });

            mLoggers.push_back(&loggerRef);
            return loggerRef;
        }

        const Logger &LoggingFramework::CreateLogger(
            std::string ctxId,
            std::string ctxDescription,
            LogLevel ctxDefLogLevel)
        {
            Logger& loggerRef =
                const_cast<Logger&>(ara::log::CreateLogger(core::StringView(ctxId.c_str()), core::StringView(ctxDescription.c_str()), ctxDefLogLevel));
            
            // FIX: Inject Sink
            sink::LogSink* sinkPtr = mLogSink;
            loggerRef.SetLogHandler([sinkPtr](LogLevel level, const std::string& msg) {
                LogStream tmp; 
                tmp << msg;
                sinkPtr->Log(tmp);
            });

            mLoggers.push_back(&loggerRef);
            return loggerRef;
        }

        // [Log, Create, Destructor methods remain the same as previous step]
        void LoggingFramework::Log(const Logger &logger, LogLevel logLevel, const LogStream &logStream) {
             bool _isLevelEnabled = logger.IsEnabled(logLevel);
             if (_isLevelEnabled) {
                 LogStream _logStreamContext = logger.WithLevel(logLevel);
                 _logStreamContext << logStream;
                 // Note: With SetLogHandler, _logStreamContext will automatically route to mLogSink on flush!
             }
        }

        LoggingFramework *LoggingFramework::Create(std::string appId, LogMode logMode, LogLevel logLevel, std::string appDescription) {
            if (logMode == LogMode::kFile) throw std::invalid_argument("File logging mode is not feasible within this constructor override.");
            if (logMode == LogMode::kConsole) {
                sink::LogSink *_logSink = new sink::ConsoleLogSink(appId, appDescription);
                return new LoggingFramework(_logSink, logLevel);
            }
            throw std::invalid_argument("The log mode is not currently supported.");
        }

        LoggingFramework *LoggingFramework::Create(std::string appId, std::string filePath, LogLevel logLevel, std::string appDescription) {
            sink::LogSink *_logSink = new sink::FileLogSink(filePath, appId, appDescription);
            return new LoggingFramework(_logSink, logLevel);
        }

        LoggingFramework::~LoggingFramework() noexcept {
            if (mLogSink) delete mLogSink;
        }
    }
}