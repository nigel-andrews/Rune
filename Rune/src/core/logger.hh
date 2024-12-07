#pragma once

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <source_location>

namespace Rune::Logger
{
    enum Level
    {
        FATAL = 0,
        INFO,
        WARN,
        ERROR,
    };

    // TODO: color output
    template <typename... Ts>
    constexpr void log(Level log_level, Ts&&... args)
    {
#if defined(DEBUG) || !defined(NDEBUG)
        static constexpr std::array string_repr{
            "FATAL",
            "INFO",
            "WARN",
            "ERROR",
        };
        std::print(std::clog, "[{}]:", string_repr[log_level]);
        ((std::clog << " " << std::forward<Ts>(args)), ...) << std::endl;
#endif
    }

    [[noreturn]] constexpr void
    abort(std::string_view message,
          std::source_location location = std::source_location::current())
    {
        log(FATAL,
            std::format("{}({}:{}): {}", location.file_name(), location.line(),
                        location.column(), message));
        std::abort();
    }
} // namespace Rune::Logger
