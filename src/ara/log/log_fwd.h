/**
 * @file log_fwd.h
 * @brief Forward declarations for ara::log types.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00098]  ClientState
 *   [SWS_LOG_00018]  LogLevel
 *   [SWS_LOG_00173]  LogStream
 *   [SWS_LOG_00261]  Argument<T>
 *   [SWS_LOG_00172]  Logger
 *   [SWS_LOG_00206]  Fmt
 *   [SWS_LOG_00207]  Format
 *
 * MISRA C++:2023 compliant | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_LOG_LOG_FWD_H_
#define ARA_LOG_LOG_FWD_H_

#include <cstdint>

namespace ara {
namespace log {

// ---- Enumerations --------------------------------------------------------

/// [SWS_LOG_00098]
enum class ClientState : std::int8_t;

/// [SWS_LOG_00018]
enum class LogLevel : std::uint8_t;

/// [SWS_LOG_00206]
enum class Fmt : std::uint16_t;

// ---- Classes / Structs ---------------------------------------------------

// Format is defined in ara/log/logger.h (complete definition required for Argument<T>).

/// [SWS_LOG_00261]
template <typename T>
class Argument;

/// [SWS_LOG_00173]
class LogStream;

/// [SWS_LOG_00172]
class Logger;

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOG_FWD_H_
