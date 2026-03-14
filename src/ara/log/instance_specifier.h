/**
 * @file instance_specifier.h
 * @brief ara::core::InstanceSpecifier stub for ara::log implementation.
 *
 * AUTOSAR Adaptive Platform R25-11
 * MISRA C++:2023 compliant | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_CORE_INSTANCE_SPECIFIER_H_
#define ARA_CORE_INSTANCE_SPECIFIER_H_

#include "ara/core/string_view.h"
#include <string>

namespace ara {
namespace core {

/**
 * @brief Represents an InstanceSpecifier in the AUTOSAR meta-model.
 *
 * Used to identify port-prototype instances at runtime.
 * [SWS_CORE_10200] – InstanceSpecifier
 */
class InstanceSpecifier final
{
public:
    /// @brief Construct from a model path string view.
    /// @param metaModelIdentifier  Short-name path of the port prototype.
    explicit InstanceSpecifier(StringView metaModelIdentifier) noexcept
        : path_{metaModelIdentifier}
    {}

    /// @brief Return the model path as StringView.
    StringView ToString() const noexcept { return path_; }

private:
    std::string path_;
};

} // namespace core
} // namespace ara

#endif // ARA_CORE_INSTANCE_SPECIFIER_H_
