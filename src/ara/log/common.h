/**
 * @file common.h
 * @brief Common public types for ara::log.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 * C++14 compliant.
 *
 * Traceability:
 *   [SWS_LOG_00098]  enum class ClientState
 *   [SWS_LOG_00018]  enum class LogLevel
 *   [SWS_LOG_00228]  enum class LogMode  (console/file/remote/off sink selector)
 *
 * Coding standards: MISRA C++:2023 Rule 6.4.1 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_LOG_COMMON_H_
#define ARA_LOG_COMMON_H_

#include <cstdint>

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00098] enum class ClientState
// ---------------------------------------------------------------------------

/**
 * @brief Connection state of an external logging client.
 *
 * Underlying type: std::int8_t as specified in [SWS_LOG_00098].
 */
enum class ClientState : std::int8_t
{
    kUnknown      = -1, ///< DLT back-end not yet running; state unknown.
    kNotConnected =  0, ///< No remote client detected.
    kConnected    =  1  ///< Remote client is connected.
};

// ---------------------------------------------------------------------------
// [SWS_LOG_00018] enum class LogLevel
// ---------------------------------------------------------------------------

/**
 * @brief Severity levels for log messages.
 *
 * Underlying type: std::uint8_t as specified in [SWS_LOG_00018].
 * Lower numeric value == higher severity.
 * kOff disables all output.
 */
enum class LogLevel : std::uint8_t
{
    kOff     = 0x00U, ///< No logging.
    kFatal   = 0x01U, ///< Fatal, non-recoverable error.
    kError   = 0x02U, ///< Error with impact on correct functionality.
    kWarn    = 0x03U, ///< Warning if correct behaviour cannot be ensured.
    kInfo    = 0x04U, ///< Informational, high-level understanding.
    kDebug   = 0x05U, ///< Detailed programmer information.
    kVerbose = 0x06U  ///< Extra-verbose debug messages.
};

// ---------------------------------------------------------------------------
// [SWS_LOG_00228..00231] enum class LogMode
// ---------------------------------------------------------------------------

/**
 * @brief Log output destination selector.
 *
 * Underlying type: std::uint8_t.
 * Values are intended as bitmask flags so multiple sinks can be active
 * simultaneously (e.g. kConsole | kFile).
 *
 * Used by LoggingFramework::Create() and InitLogging() to choose the
 * active output sink(s). [SWS_LOG_00228..00231]
 */
enum class LogMode : std::uint8_t
{
    kOff     = 0x00U, ///< [SWS_LOG_00231] Discard all output.
    kRemote  = 0x01U, ///< [SWS_LOG_00230] Send via DLT / network.
    kFile    = 0x02U, ///< [SWS_LOG_00229] Write to a local file.
    kConsole = 0x04U  ///< [SWS_LOG_00228] Write to stdout/console.
};

} // namespace log
} // namespace ara

#endif // ARA_LOG_COMMON_H_
