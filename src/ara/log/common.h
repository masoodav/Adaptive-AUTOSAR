#ifndef ARA_LOG_COMMON_H_
#define ARA_LOG_COMMON_H_

#include <cstdint>

#include "log_fwd.h"

namespace ara
{
namespace log
{

// Tags: [SWS_LOG_00001]
enum class ClientState : std::int8_t
{
    kUnknown = -1,
    kNotConnected = 0,
    kConnected = 1
};

// Tags: [SWS_LOG_00039] [SWS_LOG_00040] [SWS_LOG_00041] [SWS_LOG_00042]
// [SWS_LOG_00043] [SWS_LOG_00044] [SWS_LOG_00045] [SWS_LOG_00046]
// [SWS_LOG_00047] [SWS_LOG_00048] [SWS_LOG_00049] [SWS_LOG_00050]
// [SWS_LOG_00051] [SWS_LOG_00062] [SWS_LOG_00063] [SWS_LOG_00064]
// [SWS_LOG_00065] [SWS_LOG_00066] [SWS_LOG_00067] [SWS_LOG_00068]
// [SWS_LOG_00069] [SWS_LOG_00070]
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

// Tags: [SWS_LOG_00007] [SWS_LOG_00008] [SWS_LOG_00009] [SWS_LOG_00010]
// [SWS_LOG_00011] [SWS_LOG_00012] [SWS_LOG_00013]
enum class LogMode : std::uint8_t
{
    kConsole = 0U,
    kFile = 1U
};

}  // namespace log
}  // namespace ara

#endif  // ARA_LOG_COMMON_H_
