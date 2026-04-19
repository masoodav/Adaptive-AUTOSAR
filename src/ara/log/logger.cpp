#include "logger.h"

#include <sstream>
#include <utility>

#include "log_stream.h"
#include "backend.h"

namespace ara
{
namespace log
{

Logger::Logger(std::shared_ptr<State> state) noexcept : state_(std::move(state)) {}

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
    return LogStream(std::shared_ptr<LogStream::State>(new LogStream::State(state_, logLevel)));
}

Logger& CreateLogger(const ara::core::InstanceSpecifier& is) noexcept
{
    static std::vector<std::unique_ptr<Logger> > owned_loggers;
    std::shared_ptr<Logger::State> state = internal::Backend::Instance().CreateLogger(is.ToString());

    for (const std::unique_ptr<Logger>& logger : owned_loggers)
    {
        if (logger->state_ == state)
        {
            return *logger;
        }
    }

    owned_loggers.emplace_back(new Logger(state));
    return *owned_loggers.back();
}

Logger& CreateLogger(ara::core::StringView ctxId, ara::core::StringView ctxDescription) noexcept
{
    static std::vector<std::unique_ptr<Logger> > owned_loggers;
    std::shared_ptr<Logger::State> state =
        internal::Backend::Instance().CreateLogger(ctxId.ToString(), ctxDescription.ToString(), true, LogLevel::kWarn);

    for (const std::unique_ptr<Logger>& logger : owned_loggers)
    {
        if (logger->state_ == state)
        {
            return *logger;
        }
    }

    owned_loggers.emplace_back(new Logger(state));
    return *owned_loggers.back();
}

Logger& CreateLogger(
    ara::core::StringView ctxId,
    ara::core::StringView ctxDescription,
    LogLevel ctxDefLogLevel) noexcept
{
    static std::vector<std::unique_ptr<Logger> > owned_loggers;
    std::shared_ptr<Logger::State> state = internal::Backend::Instance().CreateLogger(
        ctxId.ToString(), ctxDescription.ToString(), false, ctxDefLogLevel);

    for (const std::unique_ptr<Logger>& logger : owned_loggers)
    {
        if (logger->state_ == state)
        {
            return *logger;
        }
    }

    owned_loggers.emplace_back(new Logger(state));
    return *owned_loggers.back();
}

void RegisterConnectionStateHandler(ConnectionStateHandler callback) noexcept
{
    internal::Backend::Instance().RegisterConnectionStateHandler(std::move(callback));
}

namespace internal
{

Backend& Backend::Instance() noexcept
{
    static Backend backend;
    return backend;
}

Backend::Backend() noexcept
    : queue_size_(64U),
      processing_enabled_(true),
      connection_state_(ClientState::kNotConnected)
{
}

std::shared_ptr<Logger::State> Backend::CreateLogger(
    const std::string& ctx_id,
    const std::string& description,
    bool use_manifest_threshold,
    LogLevel explicit_threshold) noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, std::shared_ptr<Logger::State> >::iterator it = loggers_.find(ctx_id);
    if (it != loggers_.end())
    {
        return it->second;
    }

    std::shared_ptr<Logger::State> state(
        new Logger::State(ctx_id, description, ResolveThreshold(use_manifest_threshold, explicit_threshold)));
    loggers_.insert(std::make_pair(ctx_id, state));
    return state;
}

std::shared_ptr<Logger::State> Backend::CreateLogger(const std::string& instance_specifier) noexcept
{
    return CreateLogger(instance_specifier, instance_specifier, true, LogLevel::kWarn);
}

bool Backend::IsEnabled(const std::shared_ptr<Logger::State>& state, LogLevel level) const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return IsEnabledForThreshold(state->threshold, level);
}

void Backend::SetThreshold(const std::shared_ptr<Logger::State>& state, LogLevel level) noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    state->threshold = level;
}

void Backend::Submit(MessageRecord message) noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::map<std::string, std::shared_ptr<Logger::State> >::iterator it = loggers_.find(message.ctx_id);
    if (it == loggers_.end())
    {
        return;
    }

    if (!IsEnabledForThreshold(it->second->threshold, message.level))
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
    connection_handler_ = std::move(callback);
}

void Backend::SetConnectionState(ClientState state) noexcept
{
    ConnectionStateHandler callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_state_ = state;
        callback = connection_handler_;
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
    queued_messages_.clear();
    console_lines_.clear();
    manifest_thresholds_.clear();
    queue_size_ = 64U;
    processing_enabled_ = true;
    connection_state_ = ClientState::kNotConnected;
    connection_handler_ = ConnectionStateHandler();
}

std::vector<std::string> Backend::SnapshotConsoleLines() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return console_lines_;
}

std::string Backend::MakeConsoleLine(const MessageRecord& message)
{
    std::ostringstream stream;
    stream << '[' << message.ctx_id << "] ";
    stream << static_cast<unsigned int>(message.level);

    for (std::size_t index = 0U; index < message.arguments.size(); ++index)
    {
        stream << ' ' << message.arguments[index].text;
    }

    if (message.has_tag)
    {
        stream << " tag:" << message.tag;
    }
    if (message.has_location)
    {
        stream << " loc:" << message.file << ':' << message.line;
    }
    if (message.has_privacy)
    {
        stream << " privacy:" << static_cast<unsigned int>(message.privacy);
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
