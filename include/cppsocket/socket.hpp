#pragma once

#include "details/helper_macros.hpp"

#if CPPS_WIN_IMPL
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>
#elif CPPS_POSIX_IMPL
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
#endif

#include <system_errc/system_errc.hpp>
#include <strict_enum/strict_enum.hpp>
#include "details/convert_byte_order.hpp"
#include "details/socket_resource.hpp"
#include "is_packet.hpp"
#include "address.hpp"

namespace cpps
{

STRICT_ENUM(SocketType)
(
  Stream = SOCK_STREAM,
  Datagram = SOCK_DGRAM,
);

STRICT_ENUM(SocketProtocol)
(
  TCP = IPPROTO_TCP,
  UDP = IPPROTO_UDP,
);

struct SocketInfo
{
  AddressFamily  address_family;
  SocketType     type;
  SocketProtocol protocol;
};

constexpr SocketInfo SI_IPv4_TCP = { AddressFamily::IPv4, SocketType::Stream, SocketProtocol::TCP };
constexpr SocketInfo SI_IPv6_TCP = { AddressFamily::IPv6, SocketType::Stream, SocketProtocol::TCP };
constexpr SocketInfo SI_IPv4_UDP = { AddressFamily::IPv4, SocketType::Datagram, SocketProtocol::UDP };
constexpr SocketInfo SI_IPv6_UDP = { AddressFamily::IPv6, SocketType::Datagram, SocketProtocol::UDP };

struct InvInfo
{
  bool binded;
  bool listening;
  bool connected;
};

constexpr InvInfo inv_bind         = { .binded = true,  .listening = false, .connected = false };
constexpr InvInfo inv_connect      = { .binded = false, .listening = false, .connected = true };
constexpr InvInfo inv_bind_connect = { .binded = true,  .listening = false, .connected = true };
constexpr InvInfo inv_bind_listen  = { .binded = true,  .listening = true,  .connected = false };

struct ConnectionSettings
{
  bool convert_byte_order;
};

constexpr ConnectionSettings default_connection_settings = { .convert_byte_order = true };

namespace details
{

using socklen_type = PP_IFE(CPPS_WIN_IMPL)(int)(socklen_t);

} //namespace details

template<SocketInfo SI, InvInfo INV, ConnectionSettings CS>
class Socket;

template<SocketInfo SI, InvInfo INV, ConnectionSettings CS>
struct IncomingConnection
{
  Socket<SI, INV, CS> sock;
  Address<SI.address_family> addr;
};

template<SocketInfo SI, InvInfo INV, ConnectionSettings SCS>
class Socket
{
  static_assert(!(SI.type == SocketType::Stream && SI.protocol == SocketProtocol::UDP));
  static_assert(!(SI.type == SocketType::Datagram && SI.protocol == SocketProtocol::TCP));

  friend struct Net;

  template<SocketInfo, InvInfo, ConnectionSettings>
  friend struct Socket;

  details::socket_resource m_handle_;

  Socket(details::socket_resource&& handle) noexcept :
    m_handle_(std::forward<details::socket_resource>(handle)) {}

  template<packet_type T>
  static constexpr T convert_byte_order(T t) noexcept
  {
    if constexpr(SCS.convert_byte_order)
      details::convert_byte_order(t);

    return t;
  }

public:
  static constexpr SocketInfo socket_info = SI;
  static constexpr InvInfo inv_info = INV;
  static constexpr ConnectionSettings connection_settings = SCS;

  template<ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<IncomingConnection<SI, inv_connect, CS>, sys_errc::ErrorCode, EHP> accept()
    noexcept(EHP != ehl::Policy::Exception) requires (SI.type == SocketType::Stream && INV.binded && INV.listening)
  {
    details::sockaddr_type<SI.address_family> addr;
    details::socklen_type addrlen = sizeof(addr);
    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of accept,
    //implementation know that pointer is from cast and deals with it
    details::socket_resource r = ::accept(m_handle_, reinterpret_cast<sockaddr*>(&addr), &addrlen);

    EHL_THROW_IF(r.is_invalid(), sys_errc::last_error());

    return IncomingConnection<SI, inv_connect, CS>(std::move(r), std::bit_cast<Address<SI.address_family>>(addr));
  }

  template<packet_type T, auto EHP = ehl::Policy::Exception> requires (INV.connected)
  [[nodiscard]] ehl::Result_t<T, sys_errc::ErrorCode, EHP> recv() noexcept(EHP != ehl::Policy::Exception)
  {
    T t;
    auto r = ::recv(m_handle_, reinterpret_cast<char*>(&t), sizeof(T), 0);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());

    return convert_byte_order(t);
  }

  template<packet_type T, auto EHP = ehl::Policy::Exception> requires (INV.connected)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> send(const T& t) noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = convert_byte_order(t);

    auto r = ::send(m_handle_, reinterpret_cast<const char*>(&t_copy), sizeof(T), 0);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());
  }

  template<typename T>
  struct recvfrom_result
  {
    T value;
    Address<SI.address_family> addr;
  };

  template<packet_type T, ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
    requires (SI.type == SocketType::Datagram && INV.binded)
  [[nodiscard]] ehl::Result_t<recvfrom_result<T>, sys_errc::ErrorCode, EHP> recvfrom()
    noexcept(EHP != ehl::Policy::Exception)
  {
    T t;
    details::sockaddr_type<SI.address_family> addr;
    details::socklen_type addrlen = sizeof(addr);
    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of recvfrom,
    //implementation know that pointer is from cast and deals with it
    auto r = ::recvfrom(
      m_handle_, reinterpret_cast<char*>(&t), sizeof(T), 0, reinterpret_cast<sockaddr*>(&addr), &addrlen);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());

    return {convert_byte_order(t), std::bit_cast<cpps::Address<SI.address_family>>(addr)};
  }

  template<packet_type T, ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> sendto(const T& t, const Address<SI.address_family>& addr)
    noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = convert_byte_order(t);

    details::socklen_type addrlen = sizeof(addr);
    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of recvfrom,
    //implementation know that pointer is from cast and deals with it
    auto r = ::sendto(
      m_handle_, reinterpret_cast<const char*>(&t_copy), sizeof(T), 0, reinterpret_cast<const sockaddr*>(&addr), addrlen);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());
  }
};

} //namespace cpps
