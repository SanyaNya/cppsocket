#pragma once

#include <hpp/define.hpp>

#if HPP_WIN_IMPL
  #include <winsock2.h>
#elif HPP_POSIX_IMPL
  #include <sys/socket.h>
#endif

#include <bit>
#include <strict_enum/strict_enum.hpp>
#include <ehl/ehl.hpp>
#include "details/ct_ip.hpp"
#include "details/hash_combine.hpp"

namespace cpps
{

STRICT_ENUM(AddressFamily)
(
  IPv4 = AF_INET,
  IPv6 = AF_INET6,
);

STRICT_ENUM(AddressError)
(
  Invalid,
);

namespace details
{

template<AddressFamily AF>
using sockaddr_type = std::conditional_t<AF == AddressFamily::IPv4, sockaddr_in, sockaddr_in6>;

} //namespace details

using port_t = HPP_IFE(HPP_WIN_IMPL)(USHORT)(in_port_t);

template<AddressFamily AF>
class Address
{
  details::sockaddr_type<AF> m_saddr_;

  constexpr Address(details::sockaddr_type<AF> saddr) noexcept : m_saddr_(saddr) {}

  struct uint64_pair
  {
    std::uint64_t first, second;

    constexpr bool operator==(const uint64_pair&) const noexcept = default;
  };

public:
  template<std::size_t N>
  consteval Address(const char (&addr)[N], port_t port) noexcept : m_saddr_{}
  {
    if constexpr(std::endian::native == std::endian::little)
      port = std::byteswap(port);

    if constexpr(AF == AddressFamily::IPv4)
    {
      m_saddr_.sin_family = AF_INET;
      m_saddr_.sin_port = port;
      m_saddr_.sin_addr = details::ct_inet_pton<AF_INET>(addr);
    }

    if constexpr(AF == AddressFamily::IPv6)
    {
      m_saddr_.sin6_family = AF_INET6;
      m_saddr_.sin6_port = port;
      m_saddr_.sin6_addr = details::ct_inet_pton<AF_INET6>(addr);
    }
  }

  template<auto EHP = ehl::Policy::Exception>
  [[nodiscard]] static ehl::Result_t<Address, AddressError, EHP> make(const char* addr, port_t port)
    noexcept(EHP != ehl::Policy::Exception)
  {
    details::sockaddr_type<AF> saddr{};
    int r = 0;

    if constexpr(std::endian::native == std::endian::little)
      port = std::byteswap(port);

    if constexpr(AF == AddressFamily::IPv4)
    {
      saddr.sin_family = AF_INET;
      saddr.sin_port = port;
      r = inet_pton(AF_INET, addr, &saddr.sin_addr);
    }

    if constexpr(AF == AddressFamily::IPv6)
    {
      saddr.sin6_family = AF_INET6;
      saddr.sin6_port = port;
      r = inet_pton(AF_INET6, addr, &saddr.sin6_addr);
    }

    EHL_THROW_IF(r == 0, AddressError(AddressError::Invalid));

    return Address(saddr);
  }

  constexpr bool operator==(const Address& addr) const noexcept
  {
    if constexpr(AF == AddressFamily::IPv4)
      return m_saddr_.sin_port == addr.m_saddr_.sin_port &&
             std::bit_cast<std::uint32_t>(m_saddr_.sin_addr) == std::bit_cast<std::uint32_t>(addr.m_saddr_.sin_addr);

    if constexpr(AF == AddressFamily::IPv6)
      return m_saddr_.sin6_port == addr.m_saddr_.sin6_port &&
             std::bit_cast<uint64_pair>(m_saddr_.sin6_addr) == std::bit_cast<uint64_pair>(addr.m_saddr_.sin6_addr);
  }

  constexpr bool operator!=(const Address& addr) const noexcept
  {
    return !this->operator==(addr);
  }

  constexpr std::size_t hash() const noexcept
  {
    if constexpr(AF == AddressFamily::IPv4)
    {
      const auto part1 = static_cast<std::uint64_t>(m_saddr_.sin_port) << 32;
      const auto part2 = static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(m_saddr_.sin_addr));
      return std::hash<std::uint64_t>{}(part1 | part2);
    }

    if constexpr(AF == AddressFamily::IPv6)
    {
      const auto[part1, part2] = std::bit_cast<uint64_pair>(m_saddr_.sin6_addr);
      std::size_t seed = m_saddr_.sin6_port;

      details::hash_combine(seed, part1);
      details::hash_combine(seed, part2);

      return seed;
    }
  }
};

using AddressIPv4 = Address<AddressFamily::IPv4>;
using AddressIPv6 = Address<AddressFamily::IPv6>;

} //namespace cpps

template<cpps::AddressFamily AF>
struct std::hash<cpps::Address<AF>>
{
  std::size_t operator()(const cpps::Address<AF>& s) const noexcept
  {
    return s.hash();
  }
};
