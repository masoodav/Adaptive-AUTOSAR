#ifndef FILE_LOG_SINK_H
#define FILE_LOG_SINK_H

#include "log_sink.h"
#include <string>
#include <fstream>

namespace ara::log::sink {

class FileLogSink : public LogSink {
public:
    explicit FileLogSink(
        const std::string& filePath,
        const std::string& appId,
        const std::string& appDescription
    );
    ~FileLogSink() override;

    void Log(const std::string& message) override;
    std::string GetTimestamp() const override;
    std::string GetAppstamp() const override;

private:
    std::string filePath_;
    std::string appId_;
    std::string appDescription_;
    std::ofstream fileStream_;
};

} // namespace ara::log::sink

#endif // FILE_LOG_SINK_H
