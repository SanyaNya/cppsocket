#pragma once

#include <cstddef>
#include <aggr_refl/aggregate_reflection.hpp>
#include "cppsocket/fixed_int.hpp"

namespace cpps
{

namespace details
{

template<typename T, typename IS>
struct is_packet_h : std::false_type{};

template<typename T, std::size_t ... Is>
struct is_packet_h<T, std::index_sequence<Is...>> :
  std::bool_constant<(
    is_packet_h<aggr_refl::tuple_element_t<Is, T>, std::make_index_sequence<aggr_refl::tuple_size_v<T>>>::value && ...)> {};

template<typename T, typename IS>
struct is_packet_h<fixed_int<T>, IS> : std::true_type {};

template<typename T, std::size_t N, typename IS>
struct is_packet_h<T[N], IS> :
  std::bool_constant<
    is_packet_h<T, std::make_index_sequence<aggr_refl::tuple_size_v<T>>>::value> {};

} //namespace details

template<typename T>
constexpr bool is_packet_v = details::is_packet_h<T, std::make_index_sequence<aggr_refl::tuple_size_v<T>>>::value;

template<typename T>
concept packet_type = true;//is_packet_v<T>;

} //namespace cpps
