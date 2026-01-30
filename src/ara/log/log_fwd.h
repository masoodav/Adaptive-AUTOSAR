#ifndef ARA_LOG_LOG_FWD_H
#define ARA_LOG_LOG_FWD_H

#include <cstdint>

namespace ara {
namespace log {

enum class LogLevel : std::uint8_t;
enum class ClientState : std::int8_t;
enum class Fmt : std::uint16_t;

class Logger;
class LogStream;
struct Format;

template <typename T>
class Argument;

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOG_FWD_H