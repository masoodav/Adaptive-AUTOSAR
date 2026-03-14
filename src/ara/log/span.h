/**
 * @file span.h
 * @brief ara::core::Span and ara::core::Byte stubs for ara::log implementation.
 *
 * AUTOSAR Adaptive Platform R25-11
 * MISRA C++:2023 compliant | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_SPAN_H_
#define ARA_CORE_SPAN_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace ara {
namespace core {

/// @brief Byte type alias as required by AUTOSAR AP.
using Byte = std::byte;

/// @brief Span type alias as required by AUTOSAR AP.
template <typename T>
using Span = std::span<T>;

} // namespace core
} // namespace ara

#endif // ARA_CORE_SPAN_H_
