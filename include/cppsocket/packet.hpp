#pragma once

#include <type_traits>
#include <ehl/ehl.hpp>
#include <system_errc/system_errc.hpp>
#include "details/convert_byte_order.hpp"
#include "constrained_type.hpp"

namespace cpps
{

template<typename T>
concept packet_type =
  std::is_trivially_copyable_v<T>             &&
  std::has_unique_object_representations_v<T> &&
  requires(T& t)
  {
    details::convert_byte_order(t);
    { std::as_const(t).is_valid() } noexcept -> std::same_as<bool>;
  };

template<typename T>
constexpr bool is_packet = packet_type<T>;

template<typename T>
constexpr auto packet_validate_predicate = [](const T& t) noexcept { return t.is_valid(); };

template<packet_type T>
using valid_packet = constrained_type<T, packet_validate_predicate<T>>;

template<auto EHP = ehl::Policy::Exception, typename T>
constexpr ehl::Result_t<valid_packet<std::decay_t<T>>, sys_errc::ErrorCode, EHP> make_valid_packet(T&& t)
  noexcept(EHP != ehl::Policy::Exception)
{
  EHL_THROW_IF(!t.is_valid(), sys_errc::common::sockets::invalid_argument);

  return valid_packet<std::decay_t<T>>{std::forward<T>(t)};
}

} //namespace cpps
