#include "./file_log_sink.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ara
{
    namespace log
    {
        namespace sink
        {
            FileLogSink::FileLogSink(
                std::string filePath,
                std::string appId,
                std::string appDescription)
                : LogSink(appId, appDescription)
            {
                mLogFile.open(filePath, std::ios::app);
                if (!mLogFile.is_open()) {
                    throw std::runtime_error("Failed to open log file: " + filePath);
                }
            }

            void FileLogSink::Log(const LogStream &logStream) const
            {
                LogStream timestamp = GetTimestamp();
                LogStream appstamp = GetAppstamp();

                // Get the buffer content from each stream
                std::string timestampStr = timestamp.GetBuffer();
                std::string appstampStr = appstamp.GetBuffer();
                std::string logStreamStr = logStream.GetBuffer();

                // Combine the strings
                std::string logString = timestampStr + cWhitespace + appstampStr + cWhitespace + logStreamStr;

                mLogFile << logString << std::endl;
                mLogFile.flush();
            }

            LogStream FileLogSink::GetTimestamp() const
            {
                // Create a temporary logger for the timestamp
                Logger tempLogger = Logger::CreateLogger("timestamp", "timestamp");

                // Create a log stream using the static Create method
                LogStream stream = LogStream::Create(tempLogger, LogLevel::kInfo);

                // Get current time
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);

                std::stringstream ss;
                ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");

                stream << "[" << ss.str() << "]";
                return stream;
            }

            LogStream FileLogSink::GetAppstamp() const
            {
                // Create a temporary logger for the app stamp
                Logger tempLogger = Logger::CreateLogger("appstamp", "appstamp");

                // Create a log stream using the static Create method
                LogStream stream = LogStream::Create(tempLogger, LogLevel::kInfo);

                // Add application stamp logic here
                stream << "[" << mAppId << ":" << mAppDescription << "]";

                return stream;
            }
        }
    }
}
