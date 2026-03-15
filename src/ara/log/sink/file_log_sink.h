/**
 * @file file_log_sink.h
 * @brief File log sink.
 *
 * Matches the ara::log::sink::FileLogSink interface from the
 * Adaptive-AUTOSAR project (masoodav/Adaptive-AUTOSAR, branch claude_dev).
 * C++14 compliant.
 *
 * Fixes applied vs. previous version:
 *
 *   [FIX-1] Renamed private member mFilePath → mLogFilePath.
 *           file_log_sink.cpp line 13 initialises:
 *             mLogFilePath{logFilePath}
 *           The previous stub used 'mFilePath', causing:
 *           "class FileLogSink does not have any field named 'mLogFilePath'"
 *           and the cascading "'mLogFilePath' was not declared in this scope".
 *
 *   [FIX-2] Added #include <fstream>.
 *           file_log_sink.cpp line 27 uses std::ofstream::out and
 *           std::ofstream::app. The .cpp has no #include <fstream> of its
 *           own so std::ofstream was an incomplete type, causing:
 *           "variable 'std::ofstream' has initializer but incomplete type"
 *           "incomplete type 'std::ofstream' used in nested name specifier"
 */

#ifndef ARA_LOG_SINK_FILE_LOG_SINK_H_
#define ARA_LOG_SINK_FILE_LOG_SINK_H_

#include <fstream>    /* [FIX-2] – provides std::ofstream */
#include <string>
#include "log_sink.h"

namespace ara
{
    namespace log
    {
        namespace sink
        {
            /**
             * @brief Writes log messages to a local file.
             */
            class FileLogSink final : public LogSink
            {
            public:
                /**
                 * @param filePath       Destination log file path.
                 * @param appId          Application identifier string.
                 * @param appDescription Human-readable application description.
                 *
                 * Declaration only – definition lives in file_log_sink.cpp.
                 */
                FileLogSink(std::string filePath,
                            std::string appId,
                            std::string appDescription);

                ~FileLogSink() = default;

                /**
                 * @brief Write a log message to the configured file.
                 *
                 * Declared const to match the definition in file_log_sink.cpp.
                 *
                 * @param logStream  The completed log message.
                 */
                void Log(const LogStream &logStream) const override;

            private:
                std::string mLogFilePath; /* [FIX-1] was mFilePath */
            };

        } // namespace sink
    } // namespace log
} // namespace ara

#endif // ARA_LOG_SINK_FILE_LOG_SINK_H_
