#pragma once

#include <functional>
#include <cstddef>

namespace cpps::details
{

//from https://github.com/HowardHinnant/hash_append/issues/7
template <typename T> requires (sizeof(std::size_t) == 4 || sizeof(std::size_t) == 8)
inline void hash_combine(std::size_t& seed, const T& value) noexcept
{
  if constexpr (sizeof(std::size_t) == 4)
    seed ^= std::hash<T>{}(value) + 0x9e3779b9U + (seed << 6) + (seed >> 2);
  
  if constexpr (sizeof(std::size_t) == 8)
    seed ^= std::hash<T>{}(value) + 0x9e3779b97f4a7c15LLU + (seed << 12) + (seed >> 4);
}

}
