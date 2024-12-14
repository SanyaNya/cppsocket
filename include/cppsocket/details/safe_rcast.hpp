#pragma once

#include <cstddef>
#include <numeric>
#include <cstring>
#include <type_traits>
#include <new>

namespace cpps::details
{

template<typename To, typename From>
class safe_rcast_storage
{
  static_assert(std::is_trivially_copyable_v<To>);
  static_assert(std::is_trivially_copyable_v<From>);
  static_assert(std::is_trivially_constructible_v<To>);
  static_assert(sizeof(To) <= sizeof(From));

  static constexpr auto common_align = std::lcm(alignof(To), alignof(From));

  alignas(common_align) std::byte buffer_[sizeof(From)];

public:
  explicit safe_rcast_storage(const From& f) noexcept
  {
    std::memcpy(buffer_, &f, sizeof(From));
    new(buffer_) To;
  }

  To& get() noexcept
  {
    return *std::launder(reinterpret_cast<To*>(buffer_));
  }

  const To& get() const noexcept
  {
    return *std::launder(reinterpret_cast<const To*>(buffer_));
  }

  operator To&() noexcept
  {
    return *std::launder(reinterpret_cast<To*>(buffer_));
  }

  operator const To&() const noexcept
  {
    return *std::launder(reinterpret_cast<const To*>(buffer_));
  }

  To* operator&() noexcept
  {
    return std::launder(reinterpret_cast<To*>(buffer_));
  }

  const To* operator&() const noexcept
  {
    return std::launder(reinterpret_cast<const To*>(buffer_));
  }
};

template<typename To, typename From>
inline safe_rcast_storage<To, From> safe_rcast(const From& f) noexcept
{
  return safe_rcast_storage<To, From>{f};
}

} //namespace cpps::details
