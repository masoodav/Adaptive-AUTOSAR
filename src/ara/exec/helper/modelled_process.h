#ifndef MODELLED_PROCESS_H
#define MODELLED_PROCESS_H

#include <asyncbsdsocket/poller.h>
#include <atomic>
#include <future>
#include <map>
#include <string>
#include "../../log/logging_framework.h"
#include "../deterministic_client.h"

namespace ara
{
    namespace exec
    {
        namespace helper
        {
            /// @brief A class that models an instance of an Adaptive (Platform) Application executable
            class ModelledProcess
            {
            private:
                static const log::LogMode cLogMode;
                static const std::string cContextId;
                static const std::string cContextDescription;

                log::LoggingFramework* mLoggingFramework;
                DeterministicClient mDeterministicClient;
                std::atomic_bool mCancellationToken;
                std::future<int> mExitCode;

            protected:
                log::Logger *mLogger;
                /// @brief Information severity log level
                static const ara::log::LogLevel cLogLevel;
                /// @brief Error severity log level
                static const ara::log::LogLevel cErrorLevel;

                /// @brief Successful application exit code
                const int cSuccessfulExitCode{0};
                /// @brief Unsuccessful application exit code
                const int cUnsuccessfulExitCode{1};

                /// @brief Global poller for TCP/IP network communication
                AsyncBsdSocketLib::Poller *const mPoller;

            public:
                // Public constructor
                ModelledProcess(
                    std::string appId,
                    AsyncBsdSocketLib::Poller *poller,
                    ara::log::LogLevel cLogLevel = ara::log::LogLevel::kInfo);

                /// @brief Initialize the process model to run the main block
                void Initialize(const std::map<std::string, std::string> &arguments);

                /// @brief Terminate the process model
                int Terminate();

                virtual ~ModelledProcess();

            protected:
                /// @brief Main running block of the process
                virtual int Main(
                    const std::atomic_bool *cancellationToken,
                    const std::map<std::string, std::string> &arguments) = 0;

                /// @brief Log a stream
                void Log(
                    ara::log::LogLevel logLevel,
                    const ara::log::LogStream &logStream);

                /// @brief Wait for the next main function activation cycle
                bool WaitForActivation();
            };
        }
    }
}

#endif
