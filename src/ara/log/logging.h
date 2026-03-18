/**
 * @file logging.h
 * @brief Top-level public header for the ara::log Functional Cluster.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
 *
 * Includes the full public surface of ara::log.  Application code should
 * include this header rather than individual sub-headers.
 *
 * Traceability:
 *   [SWS_LOG_00001]  InitLogging – Logging framework initialisation
 *   [SWS_LOG_00123]  Deinitialisation via ara::core::Deinitialize
 *   [SWS_LOG_00002]  Silent discard on errors
 *   [SWS_LOG_00005]  CreateLogger ownership model
 *
 * Coding standards:
 *   - MISRA C++:2023 Rule 3.3.1 – one-definition rule
 *   - CERT C++ DCL51-CPP        – reserved names not used
 *   - ISO/SAE 21434             – controlled initialisation sequence
 */

#ifndef ARA_LOG_LOGGING_H_
#define ARA_LOG_LOGGING_H_

#include "common.h"
#include "log_stream.h"
#include "./logger.h"
#include "./log_backend.h"

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00001] InitLogging
// ---------------------------------------------------------------------------

/**
 * @brief Initialise the Log and Trace Functional Cluster.
 *
 * Must be called once by the Execution Management before any logging API
 * is used. Corresponds to ara::core::Initialize() triggering the FC startup.
 *
 * [SWS_LOG_00001]: "The Logging framework shall be initialised …"
 *
 * @param appId           Application ID (max 4 chars for DLT v1).
 * @param appDescription  Human-readable application description.
 * @param logMode         Output destination: kConsole, kFile, kRemote, kOff.
 * @param logFilePath     File path (relevant only when logMode includes kFile).
 */
inline void InitLogging(
    ara::core::StringView         appId,
    ara::core::StringView         appDescription,
    LogMode             logMode,
    ara::core::StringView         logFilePath = ara::core::StringView("")) noexcept
{
    internal::LoggingFramework::Instance().Initialize(
        appId, appDescription, logMode, logFilePath);
}

// ---------------------------------------------------------------------------
// [SWS_LOG_00123] DeinitLogging
// ---------------------------------------------------------------------------

/**
 * @brief Shut down the Log and Trace Functional Cluster.
 *
 * Must be called during process teardown. Flushes all pending log records.
 * Corresponds to ara::core::Deinitialize() triggering the FC shutdown.
 *
 * [SWS_LOG_00123]: "The Logging framework shall be de-initialised …"
 */
inline void DeinitLogging() noexcept
{
    internal::LoggingFramework::Instance().Deinitialize();
}

} // namespace log
} // namespace ara

#endif // ARA_LOG_LOGGING_H_
