#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

#if defined(DEBUG) || !defined(NDEBUG)
#    define VMA_DEBUG_MARGINS 16
#    define VMA_DEBUG_DETECT_CORRUPTION 1
#    define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#endif

#include <vk_mem_alloc.h>

#pragma GCC diagnostic pop
