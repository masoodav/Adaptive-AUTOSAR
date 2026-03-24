#ifndef ARALOG_COMMON_H
#define ARALOG_COMMON_H

#include "../core/result.h"
#include "../core/string_view.h"
#include "../core/error_code.h"
#include "../core/error_domain.h"
#include <type_traits>

namespace ara::log {

/// \brief Log mode enumeration
enum class LogMode {
    kConsole,  ///< Log to console
    kFile     ///< Log to file
};

/// \brief Client connection state
enum class ClientState : std::int8_t {
    kUnknown = -1,    ///< DLT backend not up and running yet
    kNotConnected = 0, ///< No remote client detected
    kConnected = 1     ///< Remote client is connected
};

/// \brief Log severity levels
enum class LogLevel : std::uint8_t {
    kOff = 0x00,      ///< No logging
    kFatal = 0x01,     ///< Fatal error, not recoverable
    kError = 0x02,     ///< Error with impact to correct functionality
    kWarn = 0x03,      ///< Warning if correct behavior cannot be ensured
    kInfo = 0x04,      ///< Informational, providing high level understanding
    kDebug = 0x05,     ///< Detailed information for programmers
    kVerbose = 0x06    ///< Extra-verbose debug messages
};

/// \brief Converts LogLevel to StringView for display
inline ara::core::StringView ToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::kOff:      return ara::core::StringView{"Off"};
        case LogLevel::kFatal:    return ara::core::StringView{"Fatal"};
        case LogLevel::kError:    return ara::core::StringView{"Error"};
        case LogLevel::kWarn:     return ara::core::StringView{"Warn"};
        case LogLevel::kInfo:     return ara::core::StringView{"Info"};
        case LogLevel::kDebug:    return ara::core::StringView{"Debug"};
        case LogLevel::kVerbose:  return ara::core::StringView{"Verbose"};
        default:                  return ara::core::StringView{"Unknown"};
    }
}

/// \brief Error codes for Log and Trace module
enum class LogError : ara::core::ErrorDomain::CodeType {
    kBufferFull = 1,
    kInvalidArgument,
    kNotInitialized,
    kAlreadyInitialized
};

/// \brief Error domain for Log and Trace
class LogErrorDomain final : public ara::core::ErrorDomain {
public:
    static constexpr CodeType kBufferFull = static_cast<CodeType>(LogError::kBufferFull);
    static constexpr CodeType kInvalidArgument = static_cast<CodeType>(LogError::kInvalidArgument);
    static constexpr CodeType kNotInitialized = static_cast<CodeType>(LogError::kNotInitialized);
    static constexpr CodeType kAlreadyInitialized = static_cast<CodeType>(LogError::kAlreadyInitialized);

    constexpr LogErrorDomain() noexcept
        : ErrorDomain(IdType{0x80000000}) {}

    static constexpr IdType Id() noexcept { return IdType{0x80000000}; }

    const char* Name() const noexcept override { return "LogErrorDomain"; }

    const char* Message(CodeType errorCode) const noexcept override {
        switch (static_cast<LogError>(errorCode)) {
            case LogError::kBufferFull: return "Log buffer full";
            case LogError::kInvalidArgument: return "Invalid log argument";
            case LogError::kNotInitialized: return "Log not initialized";
            case LogError::kAlreadyInitialized: return "Log already initialized";
            default: return "Unknown log error";
        }
    }
};

/// \brief Global error domain instance
extern const LogErrorDomain g_LogErrorDomain;

} // namespace ara::log

#endif // ARALOG_COMMON_H
