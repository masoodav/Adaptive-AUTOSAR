/**
 * @file log_sink.cpp  (project replica)
 *
 * Violations fixed:
 *   [V3]  MISRA 5-10-1 – Renamed _result → result (lines 16, 32)
 *                         Renamed _time → rawTime, _localtime → localTm,
 *                         _timestamp → timestamp (lines 29-31)
 *   [V13] Misc          – Parameters appId, appDescription now passed by
 *                         const std::string& instead of by value (line 9)
 */
#include "./log_sink.h"

#include <ctime>

namespace ara
{
    namespace log
    {
        namespace sink
        {
            // [V13] const std::string& instead of std::string (by value)
            LogSink::LogSink(const std::string &appId,
                             const std::string &appDescription)
                : mApplicationId{appId},
                  mApplicationDescription{appDescription}
            {
            }

            // [V3] Renamed _result → result
            LogStream LogSink::GetAppstamp() const
            {
                LogStream result;           // [V3] was: LogStream _result;
                result << ara::core::StringView(mApplicationId);
                if (!mApplicationDescription.empty())
                {
                    result << ara::core::StringView("(");
                    result << ara::core::StringView(mApplicationDescription);
                    result << ara::core::StringView(")");
                }
                return result;
            }

            // [V3] Renamed _result → result, _time → rawTime,
            //      _localtime → localTm, _timestamp → timestamp
            LogStream LogSink::GetTimestamp() const
            {
                LogStream       result;                     // [V3] was: _result
                const std::time_t rawTime = std::time(NULL); // [V3] was: _time
                struct tm         localTm;                  // [V3] was: _localtime
                char              timestamp[32U];           // [V3] was: _timestamp
#if defined(_WIN32)
                (void)gmtime_s(&localTm, &rawTime);
#else
                (void)gmtime_r(&rawTime, &localTm);
#endif
                const std::size_t n =
                    std::strftime(timestamp, sizeof(timestamp),
                                  "%Y-%m-%dT%H:%M:%SZ", &localTm);
                if (n > 0U)
                {
                    result << ara::core::StringView(timestamp, n);
                }
                return result;
            }
        }
    }
}
