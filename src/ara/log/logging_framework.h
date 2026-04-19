#ifndef LOGGING_FRAMEWORK_H
#define LOGGING_FRAMEWORK_H

#include "logger.h"
#include "log_sink.h"
#include <memory>

namespace ara::log {

class LoggingFramework {
public:
    static LoggingFramework& GetInstance() {
        static LoggingFramework instance;
        return instance;
    }

    void SetDefaultLogLevel(LogLevel level);
    std::unique_ptr<Logger> CreateLogger(const std::string& appId, const std::string& appDescription);

private:
    LoggingFramework() = default;
    ~LoggingFramework() = default;
    LoggingFramework(const LoggingFramework&) = delete;
    LoggingFramework& operator=(const LoggingFramework&) = delete;
};

} // namespace ara::log

#endif // LOGGING_FRAMEWORK_H
