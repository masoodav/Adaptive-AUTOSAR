#ifndef ARA_CORE_CORE_TYPES_H
#define ARA_CORE_CORE_TYPES_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// FIX: Include the existing ErrorCode definition from the project
#include "error_code.h"
#include "instance_specifier.h"

namespace ara {
namespace core {

using Byte = std::uint8_t;

class StringView {
public:
    constexpr StringView(const char* s) : data_(s), size_(std::strlen(s)) {}
    constexpr const char* data() const { return data_; }
    constexpr size_t size() const { return size_; }
private:
    const char* data_;
    size_t size_;
};

template <typename T>
class Span {
public:
    Span(const T* ptr, size_t count) : ptr_(ptr), count_(count) {}
    const T* data() const { return ptr_; }
    size_t size() const { return count_; }
    const T& operator[](size_t idx) const { return ptr_[idx]; }
    auto begin() const { return ptr_; }
    auto end() const { return ptr_ + count_; }
private:
    const T* ptr_;
    size_t count_;
};

// class InstanceSpecifier {
// public:
//     explicit InstanceSpecifier(StringView s) : str_(s.data()) {}
//     std::string ToString() const { return str_; }
// private:
//     std::string str_;
// };

} // namespace core
} // namespace ara

#endif // ARA_CORE_CORE_TYPES_H