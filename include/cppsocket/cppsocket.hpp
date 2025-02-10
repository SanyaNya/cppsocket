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

#include <ehl/ehl.hpp>
#include "socket.hpp"

namespace cpps
{

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

  template<SocketInfo SI, ConnectionSettings SCS = default_connection_settings, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_connect, SCS>, sys_errc::ErrorCode, EHP>
  client_socket(const Address<SI.address_family>& dest_addr) const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(SI.address_family);
    constexpr int st = static_cast<int>(SI.type);
    constexpr int sp = static_cast<int>(SI.protocol);

    details::socket_resource sfd = ::socket(af, st, sp);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of connect,
    //implementation know that pointer is from cast and deals with it
    int r = ::connect(sfd, reinterpret_cast<const sockaddr*>(&dest_addr), sizeof(dest_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_connect, SCS>(std::move(sfd));
  }

  template<SocketInfo SI, ConnectionSettings SCS = default_connection_settings, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_bind_connect, SCS>, sys_errc::ErrorCode, EHP>
  client_socket(const Address<SI.address_family>& bind_addr, const Address<SI.address_family>& dest_addr)
    const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(SI.address_family);
    constexpr int st = static_cast<int>(SI.type);
    constexpr int sp = static_cast<int>(SI.protocol);

    details::socket_resource sfd = ::socket(af, st, sp);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    int r;

    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of bind,
    //implementation know that pointer is from cast and deals with it
    r = ::bind(sfd, reinterpret_cast<const sockaddr*>(&bind_addr), sizeof(bind_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of connect,
    //implementation know that pointer is from cast and deals with it
    r = ::connect(sfd, reinterpret_cast<const sockaddr*>(&dest_addr), sizeo(dest_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_bind_connect, SCS>(std::move(sfd));
  }

  template<SocketInfo SI, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_bind, default_connection_settings>, sys_errc::ErrorCode, EHP>
  server_socket(const Address<SI.address_family>& bind_addr)
    const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(SI.address_family);
    constexpr int st = static_cast<int>(SI.type);
    constexpr int sp = static_cast<int>(SI.protocol);

    details::socket_resource sfd = ::socket(af, st, sp);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of bind,
    //implementation know that pointer is from cast and deals with it
    int r = ::bind(sfd, reinterpret_cast<const sockaddr*>(&bind_addr), sizeof(bind_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_bind, default_connection_settings>(std::move(sfd));
  }

  template<SocketInfo SI, auto EHP = ehl::Policy::Exception> requires (SI.type == SocketType::Stream)
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_bind_listen, default_connection_settings>, sys_errc::ErrorCode, EHP>
  server_socket(const Address<SI.address_family>& bind_addr, unsigned max_connections)
    const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(SI.address_family);
    constexpr int st = static_cast<int>(SI.type);
    constexpr int sp = static_cast<int>(SI.protocol);

    details::socket_resource sfd = ::socket(af, st, sp);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    int r;

    //getting pointer by reinterpret_cast is not UB,
    //accessing through this pointer is UB, but access is done by implementation of bind,
    //implementation know that pointer is from cast and deals with it
    r = ::bind(sfd, reinterpret_cast<const sockaddr*>(&bind_addr), sizeof(bind_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    r = ::listen(sfd, max_connections);

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_bind_listen, default_connection_settings>(std::move(sfd));
  }

private:
  constexpr Net() noexcept = default;
};

} //namespace cpps

#undef CPPS_WIN_IMPL
#undef CPPS_POSIX_IMPL
