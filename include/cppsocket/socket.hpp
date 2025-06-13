#pragma once

#include <hpp/define.hpp>

#if HPP_WIN_IMPL
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>
#elif HPP_POSIX_IMPL
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <poll.h>
#endif

#include <utility>
#include <ehl/ehl.hpp>
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

enum class PollFlags
{
  In  = POLLIN,
  Out = POLLOUT
};

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

constexpr InvInfo inv_none         = { .binded = false, .listening = false, .connected = false };
constexpr InvInfo inv_bind         = { .binded = true,  .listening = false, .connected = false };
constexpr InvInfo inv_connect      = { .binded = false, .listening = false, .connected = true };
constexpr InvInfo inv_bind_connect = { .binded = true,  .listening = false, .connected = true };
constexpr InvInfo inv_bind_listen  = { .binded = true,  .listening = true,  .connected = false };

struct ConnectionSettings
{
  bool convert_byte_order;
};

constexpr ConnectionSettings default_connection_settings = { .convert_byte_order = true };

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

  template<ConnectionSettings CS, packet_type T>
  static constexpr T convert_byte_order(T t) noexcept
  {
    if constexpr(CS.convert_byte_order)
      details::convert_byte_order(t);

    return t;
  }

  template<typename T>
  struct extra_byte
  {
    T obj;
    std::byte byte;

    constexpr operator T&() noexcept { return obj; }
  };

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

    details::socket_resource r = ::accept(m_handle_, details::to_sockaddr_ptr(&addr), &addrlen);

    EHL_THROW_IF(r.is_invalid(), sys_errc::last_error());

    return IncomingConnection<SI, inv_connect, CS>(std::move(r), details::from_sockaddr(addr));
  }

  template<packet_type T, auto EHP = ehl::Policy::Exception> requires (INV.connected)
  [[nodiscard]] ehl::Result_t<T, sys_errc::ErrorCode, EHP> recv() noexcept(EHP != ehl::Policy::Exception)
  {
    std::conditional_t<SI.type == SocketType::Datagram, extra_byte<T>, T> t;

    //ensure all data received for stream
    constexpr int flags = SI.type == SocketType::Stream ? MSG_WAITALL : 0;
    int r = ::recv(m_handle_, reinterpret_cast<char*>(&t), sizeof(t), flags);

    //return system error or not_connected to indicate connection issue or wrong_protocol_type to indicate wrong packet size
    EHL_THROW_IF(
      r != sizeof(T),
      r < 0 ? sys_errc::last_error() :
              (r == 0 ? sys_errc::common::sockets::not_connected : sys_errc::common::sockets::wrong_protocol_type));

    return convert_byte_order<SCS, T>(t);
  }

  template<packet_type T, auto EHP = ehl::Policy::Exception> requires (INV.connected)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> send(const T& t) noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = convert_byte_order<SCS>(t);

    auto r = ::send(m_handle_, reinterpret_cast<const char*>(&t_copy), sizeof(T), 0);

    //return system error or wrong_protocol_type to indicate interruption of send
    EHL_THROW_IF(r != sizeof(T), r < 0 ? sys_errc::last_error() : sys_errc::common::sockets::wrong_protocol_type);
  }

  template<typename T>
  struct recvfrom_result
  {
    T value;
    Address<SI.address_family> addr;
  };

  template<packet_type T, ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<recvfrom_result<T>, sys_errc::ErrorCode, EHP> recvfrom()
    noexcept(EHP != ehl::Policy::Exception)
  {
    extra_byte<T> t;
    details::sockaddr_type<SI.address_family> addr;
    details::socklen_type addrlen = sizeof(addr);

    auto r = ::recvfrom(
      m_handle_, reinterpret_cast<char*>(&t), sizeof(t), 0, details::to_sockaddr_ptr(&addr), &addrlen);

    //return system error or not_connected to indicate connection issue or wrong_protocol_type to indicate wrong packet size
    EHL_THROW_IF(
      r != sizeof(T),
      r < 0 ? sys_errc::last_error() :
              (r == 0 ? sys_errc::common::sockets::not_connected : sys_errc::common::sockets::wrong_protocol_type));

    return recvfrom_result<T>{convert_byte_order<CS, T>(t), details::from_sockaddr(addr)};
  }

  template<packet_type T, ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> sendto(const T& t, const Address<SI.address_family>& addr)
    noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = convert_byte_order<CS>(t);

    details::socklen_type addrlen = sizeof(addr);

    auto r = ::sendto(
      m_handle_, reinterpret_cast<const char*>(&t_copy), sizeof(T), 0, details::to_sockaddr_ptr(&addr), addrlen);

    //return system error or wrong_protocol_type to indicate interruption of send
    EHL_THROW_IF(r != sizeof(T), r < 0 ? sys_errc::last_error() : sys_errc::common::sockets::wrong_protocol_type);
  }

  template<PollFlags PF, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<bool, sys_errc::ErrorCode, EHP> poll(int timeout_ms) noexcept(EHP != ehl::Policy::Exception)
  {
    pollfd fd{ .fd = m_handle_, .events = std::to_underlying(PF) };
    auto r = HPP_IFE(HPP_WIN_IMPL)(::WSAPoll)(::poll)(&fd, 1, timeout_ms);

    EHL_THROW_IF(r == HPP_IFE(HPP_WIN_IMPL)(SOCKET_ERROR)(-1), sys_errc::last_error());

    return static_cast<bool>(r);
  }

  template<PollFlags PF, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> poll() noexcept(EHP != ehl::Policy::Exception)
  {
    [[maybe_unused]] bool r = poll<PF, EHP>(-1);
  }
};

} //namespace cpps
