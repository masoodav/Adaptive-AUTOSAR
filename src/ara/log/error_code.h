/**
 * @file error_code.h
 * @brief ara::core::ErrorDomain and ara::core::ErrorCode – C++14 compliant.
 *
 * AUTOSAR Adaptive Platform R25-11
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_ERROR_CODE_H_
#define ARA_CORE_ERROR_CODE_H_

#include "./string_view.h"
#include <cstdint>

namespace ara {
namespace core {

/**
 * @brief Abstract base for error domains.
 */
class ErrorDomain
{
public:
    virtual StringView Name() const noexcept = 0;

    ErrorDomain()                               = default;
    virtual ~ErrorDomain()                      = default;
    ErrorDomain(const ErrorDomain &)            = delete;
    ErrorDomain &operator=(const ErrorDomain &) = delete;
    ErrorDomain(ErrorDomain &&)                 = delete;
    ErrorDomain &operator=(ErrorDomain &&)      = delete;
};

/**
 * @brief Platform error code with owning domain reference.
 * [SWS_CORE_10300]
 */
class ErrorCode final
{
public:
    typedef std::int32_t CodeType;

    ErrorCode(CodeType code, const ErrorDomain &domain) noexcept
        : code_(code), domain_(&domain)
    {}

    CodeType           Value()  const noexcept { return code_;    }
    const ErrorDomain &Domain() const noexcept { return *domain_; }

private:
    CodeType           code_;
    const ErrorDomain *domain_;
};

} // namespace core
} // namespace ara

#endif // ARA_CORE_ERROR_CODE_H_
