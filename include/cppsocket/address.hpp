#pragma once

#include <hpp/define.hpp>

#if HPP_WIN_IMPL
  #include <winsock2.h>
#elif HPP_POSIX_IMPL
  #include <sys/socket.h>
#endif

#include <strict_enum/strict_enum.hpp>
#include <ehl/ehl.hpp>
#include "details/ct_ip.hpp"

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
};

using AddressIPv4 = Address<AddressFamily::IPv4>;
using AddressIPv6 = Address<AddressFamily::IPv6>;

} //namespace cpps
