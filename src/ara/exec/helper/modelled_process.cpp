#include "./modelled_process.h"
#include <stdexcept>

namespace ara
{
    namespace exec
    {
        namespace helper
        {
            // Static member initialization
            const log::LogMode ModelledProcess::cLogMode{log::LogMode::kConsole};
            const std::string ModelledProcess::cContextId{"Lifetime"};
            const std::string ModelledProcess::cContextDescription{"Application lifetime logs"};
            const ara::log::LogLevel ModelledProcess::cLogLevel{ara::log::LogLevel::kInfo};
            const ara::log::LogLevel ModelledProcess::cErrorLevel{ara::log::LogLevel::kError};

            ModelledProcess::ModelledProcess(
                std::string appId,
                AsyncBsdSocketLib::Poller *poller,
                ara::log::LogLevel cLogLevel)
                : mPoller(poller)
                , mLoggingFramework(log::LoggingFramework::Create(appId, cLogMode))
                , mLogger(const_cast<log::Logger*>(&mLoggingFramework->CreateLogger(cContextId, cContextDescription, cLogLevel)))
            {
                if (!mPoller) {
                    throw std::invalid_argument("Poller cannot be null");
                }
            }

            void ModelledProcess::Initialize(const std::map<std::string, std::string> &arguments)
            {
                if (!mExitCode.valid()) {
                    mExitCode = std::async(
                        std::launch::async,
                        &ModelledProcess::Main,
                        this,
                        &mCancellationToken,
                        arguments
                    );
                }
            }

            int ModelledProcess::Terminate()
            {
                int result = cSuccessfulExitCode;

                if (mExitCode.valid()) {
                    mCancellationToken = true;
                    result = mExitCode.get();
                }

                return result;
            }

            void ModelledProcess::Log(ara::log::LogLevel logLevel, const ara::log::LogStream &logStream)
            {
                mLoggingFramework->Log(*mLogger, logLevel, logStream);
            }

            bool ModelledProcess::WaitForActivation()
            {
                auto activationResult = mDeterministicClient.WaitForActivation();
                return activationResult.Value() != exec::ActivationReturnType::kTerminate;
            }

            ModelledProcess::~ModelledProcess()
            {
                Terminate();
                delete mLoggingFramework;
            }
        }
    }
}
