#pragma once

#include <type_traits>
#include <variant>
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
struct is_packet_variant : std::false_type {};

template<packet_type... Ts>
struct is_packet_variant<std::variant<Ts...>> : std::true_type {};

template<typename T>
constexpr bool is_packet_variant_v = is_packet_variant<T>::value;

template<typename T>
concept packet_variant_type = is_packet_variant_v<T>;

template<packet_type T>
constexpr auto packet_validate_predicate = [](const T& t) noexcept { return t.is_valid(); };

template<packet_variant_type V>
constexpr auto packet_variant_validate_predicate =
  [](const V& v){ return std::visit([](const auto& p) { return p.is_valid(); }, v); };

template<packet_type T>
using valid_packet = constrained_type<T, packet_validate_predicate<T>>;

template<packet_variant_type V>
using valid_packet_variant = constrained_type<V, packet_variant_validate_predicate<V>>;

template<auto EHP = ehl::Policy::Exception, typename T>
constexpr ehl::Result_t<valid_packet<std::decay_t<T>>, sys_errc::ErrorCode, EHP> make_valid_packet(T&& t)
  noexcept(EHP != ehl::Policy::Exception)
{
  EHL_THROW_IF(!t.is_valid(), sys_errc::common::sockets::invalid_argument);

  return valid_packet<std::decay_t<T>>{std::forward<T>(t)};
}

} //namespace cpps
