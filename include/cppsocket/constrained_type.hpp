#pragma once

#include <stdexcept>
#include <utility>
#include <concepts>
#include <hpp/always_inline.hpp>

namespace cpps
{

template<typename T, auto P> requires std::predicate<decltype(P), const T&>
class constrained_type
{
  T m_t_;

public:
  template<typename U>
  HPP_ALWAYS_INLINE explicit constexpr constrained_type(U&& t) : 
    m_t_{P(t) ? std::forward<U>(t) : throw std::invalid_argument("Argument does not meet requirements")} {}

  HPP_ALWAYS_INLINE constexpr operator T&&() && noexcept
  {
    if(!P(m_t_)) std::unreachable();

    return std::move(m_t_);
  }

  HPP_ALWAYS_INLINE constexpr operator const T&() const noexcept
  {
    if(!P(m_t_)) std::unreachable();

    return m_t_;
  }

  HPP_ALWAYS_INLINE constexpr const T& value() const noexcept
  {
    if(!P(m_t_)) std::unreachable();

    return m_t_;
  }
};

} //namespace cpps
