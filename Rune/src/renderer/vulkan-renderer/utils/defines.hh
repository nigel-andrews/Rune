#pragma once

#include <vulkan/vulkan.hpp>

#include "core/logger/logger.hh"

#define VKASSERT(Expr, Message)                                                \
    do                                                                         \
    {                                                                          \
        if (auto result = (Expr); result != vk::Result::eSuccess)              \
            Logger::abort((Message));                                          \
    } while (false)

#define VKERROR(Expr, Message)                                                 \
    do                                                                         \
    {                                                                          \
        if (auto result = (Expr); result != vk::Result::eSuccess)              \
            Logger::error((Message));                                          \
    } while (false)

#define VKWARN(Expr, Message)                                                  \
    do                                                                         \
    {                                                                          \
        if (auto result = (Expr); result != vk::Result::eSuccess)              \
            Logger::log(Logger::WARN, (Message));                              \
    } while (false)
