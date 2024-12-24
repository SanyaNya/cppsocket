#pragma once

#include "details/helper_macros.hpp"

//Include platform specific headers
#if CPPS_WIN_IMPL
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif

  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>

  #pragma comment(lib, "Ws2_32.lib")
#elif CPPS_POSIX_IMPL
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
#endif

#include "ehl/ehl.hpp"
#include "system_errc/system_errc.hpp"
#include "strict_enum/strict_enum.hpp"
#include "details/ct_ip.hpp"
#include "details/convert_byte_order.hpp"
#include "is_packet.hpp"

namespace cpps
{

STRICT_ENUM(AddressFamily)
(
  IPv4 = AF_INET,
  IPv6 = AF_INET6,
);

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

STRICT_ENUM(AddressError)
(
  Invalid
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

struct ConnectionSettings
{
  bool convert_byte_order;
};

constexpr ConnectionSettings default_connection_settings = { .convert_byte_order = true };

namespace details
{

using SocketHandle = std::invoke_result_t<decltype(::socket), int, int, int>;
using socklen_type = PP_IFE(CPPS_WIN_IMPL)(int)(socklen_t);

constexpr auto invalid_socket = PP_IFE(CPPS_WIN_IMPL)(INVALID_SOCKET)(-1);

struct InvInfo
{
  bool binded;
  bool listening;
};

constexpr InvInfo inv_empty{};
constexpr InvInfo inv_bind = { .binded = true, .listening = false };
constexpr InvInfo inv_bind_listen = { .binded = true, .listening = true };

} //namespace details

template<AddressFamily AF>
class Address
{
  template<SocketInfo, details::InvInfo>
  friend struct Socket;

  using sockaddr_type = std::conditional_t<AF == AddressFamily::IPv4, sockaddr_in, sockaddr_in6>;

  sockaddr_type m_saddr_;

  constexpr Address() noexcept = default;
  constexpr Address(sockaddr_type saddr) noexcept : m_saddr_(saddr) {}

public:
  template<auto EHP = ehl::Policy::Exception>
  [[nodiscard]] static constexpr ehl::Result_t<Address, AddressError, EHP> make(const char* addr, in_port_t port)
    noexcept(EHP != ehl::Policy::Exception)
  {
    sockaddr_type saddr{};
    int r = 0;

    if constexpr(AF == AddressFamily::IPv4)
    {
      saddr.sin_family = AF_INET;
      saddr.sin_port = port;

      if consteval { saddr.sin_addr = details::ct_inet_pton<AF_INET>(addr); r = 1; }
      else { r = inet_pton(AF_INET, addr, &saddr.sin_addr); }
    }

    if constexpr(AF == AddressFamily::IPv6)
    {
      saddr.sin6_family = AF_INET6;
      saddr.sin6_port = port;

      if consteval { saddr.sin6_addr = details::ct_inet_pton<AF_INET6>(addr); r = 1; }
      else { r = inet_pton(AF_INET6, addr, &saddr.sin6_addr); }
    }

    EHL_THROW_IF(r == 0, AddressError(AddressError::Invalid));

    return Address(saddr);
  }
};

template<AddressFamily AF, ConnectionSettings CS>
struct IncomingConnection
{
  IncomingConnection(IncomingConnection&& s) noexcept : m_handle_(s.m_handle_)
  {
    s.m_handle_ = details::invalid_socket;
  }

  IncomingConnection(const IncomingConnection&) noexcept = delete;

  IncomingConnection& operator=(IncomingConnection&& s) noexcept
  {
    m_handle_ = std::exchange(s.m_handle_, details::invalid_socket);
    return *this;
  }

  IncomingConnection& operator=(const IncomingConnection&) noexcept = delete;

  ~IncomingConnection()
  {
    if(m_handle_ != details::invalid_socket)
    {
    #if CPPS_WIN_IMPL
      ::closesocket(m_handle_);
    #elif CPPS_POSIX_IMPL
      ::close(m_handle_);
    #endif
    }
  }

  template<packet_type T, auto EHP = ehl::Policy::Exception> requires std::is_trivially_copyable_v<T>
  [[nodiscard]] ehl::Result_t<T, sys_errc::ErrorCode, EHP> recv() noexcept(EHP != ehl::Policy::Exception)
  {
    T t;
    ssize_t r = ::recv(m_handle_, reinterpret_cast<std::byte*>(&t), sizeof(T), 0);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());

    if constexpr(CS.convert_byte_order)
      details::convert_byte_order(t);

    return t;
  }

  template<packet_type T, auto EHP = ehl::Policy::Exception> requires std::is_trivially_copyable_v<T>
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> send(const T& t) noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = t;

    if constexpr(CS.convert_byte_order)
      details::convert_byte_order(t_copy);

    ssize_t r = ::send(m_handle_, reinterpret_cast<const std::byte*>(&t_copy), sizeof(T), 0);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());
  }

private:
  template<SocketInfo, details::InvInfo>
  friend struct Socket;

  IncomingConnection(details::SocketHandle handle, Address<AF> addr) noexcept : m_handle_(handle), m_addr_(addr) {}

  details::SocketHandle m_handle_;
  Address<AF> m_addr_;
};

template<AddressFamily AF, ConnectionSettings CS>
struct OutgoingConnection
{
  constexpr OutgoingConnection(OutgoingConnection&& s) noexcept = default;

  constexpr OutgoingConnection(const OutgoingConnection&) noexcept = delete;

  constexpr OutgoingConnection& operator=(OutgoingConnection&& s) noexcept = default;

  constexpr OutgoingConnection& operator=(const OutgoingConnection&) noexcept = delete;

private:
  template<SocketInfo, details::InvInfo>
  friend struct Socket;

  constexpr OutgoingConnection(Address<AF> addr) noexcept : m_addr_(addr) {}

  Address<AF> m_addr_;
};

template<SocketInfo SI, details::InvInfo INV>
struct Socket
{
  static_assert(!(SI.type == SocketType::Stream && SI.protocol == SocketProtocol::UDP));
  static_assert(!(SI.type == SocketType::Datagram && SI.protocol == SocketProtocol::TCP));

  template<ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<IncomingConnection<SI.address_family, CS>, sys_errc::ErrorCode, EHP> accept()
    noexcept(EHP != ehl::Policy::Exception) requires (SI.type == SocketType::Stream && INV.binded && INV.listening)
  {
    Address<SI.address_family> addr;
    details::socklen_type addrlen = sizeof(addr);
    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of accept,
    //implementation know that pointer is from cast and deals with it
    details::SocketHandle r = ::accept(m_handle_, reinterpret_cast<sockaddr*>(&addr), &addrlen);

    EHL_THROW_IF(r == details::invalid_socket, sys_errc::last_error());

    return IncomingConnection<SI.address_family, CS>(r, addr);
  }

  template<ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
  [[nodiscard]]
  ehl::Result_t<OutgoingConnection<SI.address_family, CS>, sys_errc::ErrorCode, EHP> connect(const Address<SI.address_family>& addr)
    noexcept(EHP != ehl::Policy::Exception)
  {
    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of connect,
    //implementation know that pointer is from cast and deals with it
    int r = ::connect(m_handle_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return OutgoingConnection<SI.address_family, CS>(addr);
  }

  template<typename T>
  struct recvfrom_result
  {
    T value;
    Address<SI.address_family> addr;
  };

  template<packet_type T, ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
    requires (std::is_trivially_copyable_v<T> && SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<recvfrom_result<T>, sys_errc::ErrorCode, EHP> recvfrom()
    noexcept(EHP != ehl::Policy::Exception)
  {
    recvfrom_result<T> result;
    details::socklen_type addrlen = sizeof(result.addr);
    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of recvfrom,
    //implementation know that pointer is from cast and deals with it
    ssize_t r = ::recvfrom(
      m_handle_, reinterpret_cast<std::byte*>(&result.value), sizeof(T), 0, reinterpret_cast<sockaddr*>(&result.addr), &addrlen);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());

    if constexpr(CS.convert_byte_order)
      details::convert_byte_order(result.value);

    return result;
  }

  template<packet_type T, ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
    requires (std::is_trivially_copyable_v<T> && SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> sendto(const T& t, const Address<SI.address_family>& addr)
    noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = t;

    if constexpr(CS.convert_byte_order)
      details::convert_byte_order(t_copy);

    details::socklen_type addrlen = sizeof(addr);
    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of recvfrom,
    //implementation know that pointer is from cast and deals with it
    ssize_t r = ::sendto(
      m_handle_, reinterpret_cast<const std::byte*>(&t_copy), sizeof(T), 0, reinterpret_cast<const sockaddr*>(&addr), addrlen);

    EHL_THROW_IF(r < sizeof(T), sys_errc::last_error());
  }

  Socket(Socket&& s) : m_handle_(s.m_handle_)
  {
    s.m_handle_ = details::invalid_socket;
  }

  Socket(const Socket&) = delete;

  Socket& operator=(Socket&& s)
  {
    m_handle_ = std::exchange(s.m_handle_, details::invalid_socket);
    return *this;
  }

  Socket& operator=(const Socket&) = delete;

  ~Socket()
  {
    if(m_handle_ != details::invalid_socket)
    {
    #if CPPS_WIN_IMPL
      ::closesocket(m_handle_);
    #elif CPPS_POSIX_IMPL
      ::close(m_handle_);
    #endif
    }
  }

private:
  friend struct Net;

  Socket(details::SocketHandle handle) noexcept : m_handle_(handle) {}

  details::SocketHandle m_handle_;
};

struct Net
{
  template<auto EHP = ehl::Policy::Exception>
  [[nodiscard]]
  static PP_IF(CPPS_POSIX_IMPL)(constexpr) ehl::Result_t<Net, sys_errc::ErrorCode, EHP> make()
    noexcept(EHP != ehl::Policy::Exception || CPPS_POSIX_IMPL)
  {
#if CPPS_WIN_IMPL
    WSAData wsaData;
    int r = WSAStartup(MAKEWORD(2, 2), &wsaData);

    EHL_THROW_IF(r != 0, sys_errc::ErrorCode(r));
#endif

    return Net{};
  }

#if CPPS_WIN_IMPL
  ~Net()
  {
    WSACleanup();
    //WSANOTINITIALISED can`t be returned becasuse of class init invariant
    //WSAEINPROGRESS only returned in winsock 1.1
    //Just ignore WSAENETDOWN error because dectructing means we don`t need sockets working anymore
    //Otherwise if we need again sockets later, then init function will fail
  }
#endif

  template<SocketInfo SI, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, details::inv_empty>, sys_errc::ErrorCode, EHP>
  socket() const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(SI.address_family);
    constexpr int st = static_cast<int>(SI.type);
    constexpr int sp = static_cast<int>(SI.protocol);

    auto r = ::socket(af, st, sp);

    EHL_THROW_IF(r == details::invalid_socket, sys_errc::last_error());

    return Socket<SI, details::inv_empty>(r);
  }

  template<SocketInfo SI, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, details::inv_bind>, sys_errc::ErrorCode, EHP>
  socket(const Address<SI.address_family>& addr) const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(SI.address_family);
    constexpr int st = static_cast<int>(SI.type);
    constexpr int sp = static_cast<int>(SI.protocol);

    auto sfd = ::socket(af, st, sp);

    EHL_THROW_IF(sfd == details::invalid_socket, sys_errc::last_error());

    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of bind,
    //implementation know that pointer is from cast and deals with it
    int r = ::bind(sfd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, details::inv_bind>{sfd};
  }

  template<SocketInfo SI, auto EHP = ehl::Policy::Exception> requires (SI.type == SocketType::Stream)
  [[nodiscard]] ehl::Result_t<Socket<SI, details::inv_bind_listen>, sys_errc::ErrorCode, EHP>
  socket(const Address<SI.address_family>& addr, unsigned max_connections) const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(SI.address_family);
    constexpr int st = static_cast<int>(SI.type);
    constexpr int sp = static_cast<int>(SI.protocol);

    auto sfd = ::socket(af, st, sp);

    EHL_THROW_IF(sfd == details::invalid_socket, sys_errc::last_error());

    int r;

    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of bind,
    //implementation know that pointer is from cast and deals with it
    r = ::bind(sfd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    r = ::listen(sfd, max_connections);

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, details::inv_bind_listen>{sfd};
  }

private:
  constexpr Net() noexcept = default;
};

} //namespace cpps

#undef CPPS_WIN_IMPL
#undef CPPS_POSIX_IMPL
