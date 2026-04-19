#ifndef LOG_STREAM_H
#define LOG_STREAM_H

#include <string>
#include <sstream>
#include <memory>

namespace ara::log {

class LogStream {
public:
    LogStream() = default;
    ~LogStream();

    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    LogStream(LogStream&&) = default;
    LogStream& operator=(LogStream&&) = default;

    template <typename T>
    LogStream& operator<<(const T& value) {
        buffer_ << value;
        return *this;
    }

    void Flush();
    void Send();

private:
    std::ostringstream buffer_;
};

} // namespace ara::log

#endif // LOG_STREAM_H
