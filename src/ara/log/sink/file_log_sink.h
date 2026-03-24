#ifndef FILE_LOG_SINK_H
#define FILE_LOG_SINK_H

#include "./log_sink.h"
#include "../logger.h"
#include <fstream>

namespace ara
{
    namespace log
    {
        namespace sink
        {
            class FileLogSink : public LogSink
            {
            public:
                /// @brief Constructor
                /// @param filePath Path to the log file
                /// @param appId Application ID
                /// @param appDescription Application description
                FileLogSink(
                    std::string filePath,
                    std::string appId,
                    std::string appDescription);

                void Log(const LogStream &logStream) const override;
                LogStream GetTimestamp() const override;
                LogStream GetAppstamp() const override;

            private:
                mutable std::ofstream mLogFile;
            };
        }
    }
}

#endif
