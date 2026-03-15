/**
 * @file log_sink.h
 * @brief Abstract log sink base class.
 *
 * Matches the ara::log::sink::LogSink interface from the
 * Adaptive-AUTOSAR project (masoodav/Adaptive-AUTOSAR, branch claude_dev).
 * C++14 compliant.
 *
 * Fixes applied vs. previous version:
 *
 *   [FIX-1]  Constructor changed from inline definition to declaration only.
 *            log_sink.cpp line 9 defines it:
 *              LogSink::LogSink(string, string) : mApplicationId{...}, ...
 *            Having the body inline in the header AND in a .cpp violated the
 *            One Definition Rule, causing:
 *            "redefinition of LogSink::LogSink(std::string, std::string)"
 *            and three cascading parse errors (lines 9-11).
 *
 *   [FIX-2]  GetAppstamp() changed from inline definition to declaration only,
 *            and noexcept removed.
 *            log_sink.cpp line 14 defines it WITHOUT noexcept:
 *              LogStream LogSink::GetAppstamp() const
 *            An exception specifier is part of the function type – a
 *            declaration with noexcept and a definition without it are
 *            mismatched, and having both bodies is a redefinition:
 *            "redefinition of LogStream ara::log::sink::LogSink::GetAppstamp()"
 *
 *   [FIX-3]  GetTimestamp() same treatment as GetAppstamp(). [FIX-2].
 *            "redefinition of LogStream ara::log::sink::LogSink::GetTimestamp()"
 *
 *   [FIX-4]  Private member renamed mAppId → mApplicationId.
 *            log_sink.cpp line 9 initialises:
 *              mApplicationId{appId}
 *            The previous name mAppId would cause "no field named mApplicationId".
 *
 *   [FIX-5]  Private member renamed mAppDescription → mApplicationDescription.
 *            log_sink.cpp line 10 initialises:
 *              mApplicationDescription{appDescription}
 */

#ifndef ARA_LOG_SINK_LOG_SINK_H_
#define ARA_LOG_SINK_LOG_SINK_H_

#include <fstream>
#include <iostream>
#include <string>

#include "../log_stream.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            /**
             * @brief Abstract base for all log output sinks.
             *
             * Stores the application identity and provides protected helper
             * methods (GetTimestamp, GetAppstamp) and the cWhitespace constant
             * used by all concrete sink implementations.
             */
            class LogSink
            {
            public:
                /**
                 * @brief Construct with application identity.
                 *
                 * [FIX-1] Declaration only – definition is in log_sink.cpp.
                 * [FIX-4] Initialises mApplicationId (not mAppId).
                 * [FIX-5] Initialises mApplicationDescription (not mAppDescription).
                 *
                 * @param appId          Application identifier string.
                 * @param appDescription Human-readable application description.
                 */
                LogSink(std::string appId,
                        std::string appDescription);

                LogSink()                           = delete;
                virtual ~LogSink()                  = default;
                LogSink(const LogSink &)            = delete;
                LogSink &operator=(const LogSink &) = delete;
                LogSink(LogSink &&)                 = delete;
                LogSink &operator=(LogSink &&)      = delete;

                /**
                 * @brief Write a completed log message to the output.
                 *
                 * Declared const so implementations may be called via a
                 * const pointer/reference.
                 *
                 * @param logStream  The finalised log message.
                 */
                virtual void Log(const LogStream &logStream) const = 0;

            protected:
                /**
                 * @brief Return a LogStream containing the current
                 *        wall-clock timestamp.
                 *
                 * [FIX-2] Declaration only – definition is in log_sink.cpp.
                 *         noexcept removed to match the .cpp signature.
                 */
                LogStream GetTimestamp() const;

                /**
                 * @brief Return a LogStream containing the application
                 *        identity stamp.
                 *
                 * [FIX-3] Declaration only – definition is in log_sink.cpp.
                 *         noexcept removed to match the .cpp signature.
                 */
                LogStream GetAppstamp() const;

                /// Single whitespace separator used between log fields.
                static const char cWhitespace = ' ';

                /// [FIX-4] Application identifier (was mAppId).
                std::string mApplicationId;

                /// [FIX-5] Application description (was mAppDescription).
                std::string mApplicationDescription;
            };

        } // namespace sink
    } // namespace log
} // namespace ara

#endif // ARA_LOG_SINK_LOG_SINK_H_
