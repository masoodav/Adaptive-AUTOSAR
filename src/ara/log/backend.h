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

class Backend final
{
public:
    static Backend& Instance() noexcept;

    std::shared_ptr<Logger::State> CreateLogger(
        const std::string& ctx_id,
        const std::string& description,
        bool use_manifest_threshold,
        LogLevel explicit_threshold) noexcept;
    std::shared_ptr<Logger::State> CreateLogger(const std::string& instance_specifier) noexcept;

    void Submit(MessageRecord message) noexcept;
    void FlushQueued() noexcept;

    bool IsEnabled(const std::shared_ptr<Logger::State>& state, LogLevel level) const noexcept;
    void SetThreshold(const std::shared_ptr<Logger::State>& state, LogLevel level) noexcept;

    void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept;
    void SetConnectionState(ClientState state) noexcept;

    void SetProcessingEnabled(bool enabled) noexcept;
    void SetQueueSize(std::size_t queue_size) noexcept;
    void ResetForTesting() noexcept;
    std::vector<std::string> SnapshotConsoleLines() const;

private:
    Backend() noexcept;

    static std::string MakeConsoleLine(const MessageRecord& message);
    static LogLevel ResolveThreshold(bool use_manifest_threshold, LogLevel explicit_threshold) noexcept;
    static bool IsEnabledForThreshold(LogLevel threshold, LogLevel level) noexcept;

    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<Logger::State> > loggers_;
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
