/**
 * @file format.h
 * @brief ara::log::Fmt and ara::log::Format – formatting hints for log arguments.
 *
 * AUTOSAR Adaptive Platform R25-11
 * Document ID 853
 *
 * Traceability:
 *   [SWS_LOG_00206]  enum class Fmt
 *   [SWS_LOG_00207]  struct Format
 *   [SWS_LOG_00225]  Format::fmt
 *   [SWS_LOG_00226]  Format::precision
 *
 * Coding standards:
 *   - MISRA C++:2023 Rule 6.4.1 – scoped enum with fixed underlying type
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
 */
enum class Fmt : std::uint16_t
{
    kDefault  = 0U, ///< Implementation-defined formatting.
    kDec      = 1U, ///< Decimal (signed/unsigned).
    kOct      = 2U, ///< Octal.
    kHex      = 3U, ///< Hexadecimal.
    kBin      = 4U, ///< Binary.
    kDecFloat = 5U, ///< Decimal float (printf "%f").
    kEngFloat = 6U, ///< Engineering float (printf "%e").
    kHexFloat = 7U, ///< Hex float (printf "%a").
    kAutoFloat= 8U  ///< Automatic shortest float (printf "%g").
};

// ---------------------------------------------------------------------------
// [SWS_LOG_00207] struct Format
// ---------------------------------------------------------------------------

/**
 * @brief Holds a formatting hint (specifier + precision) for a log argument.
 */
struct Format final
{
    /// [SWS_LOG_00225] The format specifier.
    Fmt           fmt       {Fmt::kDefault};
    /// [SWS_LOG_00226] The precision to use.
    std::uint16_t precision {0U};
};

// ---------------------------------------------------------------------------
// Format factory functions (all constexpr noexcept)
// ---------------------------------------------------------------------------

/// [SWS_LOG_00208]
constexpr Format Dflt() noexcept { return Format{Fmt::kDefault, 0U}; }

/// [SWS_LOG_00211]
constexpr Format Dec() noexcept { return Format{Fmt::kDec, 0U}; }

/// [SWS_LOG_00212]
constexpr Format Dec(std::uint16_t precision) noexcept
{ return Format{Fmt::kDec, precision}; }

/// [SWS_LOG_00213]
constexpr Format Oct() noexcept { return Format{Fmt::kOct, 0U}; }

/// [SWS_LOG_00214]
constexpr Format Oct(std::uint16_t precision) noexcept
{ return Format{Fmt::kOct, precision}; }

/// [SWS_LOG_00209]
constexpr Format Hex() noexcept { return Format{Fmt::kHex, 0U}; }

/// [SWS_LOG_00210]
constexpr Format Hex(std::uint16_t precision) noexcept
{ return Format{Fmt::kHex, precision}; }

/// [SWS_LOG_00215]
constexpr Format Bin() noexcept { return Format{Fmt::kBin, 0U}; }

/// [SWS_LOG_00216]
constexpr Format Bin(std::uint16_t precision) noexcept
{ return Format{Fmt::kBin, precision}; }

/// [SWS_LOG_00217]
constexpr Format DecFloat(std::uint16_t precision = 6U) noexcept
{ return Format{Fmt::kDecFloat, precision}; }

/// [SWS_LOG_00218]
constexpr Format DecFloatMax() noexcept
{
    return Format{Fmt::kDecFloat,
                  static_cast<std::uint16_t>(
                      std::numeric_limits<double>::max_digits10)};
}

/// [SWS_LOG_00219]
constexpr Format EngFloat(std::uint16_t precision = 6U) noexcept
{ return Format{Fmt::kEngFloat, precision}; }

/// [SWS_LOG_00220]
constexpr Format EngFloatMax() noexcept
{
    return Format{Fmt::kEngFloat,
                  static_cast<std::uint16_t>(
                      std::numeric_limits<double>::max_digits10)};
}

/// [SWS_LOG_00221]
constexpr Format HexFloat(std::uint16_t precision) noexcept
{ return Format{Fmt::kHexFloat, precision}; }

/// [SWS_LOG_00222]
constexpr Format HexFloatMax() noexcept
{
    return Format{Fmt::kHexFloat,
                  static_cast<std::uint16_t>(
                      std::numeric_limits<double>::max_digits10)};
}

/// [SWS_LOG_00223]
constexpr Format AutoFloat(std::uint16_t precision = 6U) noexcept
{ return Format{Fmt::kAutoFloat, precision}; }

/// [SWS_LOG_00224]
constexpr Format AutoFloatMax() noexcept
{
    return Format{Fmt::kAutoFloat,
                  static_cast<std::uint16_t>(
                      std::numeric_limits<double>::max_digits10)};
}

} // namespace log
} // namespace ara

#endif // ARA_LOG_FORMAT_H_
