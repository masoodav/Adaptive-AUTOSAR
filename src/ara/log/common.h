#ifndef ARA_LOG_COMMON_H_
#define ARA_LOG_COMMON_H_

#include <cstdint>

#include "log_fwd.h"

namespace ara
{
namespace log
{

enum class ClientState : std::int8_t
{
    kUnknown = -1,
    kNotConnected = 0,
    kConnected = 1
};

enum class LogLevel : std::uint8_t
{
    kOff = 0x00U,
    kFatal = 0x01U,
    kError = 0x02U,
    kWarn = 0x03U,
    kInfo = 0x04U,
    kDebug = 0x05U,
    kVerbose = 0x06U
};

}  // namespace log
}  // namespace ara

#endif  // ARA_LOG_COMMON_H_
