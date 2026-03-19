/**
 * @file file_log_sink.cpp  (project replica)
 *
 * Violations fixed:
 *   [V3]  MISRA 5-10-1 – Renamed _timestamp → timestamp, _appstamp → appstamp,
 *                         _logString → logString (lines 21-24)
 *   [V6]  MISRA 13-3-3 – Parameter names now match file_log_sink.h declaration
 *                         (logFilePath, appId, appDescription – was mismatch)
 *   [V13] Misc          – All parameters now const std::string&
 */
#include "./file_log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            // [V6] Parameter names match header exactly
            // [V13] const std::string& instead of by value
            FileLogSink::FileLogSink(
                const std::string &logFilePath,  // [V6][V13]
                const std::string &appId,        // [V6][V13]
                const std::string &appDescription) // [V6][V13]
                : LogSink(appId, appDescription),
                  mLogFilePath{logFilePath}
            {
            }

            void FileLogSink::Log(const LogStream &logStream) const
            {
                LogStream   timestamp = GetTimestamp();   // [V3] was: _timestamp
                LogStream   appstamp  = GetAppstamp();    // [V3] was: _appstamp
                (void)(timestamp << cWhitespace << appstamp << cWhitespace << logStream);  // [V3a]
                std::string logString = timestamp.ToString(); // [V3] was: _logString

                std::ofstream logFileStream(
                    mLogFilePath, std::ofstream::out | std::ofstream::app);
                if (logFileStream.is_open())
                {
                    (void)(logFileStream << logString << std::endl);  // [V3a]
                }
            }
        }
    }
}
