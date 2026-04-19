#ifndef LOG_SINK_H
#define LOG_SINK_H

#include <string>

namespace ara::log::sink {

class LogSink {
protected:
    LogSink() = default;
    LogSink(const LogSink&) = delete;
    LogSink& operator=(const LogSink&) = delete;
    virtual ~LogSink() = default;

public:
    virtual void Log(const std::string& message) = 0;
    virtual std::string GetTimestamp() const = 0;
    virtual std::string GetAppstamp() const = 0;
};

} // namespace ara::log::sink

#endif // LOG_SINK_H
