#include "logger.h"
#include <iostream>

namespace ara::log {

Logger::Logger(const std::string& appId, const std::string& appDescription)
    : appId_(appId), appDescription_(appDescription) {
    LoggerRegistry::Instance().AddLogger(this);
}

Logger& Logger::operator=(Logger&& other) noexcept {
    if (this != &other) {
        appId_ = std::move(other.appId_);
        appDescription_ = std::move(other.appDescription_);
    }
    return *this;
}

} // namespace ara::log
