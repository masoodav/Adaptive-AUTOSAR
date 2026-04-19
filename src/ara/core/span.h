#ifndef ARA_CORE_SPAN_H_
#define ARA_CORE_SPAN_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace ara
{
namespace core
{

enum class Byte : std::uint8_t
{
};

template <typename T>
class Span final
{
public:
    typedef T value_type;

    Span() noexcept : data_(nullptr), size_(0U) {}

    Span(T* data, std::size_t size) noexcept : data_(data), size_(size) {}

    template <typename Allocator>
    explicit Span(std::vector<typename std::remove_const<T>::type, Allocator>& value) noexcept
        : data_(value.data()),
          size_(value.size())
    {
    }

    template <typename Allocator>
    explicit Span(const std::vector<typename std::remove_const<T>::type, Allocator>& value) noexcept
        : data_(value.data()),
          size_(value.size())
    {
    }

    const T* data() const noexcept
    {
        return data_;
    }

    T* data() noexcept
    {
        return data_;
    }

    std::size_t size() const noexcept
    {
        return size_;
    }

    const T& operator[](std::size_t index) const noexcept
    {
        return data_[index];
    }

private:
    T* data_;
    std::size_t size_;
};

}  // namespace core
}  // namespace ara

#endif  // ARA_CORE_SPAN_H_
