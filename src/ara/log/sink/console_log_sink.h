/**
 * @file console_log_sink.h
 * @brief Console (stdout) log sink.
 *
 * Matches the ara::log::sink::ConsoleLogSink interface from the
 * Adaptive-AUTOSAR project (masoodav/Adaptive-AUTOSAR, branch claude_dev).
 * C++14 compliant.
 *
 * Fixes applied vs. previous stub:
 *   [FIX-1] Constructor is DECLARED only (no inline body).
 *           The project already has console_log_sink.cpp which DEFINES it.
 *           Having an inline definition in the header AND a .cpp definition
 *           caused: "redefinition of ConsoleLogSink::ConsoleLogSink".
 *
 *   [FIX-2A] Log() is DECLARED only (no inline body) and marked const,
 *            matching the definition in console_log_sink.cpp (line 15):
 *              void ConsoleLogSink::Log(const LogStream&) const
 *            The previous stub declared Log() without const, causing:
 *            "no declaration matches void ConsoleLogSink::Log(…) const".
 */

#ifndef ARA_LOG_SINK_CONSOLE_LOG_SINK_H_
#define ARA_LOG_SINK_CONSOLE_LOG_SINK_H_

#include <string>
#include "log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            /**
             * @brief Writes log messages to stdout.
             */
            class ConsoleLogSink final : public LogSink
            {
            public:
                /**
                 * @param appId          Application identifier string.
                 * @param appDescription Human-readable application description.
                 *
                 * [FIX-1] Declaration only – definition lives in
                 *         console_log_sink.cpp to avoid redefinition error.
                 */
                ConsoleLogSink(std::string appId,
                               std::string appDescription);

                ~ConsoleLogSink() = default;

                /**
                 * @brief Write a log message to stdout.
                 *
                 * [FIX-2A] Declared const to match the definition in
                 *          console_log_sink.cpp:
                 *            void ConsoleLogSink::Log(…) const
                 *
                 * @param logStream  The completed log message.
                 */
                void Log(
                    const LogStream &logStream) const override;

            };

        } // namespace sink
    } // namespace log
} // namespace ara

#endif // ARA_LOG_SINK_CONSOLE_LOG_SINK_H_
