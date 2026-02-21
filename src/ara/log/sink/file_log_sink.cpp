#include "./file_log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            FileLogSink::FileLogSink(
                const std::string& appId,
                const std::string& appDescription,
                const std::string& logFilePath) : LogSink(appId, appDescription),
                                           mLogFilePath{logFilePath}
            {
            }

            void FileLogSink::Log(const LogStream &logStream) const
            {
                const std::string cNewline{"\n"};

                LogStream timestamp = GetTimestamp();
                LogStream appstamp = GetAppstamp();
                static_cast<void>(timestamp << cWhitespace << appstamp  << cWhitespace << logStream);
                std::string logString = timestamp.ToString();

                std::ofstream logFileStream(
                    mLogFilePath, std::ofstream::out | std::ofstream::app);
                static_cast<void>(logFileStream << logString << cNewline);
                logFileStream.close();
            }
        }
    }
}