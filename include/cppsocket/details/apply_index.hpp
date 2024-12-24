#pragma once

#include <cstddef>
#include <utility>

namespace cpps::details
{

namespace detail
{

template<std::size_t N>
struct Index
{
  constexpr operator std::size_t() const noexcept { return N; }
};

template<typename F, std::size_t... Ns>
constexpr auto apply_index_impl(F f, std::index_sequence<Ns...>)
{
  return f(Index<Ns>{}...);
}

} //namespace details

template<size_t Size, typename F>
constexpr auto apply_index(F f)
{
  return detail::apply_index_impl(f, std::make_index_sequence<Size>{});
}

} //namespace cpps::details
