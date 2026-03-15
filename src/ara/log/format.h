/**
 * @file format.h
 * @brief ara::log::Fmt, ara::log::Format and constexpr factory functions.
 *
 * AUTOSAR Adaptive Platform R25-11  Document ID 853
 * C++14 compliant.
 *
 * Traceability:
 *   [SWS_LOG_00206]  enum class Fmt
 *   [SWS_LOG_00207]  struct Format
 *   [SWS_LOG_00225]  Format::fmt
 *   [SWS_LOG_00226]  Format::precision
 *   [SWS_LOG_00208]  Dflt()
 *   [SWS_LOG_00211]  Dec()       [SWS_LOG_00212] Dec(precision)
 *   [SWS_LOG_00213]  Oct()       [SWS_LOG_00214] Oct(precision)
 *   [SWS_LOG_00209]  Hex()       [SWS_LOG_00210] Hex(precision)
 *   [SWS_LOG_00215]  Bin()       [SWS_LOG_00216] Bin(precision)
 *   [SWS_LOG_00217]  DecFloat()  [SWS_LOG_00218] DecFloatMax()
 *   [SWS_LOG_00219]  EngFloat()  [SWS_LOG_00220] EngFloatMax()
 *   [SWS_LOG_00221]  HexFloat()  [SWS_LOG_00222] HexFloatMax()
 *   [SWS_LOG_00223]  AutoFloat() [SWS_LOG_00224] AutoFloatMax()
 *
 * MISRA C++:2023 | ISO/SAE 21434 | CERT C++ | CWE-safe
 */

#ifndef ARA_LOG_FORMAT_H_
#define ARA_LOG_FORMAT_H_

#include <cstdint>
#include <limits>

namespace ara {
namespace log {

// ---------------------------------------------------------------------------
// [SWS_LOG_00206] enum class Fmt
// ---------------------------------------------------------------------------

/**
 * @brief Format specifiers for log message arguments.
 * Underlying type fixed to std::uint16_t per spec.
 * MISRA C++:2023 Rule 6.4.1 – scoped enum with fixed underlying type.
 */
enum class Fmt : std::uint16_t
{
    kDefault  = 0U,
    kDec      = 1U,
    kOct      = 2U,
    kHex      = 3U,
    kBin      = 4U,
    kDecFloat = 5U,
    kEngFloat = 6U,
    kHexFloat = 7U,
    kAutoFloat= 8U
};

// ---------------------------------------------------------------------------
// [SWS_LOG_00207] struct Format
// ---------------------------------------------------------------------------

/**
 * @brief Holds a format specifier and its precision.
 */
struct Format final
{
    Fmt           fmt;        ///< [SWS_LOG_00225]
    std::uint16_t precision;  ///< [SWS_LOG_00226]
};

// ---------------------------------------------------------------------------
// Compile-time constant for round-trip float precision (C++11 / C++14 OK).
// std::numeric_limits<double>::max_digits10 is constexpr since C++11.
// ---------------------------------------------------------------------------

namespace detail {
    static const std::uint16_t kDoubleMaxDigits10 =
        static_cast<std::uint16_t>(
            std::numeric_limits<double>::max_digits10);
} // namespace detail

// ---------------------------------------------------------------------------
// Factory functions – all constexpr noexcept per spec
// ---------------------------------------------------------------------------

/// [SWS_LOG_00208]
inline constexpr Format Dflt() noexcept
{
    Format f = { Fmt::kDefault, 0U };
    return f;
}

/// [SWS_LOG_00211]
inline constexpr Format Dec() noexcept
{
    Format f = { Fmt::kDec, 0U };
    return f;
}

/// [SWS_LOG_00212]
inline constexpr Format Dec(std::uint16_t precision) noexcept
{
    Format f = { Fmt::kDec, precision };
    return f;
}

/// [SWS_LOG_00213]
inline constexpr Format Oct() noexcept
{
    Format f = { Fmt::kOct, 0U };
    return f;
}

/// [SWS_LOG_00214]
inline constexpr Format Oct(std::uint16_t precision) noexcept
{
    Format f = { Fmt::kOct, precision };
    return f;
}

/// [SWS_LOG_00209]
inline constexpr Format Hex() noexcept
{
    Format f = { Fmt::kHex, 0U };
    return f;
}

/// [SWS_LOG_00210]
inline constexpr Format Hex(std::uint16_t precision) noexcept
{
    Format f = { Fmt::kHex, precision };
    return f;
}

/// [SWS_LOG_00215]
inline constexpr Format Bin() noexcept
{
    Format f = { Fmt::kBin, 0U };
    return f;
}

/// [SWS_LOG_00216]
inline constexpr Format Bin(std::uint16_t precision) noexcept
{
    Format f = { Fmt::kBin, precision };
    return f;
}

/// [SWS_LOG_00217]
inline constexpr Format DecFloat(std::uint16_t precision = 6U) noexcept
{
    Format f = { Fmt::kDecFloat, precision };
    return f;
}

/// [SWS_LOG_00218]
inline Format DecFloatMax() noexcept
{
    Format f = { Fmt::kDecFloat, detail::kDoubleMaxDigits10 };
    return f;
}

/// [SWS_LOG_00219]
inline constexpr Format EngFloat(std::uint16_t precision = 6U) noexcept
{
    Format f = { Fmt::kEngFloat, precision };
    return f;
}

/// [SWS_LOG_00220]
inline Format EngFloatMax() noexcept
{
    Format f = { Fmt::kEngFloat, detail::kDoubleMaxDigits10 };
    return f;
}

/// [SWS_LOG_00221]
inline constexpr Format HexFloat(std::uint16_t precision) noexcept
{
    Format f = { Fmt::kHexFloat, precision };
    return f;
}

/// [SWS_LOG_00222]
inline Format HexFloatMax() noexcept
{
    Format f = { Fmt::kHexFloat, detail::kDoubleMaxDigits10 };
    return f;
}

/// [SWS_LOG_00223]
inline constexpr Format AutoFloat(std::uint16_t precision = 6U) noexcept
{
    Format f = { Fmt::kAutoFloat, precision };
    return f;
}

/// [SWS_LOG_00224]
inline Format AutoFloatMax() noexcept
{
    Format f = { Fmt::kAutoFloat, detail::kDoubleMaxDigits10 };
    return f;
}

} // namespace log
} // namespace ara

#endif // ARA_LOG_FORMAT_H_
