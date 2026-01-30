#ifndef ARA_LOG_COMMON_H
#define ARA_LOG_COMMON_H

#include "./log_fwd.h"

namespace ara {
namespace log {

// [SWS_LOG_00098] Definition of API enum ara::log::ClientState
enum class ClientState : std::int8_t {
    kUnknown = -1,
    kNotConnected = 0,
    kConnected = 1
};

// [SWS_LOG_00018] Definition of API enum ara::log::LogLevel
enum class LogLevel : std::uint8_t {
    kOff = 0x00,
    kFatal = 0x01,
    kError = 0x02,
    kWarn = 0x03,
    kInfo = 0x04,
    kDebug = 0x05,
    kVerbose = 0x06
};

// Definition of LogMode required by LoggingFramework
enum class LogMode : std::uint8_t {
    kConsole = 0x00,
    kFile = 0x01,
    kRemote = 0x02
};

} // namespace log
} // namespace ara

#endif // ARA_LOG_COMMON_H