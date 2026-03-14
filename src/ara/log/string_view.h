/**
 * @file string_view.h
 * @brief ara::core::StringView stub for ara::log implementation.
 *
 * AUTOSAR Adaptive Platform R25-11
 * MISRA C++:2023 compliant | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_STRING_VIEW_H_
#define ARA_CORE_STRING_VIEW_H_

#include <string_view>

namespace ara {
namespace core {

/// @brief Alias to std::string_view as required by AUTOSAR AP.
using StringView = std::string_view;

} // namespace core
} // namespace ara

#endif // ARA_CORE_STRING_VIEW_H_
