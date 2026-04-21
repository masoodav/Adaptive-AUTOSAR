#ifndef ARA_LOG_BACKEND_H_
#define ARA_LOG_BACKEND_H_

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "logger.h"

namespace ara
{
namespace log
{
namespace internal
{

struct RenderedArgument final
{
    std::string text;
};

struct MessageRecord final
{
    std::string ctx_id;
    std::string ctx_description;
    LogLevel level;
    std::vector<RenderedArgument> arguments;
    std::string file;
    int line;
    bool has_location;
    std::string tag;
    bool has_tag;
    std::uint8_t privacy;
    bool has_privacy;
};

struct LoggerState final
{
    LoggerState(const std::string& ctx, const std::string& description, LogLevel threshold_value) noexcept
        : ctx_id(ctx),
          ctx_description(description),
          threshold(threshold_value)
    {
    }

    std::string ctx_id;
    std::string ctx_description;
    LogLevel threshold;
};

struct LogStreamState final
{
    LogStreamState(const std::shared_ptr<LoggerState>& logger_state, LogLevel severity) noexcept
        : logger(logger_state),
          level(severity),
          line(0),
          has_location(false),
          has_tag(false),
          privacy(0U),
          has_privacy(false)
    {
    }

    std::shared_ptr<LoggerState> logger;
    LogLevel level;
    std::vector<std::string> arguments;
    std::string file;
    int line;
    bool has_location;
    std::string tag;
    bool has_tag;
    std::uint8_t privacy;
    bool has_privacy;
};

class Backend final
{
public:
    static Backend& Instance() noexcept;
    Backend() noexcept;

    Logger& CreateLogger(
        const std::string& ctx_id,
        const std::string& description,
        bool use_manifest_threshold,
        LogLevel explicit_threshold);
    Logger& CreateLogger(const std::string& instance_specifier);

    void Submit(MessageRecord message) noexcept;
    void FlushQueued() noexcept;

    bool IsEnabled(const std::shared_ptr<LoggerState>& state, LogLevel level) const noexcept;
    void SetThreshold(const std::shared_ptr<LoggerState>& state, LogLevel level) noexcept;

    void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept;
    void SetConnectionState(ClientState state) noexcept;

    void SetProcessingEnabled(bool enabled) noexcept;
    void SetQueueSize(std::size_t queue_size) noexcept;
    void ResetForTesting() noexcept;
    std::vector<std::string> SnapshotConsoleLines() const;

private:
    static std::string MakeConsoleLine(const MessageRecord& message);
    static LogLevel ResolveThreshold(bool use_manifest_threshold, LogLevel explicit_threshold) noexcept;
    static bool IsEnabledForThreshold(LogLevel threshold, LogLevel level) noexcept;

    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<LoggerState> > logger_states_;
    std::map<std::string, Logger> loggers_;
    std::map<std::string, LogLevel> manifest_thresholds_;
    std::deque<MessageRecord> queued_messages_;
    std::vector<std::string> console_lines_;
    std::size_t queue_size_;
    bool processing_enabled_;
    ClientState connection_state_;
    ConnectionStateHandler connection_handler_;
};

}  // namespace internal
}  // namespace log
}  // namespace ara

#endif  // ARA_LOG_BACKEND_H_
