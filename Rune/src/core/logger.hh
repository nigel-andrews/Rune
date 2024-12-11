#pragma once

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <source_location>

namespace Rune::Logger
{
    namespace
    {
        constexpr std::string
        filename_from_location(const std::source_location& location)
        {
            return std::filesystem::path(location.file_name())
                .filename()
                .c_str();
        }
    } // namespace

    enum Level
    {
        FATAL = 0,
        INFO,
        WARN,
        ERROR,
    };

    template <typename... Ts>
    constexpr void log(Level log_level [[maybe_unused]],
                       Ts&&... args [[maybe_unused]])
    {
#if defined(DEBUG) || !defined(NDEBUG)
        static constexpr std::array string_repr{
            // TODO: extract escapes for easier use
            "\033[7;31m[FATAL]\033[0m",
            "\033[0;32m[INFO]\033[0m",
            "\033[0;33m[WARN]\033[0m",
            "\033[0;31m[ERROR]\033[0m",
        };
        std::print(std::clog, "{}:", string_repr[log_level]);
        ((std::clog << " " << std::forward<Ts>(args)), ...) << std::endl;
#endif
    }

    [[noreturn]] constexpr void
    abort(std::string_view message,
          std::source_location location = std::source_location::current())
    {
        log(FATAL,
            std::format("{}({}:{}): {}", filename_from_location(location),
                        location.line(), location.column(), message));
        std::abort();
    }

    constexpr void
    error(std::string_view message,
          std::source_location location = std::source_location::current())
    {
        log(ERROR,
            std::format("{}({}:{}): {}", filename_from_location(location),
                        location.line(), location.column(), message));
    }
} // namespace Rune::Logger
