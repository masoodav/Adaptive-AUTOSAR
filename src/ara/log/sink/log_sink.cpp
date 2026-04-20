#include "./log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            LogSink::LogSink(const std::string& appId, const std::string& appDescription) : mApplicationId{appId},
                                                                                             mApplicationDescription{appDescription}
            {
            }

            LogStream LogSink::GetAppstamp() const
            {
                LogStream result;
                static_cast<void>(result << mApplicationId);

                if (!mApplicationDescription.empty())
                {
                    static_cast<void>(result << cWhitespace << mApplicationDescription);
                }

                return result;
            }

            LogStream LogSink::GetTimestamp() const
            {
                std::time_t currentTime = std::time(nullptr);
                std::tm *localTime = std::localtime(&currentTime);
                char *timestamp = std::asctime(localTime);
                LogStream result;
                static_cast<void>(result << timestamp);

                return result;
            }
        }
    }
}
