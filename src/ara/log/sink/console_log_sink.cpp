/**
 * @file console_log_sink.cpp  (project replica)
 *
 * Violations fixed:
 *   [V3]  MISRA 5-10-1 – Renamed _timestamp → timestamp, _appstamp → appstamp,
 *                         _logString → logString (lines 17-20)
 *   [V13] Misc          – Parameters appId, appDescription now const std::string&
 */
#include "./console_log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            // [V13] const std::string& instead of std::string (by value)
            ConsoleLogSink::ConsoleLogSink(
                const std::string &appId,
                const std::string &appDescription)
                : LogSink(appId, appDescription)
            {
            }

            void ConsoleLogSink::Log(const LogStream &logStream) const
            {
                LogStream   timestamp = GetTimestamp();  // [V3] was: _timestamp
                LogStream   appstamp  = GetAppstamp();   // [V3] was: _appstamp
                timestamp << cWhitespace << appstamp << cWhitespace << logStream;
                std::string logString = timestamp.ToString(); // [V3] was: _logString

                std::cout << logString << std::endl;
            }
        }
    }
}
