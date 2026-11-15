#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string_view>

#include <oxide/log/logger.hpp>

namespace oxide {

namespace {

class ConsoleLogger final : public Logger {
public:
    ~ConsoleLogger() override = default;

    static ConsoleLogger& default_logger() noexcept {
        static ConsoleLogger logger;
        return logger;
    }

    void trace(const char* msg) noexcept override { write("TRACE", msg); }

    void debug(const char* msg) noexcept override { write("DEBUG", msg); }

    void info(const char* msg) noexcept override { write("INFO", msg); }

    void warn(const char* msg) noexcept override { write("WARN", msg); }

    void error(const char* msg) noexcept override { write("ERROR", msg); }

    void fatal(const char* msg) noexcept override { write("FATAL", msg); }

    void set_log_level(LogLevel log_level) noexcept override {
        _log_level.store(log_level, std::memory_order_relaxed);
    }

    LogLevel log_level() noexcept override { return _log_level.load(std::memory_order_relaxed); }

private:
    static void write(const char* level, const char* msg) noexcept {
        const auto now = std::time(nullptr);
        std::tm local_time{};
        char timestamp[] = "--:--:--";
        const bool has_local_time = localtime_r(&now, &local_time) != nullptr;
        if (has_local_time) {
            std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &local_time);
        }
        std::printf("%s %s %s\n", timestamp, level, msg);
    }

    std::atomic<LogLevel> _log_level{LogLevel::info()};
};

Logger* logger = nullptr;

} // namespace

LogLevel OXIDE_DEFAULT_LOG_LEVEL = LogLevel::info();

void set_log_level(LogLevel log_level) noexcept {
    get_logger().set_log_level(log_level);
}

LogLevel get_log_level() noexcept {
    return get_logger().log_level();
}

Logger& get_logger() noexcept {
    return logger != nullptr ? *logger : ConsoleLogger::default_logger();
}

void set_logger(Logger& logger) noexcept {
    oxide::logger = &logger;
}

void set_log_level_from_env_or(LogLevel fallback) noexcept {
    const char* result = std::getenv("OXIDE_LOG_LEVEL");
    if (result != nullptr) {
        const std::string_view env = result;
        // clang-format off
        if (env == LogLevel::trace().to_string()) { set_log_level(LogLevel::trace()); return; }
        if (env == LogLevel::debug().to_string()) { set_log_level(LogLevel::debug()); return; }
        if (env == LogLevel::info().to_string())  { set_log_level(LogLevel::info());  return; }
        if (env == LogLevel::warn().to_string())  { set_log_level(LogLevel::warn());  return; }
        if (env == LogLevel::error().to_string()) { set_log_level(LogLevel::error()); return; }
        if (env == LogLevel::fatal().to_string()) { set_log_level(LogLevel::fatal()); return; }
        // clang-format on
    }

    set_log_level(fallback);
}

void set_log_level_from_env_or_default() noexcept {
    set_log_level_from_env_or(OXIDE_DEFAULT_LOG_LEVEL);
}

} // namespace oxide
