/**
 * @file error_code.h
 * @brief ara::core::ErrorCode stub for ara::log implementation.
 *
 * AUTOSAR Adaptive Platform R25-11
 * MISRA C++:2023 compliant | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_ERROR_CODE_H_
#define ARA_CORE_ERROR_CODE_H_

#include "ara/core/string_view.h"
#include <cstdint>

namespace ara {
namespace core {

/**
 * @brief Minimal ErrorDomain base required for ErrorCode.
 */
class ErrorDomain
{
public:
    /// @brief Return the short name of the error domain.
    virtual StringView Name() const noexcept = 0;

    ErrorDomain()                            = default;
    virtual ~ErrorDomain()                   = default;
    ErrorDomain(const ErrorDomain &)         = delete;
    ErrorDomain &operator=(const ErrorDomain &) = delete;
    ErrorDomain(ErrorDomain &&)              = delete;
    ErrorDomain &operator=(ErrorDomain &&)   = delete;
};

/**
 * @brief Represents a platform-level error code.
 * [SWS_CORE_10300]
 */
class ErrorCode final
{
public:
    using CodeType = std::int32_t;

    /// @brief Construct an ErrorCode with a numeric code and its owning domain.
    constexpr ErrorCode(CodeType code, const ErrorDomain &domain) noexcept
        : code_{code}, domain_{&domain}
    {}

    /// @brief Return the numeric error value.
    constexpr CodeType Value() const noexcept { return code_; }

    /// @brief Return the owning error domain.
    constexpr const ErrorDomain &Domain() const noexcept { return *domain_; }

private:
    CodeType           code_;
    const ErrorDomain *domain_;
};

} // namespace core
} // namespace ara

#endif // ARA_CORE_ERROR_CODE_H_
