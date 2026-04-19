#ifndef CONSOLE_LOG_SINK_H
#define CONSOLE_LOG_SINK_H

#include "log_sink.h"
#include <string>

namespace ara::log::sink {

class ConsoleLogSink : public LogSink {
public:
    explicit ConsoleLogSink(
        const std::string& appId,
        const std::string& appDescription
    );
    ~ConsoleLogSink() override = default;

    void Log(const std::string& message) override;
    std::string GetTimestamp() const override;
    std::string GetAppstamp() const override;

private:
    std::string appId_;
    std::string appDescription_;
};

} // namespace ara::log::sink

#endif // CONSOLE_LOG_SINK_H
