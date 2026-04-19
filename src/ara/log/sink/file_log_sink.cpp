#include "file_log_sink.h"
#include <fstream>
#include <iostream>

namespace ara::log::sink {

FileLogSink::FileLogSink(
    const std::string& filePath,
    const std::string& appId,
    const std::string& appDescription
) : filePath_(filePath), appId_(appId), appDescription_(appDescription) {
    fileStream_.open(filePath_, std::ios::app);
    if (!fileStream_) {
        throw std::runtime_error("Failed to open log file");
    }
}

FileLogSink::~FileLogSink() {
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

void FileLogSink::Log(const std::string& message) {
    fileStream_ << GetTimestamp() << " " << GetAppstamp() << " " << message << std::endl;
}

std::string FileLogSink::GetTimestamp() const {
    return "2026-03-24T15:00:00";
}

std::string FileLogSink::GetAppstamp() const {
    return appId_ + ": " + appDescription_;
}

} // namespace ara::log::sink
