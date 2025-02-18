#pragma once

#include <functional>
#include <stack>

namespace Rune
{
    class DeletionStack
    {
    public:
        void clear();
        void push(std::function<void()> deletor);

    private:
        std::stack<std::function<void()>> deletors_;
    };
} // namespace Rune
