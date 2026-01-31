#include "./log_recovery_action.h"

namespace application
{
    namespace helper
    {
        const ara::core::InstanceSpecifier LogRecoveryAction::cInstance{"LogRecoveryAction"};
        const ara::log::LogMode LogRecoveryAction::cLogMode{ara::log::LogMode::kConsole};
        const std::string LogRecoveryAction::cContextId{"RecoveryAction"};
        const std::string LogRecoveryAction::cContextDescription{"Recovery action logs"};
        const ara::log::LogLevel LogRecoveryAction::cErrorLevel{ara::log::LogLevel::kError};

        LogRecoveryAction::LogRecoveryAction() : ara::phm::RecoveryAction(cInstance),
                                                 mLoggingFramework{ara::log::LoggingFramework::Create(cInstance.ToString(), cLogMode)},
                                                 // FIX: Initialize the reference directly from the framework
                                                 mLogger{mLoggingFramework->CreateLogger(cContextId, cContextDescription, cErrorLevel)}
        {
        }

        void LogRecoveryAction::RecoveryHandler(
            const ara::exec::ExecutionErrorEvent &executionError,
            ara::phm::TypeOfSupervision supervision)
        {
            const ara::exec::ExecutionError cExtendedVehicleExpiration{0};

            if (IsOffered() &&
                executionError.executionError == cExtendedVehicleExpiration)
            {
                // FIX: Use standard API. logStream is a temporary wrapper here.
                auto logStream = mLogger.LogError();

                switch (supervision)
                {
                case ara::phm::TypeOfSupervision::AliveSupervision:
                    logStream << "Alive supervision";
                    break;

                case ara::phm::TypeOfSupervision::DeadlineSupervision:
                    logStream << "Deadline supervision";
                    break;

                default:
                    // Unsupported supervision type
                    return;
                }

                logStream << " is expired on "
                          << executionError.functionGroup->GetInstance().ToString();
                
                // logStream destructor calls Flush() automatically here
            }
        }

        LogRecoveryAction::~LogRecoveryAction()
        {
            delete mLoggingFramework;
        }
    }
}