#pragma once

#include <oxide/compiler/location.hpp>
#include <oxide/fmt/format.hpp>
#include <oxide/panic.hpp>

namespace oxide {

class LogLevel {
public:
    enum struct Type : u8 {
        TRACE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };

    constexpr LogLevel() = default;

    constexpr LogLevel(Type type) noexcept : _type(type) {}

    static constexpr LogLevel trace() noexcept { return LogLevel(Type::TRACE); }

    static constexpr LogLevel debug() noexcept { return LogLevel(Type::DEBUG); }

    static constexpr LogLevel info() noexcept { return LogLevel(Type::INFO); }

    static constexpr LogLevel warn() noexcept { return LogLevel(Type::WARN); }

    static constexpr LogLevel error() noexcept { return LogLevel(Type::ERROR); }

    static constexpr LogLevel fatal() noexcept { return LogLevel(Type::FATAL); }

    constexpr Type type() const noexcept { return _type; }

    std::string to_string() const noexcept {
        // clang-format off
        switch (_type) {
            case Type::TRACE: return "TRACE";
            case Type::DEBUG: return "DEBUG";
            case Type::INFO: return "INFO";
            case Type::WARN: return "WARN";
            case Type::ERROR: return "ERROR";
            case Type::FATAL: return "FATAL";
        }
        // clang-format on
        OXIDE_UNREACHABLE();
    }

private:
    Type _type{Type::INFO};
};

struct Logger {
    Logger() = default;
    Logger(const Logger&) = default;
    Logger(Logger&&) = default;
    Logger& operator=(const Logger&) = default;
    Logger& operator=(Logger&&) = default;

    virtual ~Logger() = default;

    virtual void trace(const char* msg) noexcept = 0;
    virtual void debug(const char* msg) noexcept = 0;
    virtual void info(const char* msg) noexcept = 0;
    virtual void warn(const char* msg) noexcept = 0;
    virtual void error(const char* msg) noexcept = 0;
    virtual void fatal(const char* msg) noexcept = 0;

    template <typename... Args>
    void trace(oxide::fmt::FormatString<Args...> fmt, Args&&... args) {
        auto msg = oxide::fmt::format(fmt, std::forward<Args>(args)...);
        msg.push('\0');
        trace(msg.data());
    }

    template <typename... Args>
    void debug(oxide::fmt::FormatString<Args...> fmt, Args&&... args) {
        auto msg = oxide::fmt::format(fmt, std::forward<Args>(args)...);
        msg.push('\0');
        debug(msg.data());
    }

    template <typename... Args>
    void info(oxide::fmt::FormatString<Args...> fmt, Args&&... args) {
        auto msg = oxide::fmt::format(fmt, std::forward<Args>(args)...);
        msg.push('\0');
        info(msg.data());
    }

    template <typename... Args>
    void warn(oxide::fmt::FormatString<Args...> fmt, Args&&... args) {
        auto msg = oxide::fmt::format(fmt, std::forward<Args>(args)...);
        msg.push('\0');
        warn(msg.data());
    }

    template <typename... Args>
    void error(oxide::fmt::FormatString<Args...> fmt, Args&&... args) {
        auto msg = oxide::fmt::format(fmt, std::forward<Args>(args)...);
        msg.push('\0');
        error(msg.data());
    }

    template <typename... Args>
    void fatal(oxide::fmt::FormatString<Args...> fmt, Args&&... args) {
        auto msg = oxide::fmt::format(fmt, std::forward<Args>(args)...);
        msg.push('\0');
        fatal(msg.data());
    }

    virtual void set_log_level(LogLevel log_level) noexcept = 0;
    virtual LogLevel log_level() noexcept = 0;
};

LogLevel get_log_level() noexcept;
void set_log_level(LogLevel log_level) noexcept;

void set_logger(Logger& logger) noexcept;
Logger& get_logger() noexcept;

void set_log_level_from_env_or_default() noexcept;
void set_log_level_from_env_or(LogLevel fallback) noexcept;

extern LogLevel OXIDE_DEFAULT_LOG_LEVEL;

namespace detail {

template <typename... Args>
void
log(void (Logger::*write)(const char*) noexcept,
    const SourceLocation& location,
    oxide::fmt::FormatString<Args...> format_string,
    Args&&... args) {
    const auto message = oxide::fmt::format(format_string, std::forward<Args>(args)...);
    auto output = oxide::fmt::format("{}:{} {}", location.file_name(), location.line(), message);
    output.push('\0');
    (get_logger().*write)(output.data());
}

} // namespace detail

#define OXIDE_LOG(level, ...) \
    ::oxide::detail::log( \
        static_cast<void (::oxide::Logger::*)(const char*) noexcept>(&::oxide::Logger::level), \
        ::oxide::SourceLocation::current(), \
        __VA_ARGS__ \
    )

#define OXIDE_TRACE(...) OXIDE_LOG(trace, __VA_ARGS__)
#define OXIDE_DEBUG(...) OXIDE_LOG(debug, __VA_ARGS__)
#define OXIDE_INFO(...) OXIDE_LOG(info, __VA_ARGS__)
#define OXIDE_WARN(...) OXIDE_LOG(warn, __VA_ARGS__)
#define OXIDE_ERROR(...) OXIDE_LOG(error, __VA_ARGS__)
#define OXIDE_FATAL(...) OXIDE_LOG(fatal, __VA_ARGS__)

} // namespace oxide
