#pragma once

#include <vulkan/vulkan.hpp>

#include "core/logger.hh"

#define VKCALL(Func)                                                           \
    do                                                                         \
    {                                                                          \
        if ((Func) != VK_SUCCESS)                                              \
        {                                                                      \
            Logger::error("Function " #Func " failed");                        \
            throw std::runtime_error("");                                      \
        }                                                                      \
    } while (false)
