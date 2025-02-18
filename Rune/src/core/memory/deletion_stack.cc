#include "deletion_stack.hh"

namespace Rune
{
    void DeletionStack::clear()
    {
        while (!deletors_.empty())
        {
            deletors_.top()();
            deletors_.pop();
        }
    }

    void DeletionStack::push(std::function<void()> deletor)
    {
        deletors_.push(std::move(deletor));
    }
} // namespace Rune
