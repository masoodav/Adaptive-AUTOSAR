#ifndef ARA_LOG_LOG_FWD_H_
#define ARA_LOG_LOG_FWD_H_

#include <cstdint>
#include <functional>

namespace ara
{
namespace log
{

enum class ClientState : std::int8_t;
enum class LogLevel : std::uint8_t;
enum class LogMode : std::uint8_t;
enum class Fmt : std::uint16_t;

struct Format;

template <typename T>
class Argument;

class Logger;
class LogStream;

using ConnectionStateHandler = std::function<void(ClientState)>;

}  // namespace log
}  // namespace ara

#endif  // ARA_LOG_LOG_FWD_H_
