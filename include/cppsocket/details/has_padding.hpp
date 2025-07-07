#pragma once

#include <cstddef>
#include <type_traits>
#include <aggr_refl/aggregate_reflection.hpp>
#include "cppsocket/fixed_t.hpp"
#include "apply_index.hpp"

namespace cpps::details
{

template<typename T>
struct no_padding_size :
  std::integral_constant<
    std::size_t, 
    details::apply_index<aggr_refl::tuple_size_v<T>>([](auto... Is)
    {
      return
        (0 + ... + no_padding_size<aggr_refl::tuple_element_t<Is, T>>::value);
    })>{};

template<typename T>
struct no_padding_size<fixed_t<T>> :
  std::integral_constant<std::size_t, sizeof(T)>{};

template<typename T, std::size_t N>
struct no_padding_size<T[N]> :
  std::integral_constant<std::size_t, N * no_padding_size<T>::value>{};

template<typename T>
constexpr std::size_t no_padding_size_v = no_padding_size<T>::value;

template<typename T>
constexpr bool has_padding_v = sizeof(T) != no_padding_size_v<T>;

} //namespace cpps::details