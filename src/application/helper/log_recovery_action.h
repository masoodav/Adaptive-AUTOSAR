/**
 * @file log_recovery_action.h
 *
 * Fix applied:
 *   [FIX] Changed  ara::log::Logger mLogger;
 *         to       const ara::log::Logger &mLogger;
 *
 *   Root cause: LoggingFramework::CreateLogger() returns const Logger&.
 *   Binding a const Logger& to a Logger value member invokes the copy
 *   constructor, which is = delete per [SWS_LOG_00172]:
 *     "error: use of deleted function 'ara::log::Logger::Logger(const ara::log::Logger&)'"
 *
 *   A const reference member binds directly to the returned const Logger&
 *   without any copy or move — no copy constructor is needed.
 *
 *   The constructor initialiser in log_recovery_action.cpp is unchanged:
 *     mLogger{mLoggingFramework->CreateLogger(...)}
 *   now correctly initialises a const reference member.
 *
 *   Lifetime is safe: mLoggingFramework is declared before mLogger in the
 *   class body, so it is constructed first and destroyed last, guaranteeing
 *   the Logger (owned by the framework) is always alive while mLogger exists.
 */

#ifndef LOG_RECOVERY_ACTION_H
#define LOG_RECOVERY_ACTION_H

#include "../../ara/phm/recovery_action.h"
#include "../../ara/log/logging_framework.h"

namespace application
{
    namespace helper
    {
        /// @brief A class to provide logging as the recovery action for a failed supervised entity
        class LogRecoveryAction : public ara::phm::RecoveryAction
        {
        private:
            static const ara::core::InstanceSpecifier cInstance;
            static const ara::log::LogMode cLogMode;
            static const std::string cContextId;
            static const std::string cContextDescription;
            static const ara::log::LogLevel cErrorLevel;

            ara::log::LoggingFramework *mLoggingFramework;
            const ara::log::Logger &mLogger;  // [FIX] was: ara::log::Logger mLogger;

        public:
            LogRecoveryAction();
            ~LogRecoveryAction() override;

            void RecoveryHandler(
                const ara::exec::ExecutionErrorEvent &executionError,
                ara::phm::TypeOfSupervision supervision) override;
        };
    }
}

#endif
