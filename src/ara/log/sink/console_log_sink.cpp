#include "./console_log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            ConsoleLogSink::ConsoleLogSink(
                const std::string& appId,
                const std::string& appDescription) : LogSink(appId, appDescription)
            {
            }

            void ConsoleLogSink::Log(const LogStream &logStream) const
            {
                LogStream timestamp = GetTimestamp();
                LogStream appstamp = GetAppstamp();
                static_cast<void>(timestamp << cWhitespace << appstamp << cWhitespace << logStream);
                std::string logString = timestamp.ToString();

                static_cast<void>(std::cout << logString << std::endl);
            }
        }
    }
}
