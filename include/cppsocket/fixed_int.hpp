#pragma once

#include <cstdint>
#include <type_traits>
#include <concepts>

namespace cpps
{

template<std::integral T>
struct fixed_int
{
  constexpr operator T() noexcept { return m_val_; }
  
  constexpr T underlying_value() noexcept { return m_val_; }

  T m_val_;
};

template<typename T>
struct is_fixed_int : std::false_type {};

template<typename T>
struct is_fixed_int<fixed_int<T>> : std::true_type {};

template<typename T>
constexpr bool is_fixed_int_v = is_fixed_int<T>::value;

template<typename T>
concept fixed_int_type = is_fixed_int_v<T>;

using int8_t  = fixed_int<std::int8_t>;
using int16_t = fixed_int<std::int16_t>;
using int32_t = fixed_int<std::int32_t>;
using int64_t = fixed_int<std::int64_t>;

using uint8_t  = fixed_int<std::uint8_t>;
using uint16_t = fixed_int<std::uint16_t>;
using uint32_t = fixed_int<std::uint32_t>;
using uint64_t = fixed_int<std::uint64_t>;

} //namespace cpps
