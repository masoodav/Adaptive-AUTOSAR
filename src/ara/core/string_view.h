#ifndef ARA_CORE_STRING_VIEW_H_
#define ARA_CORE_STRING_VIEW_H_

#include <cstddef>
#include <cstring>
#include <string>

namespace ara
{
namespace core
{

class StringView final
{
public:
    StringView() noexcept : data_(nullptr), size_(0U) {}

    StringView(const char* value) noexcept
        : data_(value),
          size_((value == nullptr) ? 0U : static_cast<std::size_t>(std::strlen(value)))
    {
    }

    StringView(const std::string& value) noexcept : data_(value.data()), size_(value.size()) {}

    StringView(const char* value, std::size_t size) noexcept : data_(value), size_(size) {}

    const char* data() const noexcept
    {
        return data_;
    }

    std::size_t size() const noexcept
    {
        return size_;
    }

    bool empty() const noexcept
    {
        return size_ == 0U;
    }

    std::string ToString() const
    {
        if (data_ == nullptr)
        {
            return std::string();
        }
        return std::string(data_, size_);
    }

private:
    const char* data_;
    std::size_t size_;
};

}  // namespace core
}  // namespace ara

#endif  // ARA_CORE_STRING_VIEW_H_
