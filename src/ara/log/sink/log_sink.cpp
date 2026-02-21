#include "./log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            LogSink::LogSink(const std::string& appId, const std::string& appDescription) 
                : mApplicationId{appId},
                  mApplicationDescription{appDescription}
            {
            }

            LogStream LogSink::GetAppstamp() const
            {
                LogStream result; // Fixed variable name from previous step
                static_cast<void>(result << mApplicationId);

                if (!mApplicationDescription.empty())
                {
                    static_cast<void>(result << cWhitespace << mApplicationDescription);
                }

                return result;
            }

            LogStream LogSink::GetTimestamp() const
            {
                std::time_t timeVal = std::time(nullptr);
                std::tm *localTime = std::localtime(&timeVal);
                char *timestampStr = std::asctime(localTime);
                LogStream result;
                static_cast<void>(result << timestampStr);

                return result;
            }
        }
    }
}