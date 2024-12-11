#pragma once

#include <string>

#include "utils/types.hh"

namespace Rune
{
    struct AppConfig
    {
        std::string name = "Application";
        i32 width = 800;
        i32 height = 600;
    };
} // namespace Rune
