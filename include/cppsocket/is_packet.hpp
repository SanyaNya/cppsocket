#pragma once

#include <type_traits>
#include "details/convert_byte_order.hpp"

namespace cpps
{

template<typename T>
concept packet_type =
  requires(T& t) { details::convert_byte_order(t); } &&
  std::is_trivially_copyable_v<T>                    &&
  std::has_unique_object_representations_v<T>;

template<typename T>
constexpr bool is_packet = packet_type<T>;

} //namespace cpps
