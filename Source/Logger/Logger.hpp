#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <atomic>
#include <array>
#include <thread>
#include <format>
#include <string_view>
#include <cstdint>
#include <cstring>

namespace FastLog {

    enum class Level : uint8_t {
        Info,
        Warning,
        Error,
        Debug
    };

    class Logger {
    public:
        static Logger& instance();
        void start() noexcept;
        void stop() noexcept;

        template<Level L, typename... Args>
        inline void log(std::format_string<Args...> fmt, Args&&... args) noexcept {
            constexpr bool compile_debug_enabled =
#if defined(_DEBUG)
                true;
#else
                false;
#endif
            thread_local char buffer[16384];
            auto res = std::format_to_n(buffer, sizeof(buffer) - 1, fmt, std::forward<Args>(args)...);
            size_t size = res.out - buffer;

            if constexpr (L == Level::Debug) {
#if defined(_WIN32)
                if (!compile_debug_enabled && !IsDebuggerPresent()) return;
#else
                if (!compile_debug_enabled) return;
#endif
            }

            push(L, buffer, size);
        }

        static void set_thread_name(const char* name) noexcept;

    private:
        Logger() noexcept;
        ~Logger();
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void push(Level lvl, const char* data, size_t size) noexcept;
        void writer_loop() noexcept;

        static constexpr size_t QUEUE_SIZE = 1 << 15;
        static constexpr size_t MAX_MSG_SIZE = 16384;

        struct alignas(64) Slot {
            std::atomic<bool> ready{ false };
            Level level;
            uint32_t size;
            uint64_t timestamp;
            char thread_name[16]{};
            char data[MAX_MSG_SIZE];
        };

        std::array<Slot, QUEUE_SIZE> queue_;
        alignas(64) std::atomic<uint32_t> write_index_{ 0 };
        alignas(64) std::atomic<uint32_t> read_index_{ 0 };
        std::atomic<bool> running_{ false };
        std::thread writer_;

        static char*& thread_name_ptr() noexcept;
    };

#define LOG_INFO(...)  FastLog::Logger::instance().log<FastLog::Level::Info>(__VA_ARGS__)
#define LOG_WARN(...)  FastLog::Logger::instance().log<FastLog::Level::Warning>(__VA_ARGS__)
#define LOG_ERROR(...) FastLog::Logger::instance().log<FastLog::Level::Error>(__VA_ARGS__)
#define LOG_DEBUG(...) FastLog::Logger::instance().log<FastLog::Level::Debug>(__VA_ARGS__)

} // namespace FastLog