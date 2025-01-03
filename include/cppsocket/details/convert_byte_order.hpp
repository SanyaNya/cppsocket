#pragma once

#include <bit>
#include <cstddef>
#include <aggr_refl/aggregate_reflection.hpp>
#include "cppsocket/fixed_int.hpp"
#include "apply_index.hpp"

namespace cpps::details
{

template<typename T>
constexpr void convert_byte_order(fixed_int<T>& t) noexcept
{
  if constexpr(std::endian::native == std::endian::little)
    t = fixed_int<T>{std::byteswap(t.underlying_value())};
}

template<typename T, std::size_t N>
constexpr void convert_byte_order(T (&t)[N]) noexcept
{
  for(auto& v : t)
    convert_byte_order(v);
}

template<typename T>
  requires std::is_class_v<T>
constexpr void convert_byte_order(T& t) noexcept
{
  details::apply_index<aggr_refl::tuple_size_v<T>>([&](auto... Is) noexcept
  {
    (convert_byte_order(aggr_refl::get<Is>(t)), ...);
  });
}

} //namespace cpps::details
