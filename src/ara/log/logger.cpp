#include "logger.h"

#include <sstream>
#include <utility>

#include "log_stream.h"
#include "backend.h"

namespace ara
{
namespace log
{

namespace internal
{
namespace
{

Backend& GetBackendInstance() noexcept
{
    static Backend backend_instance;
    return backend_instance;
}

}  // namespace
}

Logger::Logger(std::shared_ptr<internal::LoggerState> state) noexcept : state_(std::move(state)) {}

Logger::~Logger() {}

bool Logger::IsEnabled(LogLevel logLevel) const noexcept
{
    return internal::Backend::Instance().IsEnabled(state_, logLevel);
}

LogStream Logger::LogDebug() const noexcept
{
    return WithLevel(LogLevel::kDebug);
}

LogStream Logger::LogError() const noexcept
{
    return WithLevel(LogLevel::kError);
}

LogStream Logger::LogFatal() const noexcept
{
    return WithLevel(LogLevel::kFatal);
}

LogStream Logger::LogInfo() const noexcept
{
    return WithLevel(LogLevel::kInfo);
}

LogStream Logger::LogVerbose() const noexcept
{
    return WithLevel(LogLevel::kVerbose);
}

LogStream Logger::LogWarn() const noexcept
{
    return WithLevel(LogLevel::kWarn);
}

void Logger::SetThreshold(LogLevel threshold) noexcept
{
    internal::Backend::Instance().SetThreshold(state_, threshold);
}

LogStream Logger::WithLevel(LogLevel logLevel) const noexcept
{
    try
    {
        return LogStream(std::make_shared<internal::LogStreamState>(state_, logLevel));
    }
    catch (...)
    {
        return LogStream();
    }
}

Logger& CreateLogger(const ara::core::InstanceSpecifier& is) noexcept
{
    try
    {
        return internal::Backend::Instance().CreateLogger(is.ToString());
    }
    catch (...)
    {
        return internal::Backend::Instance().CreateLogger("LOGF", "Fallback logger", false, LogLevel::kOff);
    }
}

Logger& CreateLogger(ara::core::StringView ctxId, ara::core::StringView ctxDescription) noexcept
{
    try
    {
        return internal::Backend::Instance().CreateLogger(
            ctxId.ToString(), ctxDescription.ToString(), true, LogLevel::kWarn);
    }
    catch (...)
    {
        return internal::Backend::Instance().CreateLogger("LOGF", "Fallback logger", false, LogLevel::kOff);
    }
}

Logger& CreateLogger(
    ara::core::StringView ctxId,
    ara::core::StringView ctxDescription,
    LogLevel ctxDefLogLevel) noexcept
{
    try
    {
        return internal::Backend::Instance().CreateLogger(
            ctxId.ToString(), ctxDescription.ToString(), false, ctxDefLogLevel);
    }
    catch (...)
    {
        return internal::Backend::Instance().CreateLogger("LOGF", "Fallback logger", false, LogLevel::kOff);
    }
}

void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept
{
    internal::Backend::Instance().RegisterConnectionStateHandler(std::move(callback));
}

namespace internal
{

Backend& Backend::Instance() noexcept
{
    return GetBackendInstance();
}

Backend::Backend() noexcept
    : queue_size_(64U),
      processing_enabled_(true),
      connection_state_(ClientState::kNotConnected)
{
}

Logger& Backend::CreateLogger(
    const std::string& ctx_id,
    const std::string& description,
    bool use_manifest_threshold,
    LogLevel explicit_threshold)
{
    try
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::map<std::string, Logger>::iterator logger_it = loggers_.find(ctx_id);
        if (logger_it != loggers_.end())
        {
            return logger_it->second;
        }

        std::shared_ptr<LoggerState> state = std::make_shared<LoggerState>(
            ctx_id, description, ResolveThreshold(use_manifest_threshold, explicit_threshold));
        logger_states_.insert(std::make_pair(ctx_id, state));
        std::pair<std::map<std::string, Logger>::iterator, bool> insert_result =
            loggers_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(ctx_id),
                             std::forward_as_tuple(state));
        return insert_result.first->second;
    }
    catch (...)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::string fallback_ctx_id("LOGF");
        std::map<std::string, Logger>::iterator fallback_it = loggers_.find(fallback_ctx_id);
        if (fallback_it == loggers_.end())
        {
            std::shared_ptr<LoggerState> fallback_state =
                std::make_shared<LoggerState>(fallback_ctx_id, "Fallback logger", LogLevel::kOff);
            logger_states_.insert(std::make_pair(fallback_ctx_id, fallback_state));
            fallback_it = loggers_.emplace(std::piecewise_construct,
                                           std::forward_as_tuple(fallback_ctx_id),
                                           std::forward_as_tuple(fallback_state))
                              .first;
        }
        return fallback_it->second;
    }
}

Logger& Backend::CreateLogger(const std::string& instance_specifier)
{
    return CreateLogger(instance_specifier, instance_specifier, true, LogLevel::kWarn);
}

bool Backend::IsEnabled(const std::shared_ptr<LoggerState>& state, LogLevel level) const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return IsEnabledForThreshold(state->threshold, level);
}

void Backend::SetThreshold(const std::shared_ptr<LoggerState>& state, LogLevel level) noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    state->threshold = level;
}

void Backend::Submit(MessageRecord message) noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::map<std::string, std::shared_ptr<LoggerState> >::iterator state_it = logger_states_.find(message.ctx_id);
    if (state_it == logger_states_.end())
    {
        return;
    }

    if (!IsEnabledForThreshold(state_it->second->threshold, message.level))
    {
        return;
    }

    if (!processing_enabled_)
    {
        if (queued_messages_.size() >= queue_size_)
        {
            queued_messages_.pop_front();
        }
        queued_messages_.push_back(message);
        return;
    }

    console_lines_.push_back(MakeConsoleLine(message));
}

void Backend::FlushQueued() noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queued_messages_.empty())
    {
        console_lines_.push_back(MakeConsoleLine(queued_messages_.front()));
        queued_messages_.pop_front();
    }
}

void Backend::RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    static_cast<void>(connection_handler_ = std::move(callback));
}

void Backend::SetConnectionState(ClientState state) noexcept
{
    ConnectionStateHandler callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_state_ = state;
        static_cast<void>(callback = connection_handler_);
    }

    if (callback)
    {
        callback(state);
    }
}

void Backend::SetProcessingEnabled(bool enabled) noexcept
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        processing_enabled_ = enabled;
    }
    if (enabled)
    {
        FlushQueued();
    }
}

void Backend::SetQueueSize(std::size_t queue_size) noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_size_ = queue_size;
}

void Backend::ResetForTesting() noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    loggers_.clear();
    logger_states_.clear();
    queued_messages_.clear();
    console_lines_.clear();
    manifest_thresholds_.clear();
    queue_size_ = 64U;
    processing_enabled_ = true;
    connection_state_ = ClientState::kNotConnected;
    static_cast<void>(connection_handler_ = ConnectionStateHandler());
}

std::vector<std::string> Backend::SnapshotConsoleLines() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return console_lines_;
}

std::string Backend::MakeConsoleLine(const MessageRecord& message)
{
    std::ostringstream stream;
    static_cast<void>(stream << '[' << message.ctx_id << "] ");
    static_cast<void>(stream << static_cast<unsigned int>(message.level));

    std::vector<RenderedArgument>::size_type argument_index = 0U;
    const std::vector<RenderedArgument>::size_type argument_count = message.arguments.size();
    while (argument_index < argument_count)
    {
        static_cast<void>(stream << ' ' << message.arguments[argument_index].text);
        ++argument_index;
    }

    if (message.has_tag)
    {
        static_cast<void>(stream << " tag:" << message.tag);
    }
    if (message.has_location)
    {
        static_cast<void>(stream << " loc:" << message.file << ':' << message.line);
    }
    if (message.has_privacy)
    {
        static_cast<void>(stream << " privacy:" << static_cast<unsigned int>(message.privacy));
    }

    return stream.str();
}

LogLevel Backend::ResolveThreshold(bool use_manifest_threshold, LogLevel explicit_threshold) noexcept
{
    if (!use_manifest_threshold)
    {
        return explicit_threshold;
    }
    return LogLevel::kWarn;
}

bool Backend::IsEnabledForThreshold(LogLevel threshold, LogLevel level) noexcept
{
    if ((threshold == LogLevel::kOff) || (level == LogLevel::kOff))
    {
        return false;
    }

    return static_cast<std::uint8_t>(level) <= static_cast<std::uint8_t>(threshold);
}

}  // namespace internal

}  // namespace log
}  // namespace ara
