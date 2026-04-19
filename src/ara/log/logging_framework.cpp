#include "logging_framework.h"
#include <iostream>

namespace ara::log {

void LoggingFramework::SetDefaultLogLevel(LogLevel level) {
    // Set default log level
}

std::unique_ptr<Logger> LoggingFramework::CreateLogger(
    const std::string& appId,
    const std::string& appDescription
) {
    auto logger = std::make_unique<Logger>(appId, appDescription);
    return logger;
}

} // namespace ara::log
