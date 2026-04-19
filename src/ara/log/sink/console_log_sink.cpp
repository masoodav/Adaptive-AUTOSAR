#include "console_log_sink.h"
#include <iostream>

namespace ara::log::sink {

ConsoleLogSink::ConsoleLogSink(
    const std::string& appId,
    const std::string& appDescription
) : appId_(appId), appDescription_(appDescription) {}

void ConsoleLogSink::Log(const std::string& message) {
    std::cout << GetTimestamp() << " " << GetAppstamp() << " " << message << std::endl;
}

std::string ConsoleLogSink::GetTimestamp() const {
    return "2026-03-24T15:00:00";
}

std::string ConsoleLogSink::GetAppstamp() const {
    return appId_ + ": " + appDescription_;
}

} // namespace ara::log::sink
