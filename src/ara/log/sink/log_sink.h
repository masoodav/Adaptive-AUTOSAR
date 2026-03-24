#ifndef LOG_SINK_H
#define LOG_SINK_H

#include "../log_stream.h"
#include <string>

namespace ara
{
    namespace log
    {
        namespace sink
        {
            class LogSink
            {
            protected:
                std::string mAppId;
                std::string mAppDescription;
                static const std::string cWhitespace;

            public:
                /// @brief Constructor
                /// @param appId Application ID
                /// @param appDescription Application description
                LogSink(std::string appId, std::string appDescription);

                virtual ~LogSink() = default;

                /// @brief Log a message
                /// @param logStream Stream to be logged
                virtual void Log(const LogStream &logStream) const = 0;

                /// @brief Get a timestamp log stream
                virtual LogStream GetTimestamp() const = 0;

                /// @brief Get an application stamp log stream
                virtual LogStream GetAppstamp() const = 0;
            };
        }
    }
}

#endif
