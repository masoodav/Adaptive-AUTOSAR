/**
 * @file common.h
 * @brief Common types for ara::log – ClientState and LogLevel.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00098]  enum class ClientState
 *   [SWS_LOG_00018]  enum class LogLevel
 *
 * Coding standards:
 *   - MISRA C++:2023 Rule 10.2.1  – scoped enumerations
 *   - CERT C++ DCL50-CPP           – no variadic function misuse
 *   - CWE-188                      – reliance on data/memory layout avoided
 *   - ISO/SAE 21434                – safety-critical type definitions
 */

#ifndef ARA_LOG_COMMON_H_
#define ARA_LOG_COMMON_H_

#include <cstdint>

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00098] ClientState
// ---------------------------------------------------------------------------

/**
 * @brief Represents the connection state of an external logging client.
 *
 * Underlying type fixed to std::int8_t as specified in [SWS_LOG_00098].
 */
enum class ClientState : std::int8_t
{
    kUnknown      = -1, ///< DLT back-end not yet running; state indeterminate.
    kNotConnected =  0, ///< No remote client detected.
    kConnected    =  1  ///< Remote client is connected.
};

// ---------------------------------------------------------------------------
// [SWS_LOG_00018] LogLevel
// ---------------------------------------------------------------------------

/**
 * @brief Severity levels for log messages.
 *
 * Underlying type fixed to std::uint8_t as specified in [SWS_LOG_00018].
 * Values are ordered from most severe (kFatal) to most verbose (kVerbose).
 * kOff disables all logging.
 */
enum class LogLevel : std::uint8_t
{
    kOff     = 0x00U, ///< No logging.
    kFatal   = 0x01U, ///< Fatal, non-recoverable error.
    kError   = 0x02U, ///< Error with impact on correct functionality.
    kWarn    = 0x03U, ///< Warning if correct behavior cannot be ensured.
    kInfo    = 0x04U, ///< Informational, high-level understanding.
    kDebug   = 0x05U, ///< Detailed programmer information.
    kVerbose = 0x06U  ///< Extra-verbose debug messages.
};

} // namespace log
} // namespace ara

#endif // ARA_LOG_COMMON_H_
