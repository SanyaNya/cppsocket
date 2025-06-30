#pragma once

#include <cstdint>
#include <type_traits>
#include <limits>
#include <concepts>

namespace cpps
{

template<typename T>
  requires (std::integral<T> || (std::floating_point<T> && std::numeric_limits<T>::is_iec559))
struct fixed_t
{
  constexpr operator T() const noexcept { return m_val_; }
  
  constexpr T underlying_value() const noexcept { return m_val_; }

  T m_val_;
};

template<typename T>
struct is_fixed_t : std::false_type {};

template<typename T>
struct is_fixed_t<fixed_t<T>> : std::true_type {};

template<typename T>
constexpr bool is_fixed_t_v = is_fixed_t<T>::value;

template<typename T>
concept fixed_t_type = is_fixed_t_v<T>;

using int8_t  = fixed_t<std::int8_t>;
using int16_t = fixed_t<std::int16_t>;
using int32_t = fixed_t<std::int32_t>;
using int64_t = fixed_t<std::int64_t>;

using uint8_t  = fixed_t<std::uint8_t>;
using uint16_t = fixed_t<std::uint16_t>;
using uint32_t = fixed_t<std::uint32_t>;
using uint64_t = fixed_t<std::uint64_t>;

using ieee754_float32_t = fixed_t<float>;
using ieee754_float64_t = fixed_t<double>;

} //namespace cpps
