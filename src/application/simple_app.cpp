#include <json/json.h>
#include "../ara/com/someip/sd/sd_network_layer.h"
#include "../ara/diag/conversation.h"
#include "../application/helper/argument_configuration.h"
#include "./simple_app.h"
#include <thread>
#include <chrono>

namespace application
{
    const std::string SimpleApp::cAppId{"SimpleApp"};
    const ara::core::InstanceSpecifier SimpleApp::cSeInstance{"SimpleAppSE"};

    SimpleApp::SimpleApp(
        AsyncBsdSocketLib::Poller *poller,
        ara::phm::CheckpointCommunicator *checkpointCommunicator) : ara::exec::helper::ModelledProcess(cAppId, poller),
                                                                    mSupervisedEntity{cSeInstance, checkpointCommunicator}    {
    }

    // [Commented out configuration methods omitted for brevity]

    int SimpleApp::Main(
        const std::atomic_bool *cancellationToken,
        const std::map<std::string, std::string> &arguments)
    {
        // FIX: Removed manual LogStream variable
        // ara::log::LogStream _logStream;

        try
        {
            bool _running{true};
            uint32_t counter{0};

            // FIX: Standard API usage
            mLogger.LogInfo() << "SimpleApp AA has been initialized.";

            while (!cancellationToken->load() && _running)
            {
                mSupervisedEntity.ReportCheckpoint(
                    SmpCheckpointType::AliveCheckpoint);
                mSupervisedEntity.ReportCheckpoint(
                    SmpCheckpointType::DeadlineSourceCheckpoint);

                _running = WaitForActivation();

                counter++;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                // FIX: Standard API usage
                mLogger.LogInfo() << "Cycle " << counter << ": Application is running." << "\n";

                mSupervisedEntity.ReportCheckpoint(
                    SmpCheckpointType::DeadlineTargetCheckpoint);
            }

            // FIX: Standard API usage
            mLogger.LogInfo() << "SimpleApp AA has been terminated.";

            return cSuccessfulExitCode;
        }
        catch (const std::runtime_error &ex)
        {
            // FIX: Standard API usage
            mLogger.LogError() << ex.what();

            return cUnsuccessfulExitCode;
        }
    }

    SimpleApp::~SimpleApp()
    {
    }
}