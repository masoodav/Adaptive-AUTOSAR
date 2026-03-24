#include "./log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            const std::string LogSink::cWhitespace = " ";

            LogSink::LogSink(std::string appId, std::string appDescription)
                : mAppId(appId), mAppDescription(appDescription)
            {
            }
        }
    }
}
