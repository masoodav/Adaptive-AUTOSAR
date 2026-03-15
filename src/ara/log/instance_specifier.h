/**
 * @file instance_specifier.h
 * @brief ara::core::InstanceSpecifier – C++14 compliant stub.
 *
 * AUTOSAR Adaptive Platform R25-11
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_INSTANCE_SPECIFIER_H_
#define ARA_CORE_INSTANCE_SPECIFIER_H_

#include "./string_view.h"
#include <string>

namespace ara {
namespace core {

/**
 * @brief Identifies a port-prototype instance at runtime.
 * [SWS_CORE_10200]
 */
class InstanceSpecifier final
{
public:
    /// Construct from a short-name model path.
    explicit InstanceSpecifier(StringView metaModelIdentifier) noexcept
        : path_(metaModelIdentifier.data(), metaModelIdentifier.size())
    {}

    /// Return the model path as StringView.
    StringView ToString() const noexcept { return StringView(path_); }

private:
    std::string path_;
};

} // namespace core
} // namespace ara

#endif // ARA_CORE_INSTANCE_SPECIFIER_H_
