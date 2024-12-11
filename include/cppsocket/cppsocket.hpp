#pragma once

//Define platform
#if defined(WIN32) || defined(__MINGW32__)
  #define CPPS_WIN_IMPL 1
  #define CPPS_POSIX_IMPL 0
#else
  #define CPPS_WIN_IMPL 0
  #define CPPS_POSIX_IMPL 1
#endif

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
#endif

#include "ehl/ehl.hpp"
#include "system_errc/system_errc.hpp"
#include "strict_enum/strict_enum.hpp"
#include "helper_macros.hpp"

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

template<AddressFamily AF, SocketType ST, SocketProtocol SP>
struct Socket
{
  using Handle = std::invoke_result_t<decltype(::socket), int, int, int>;

  static_assert(!(ST == SocketType::Stream && SP == SocketProtocol::UDP));
  static_assert(!(ST == SocketType::Datagram && SP == SocketProtocol::TCP));

private:
  friend struct Net;

  Socket(Handle handle) noexcept : m_handle_(handle) {}

  Handle m_handle_;
};

struct Net
{
  template<auto EHP = ehl::Policy::Exception>
  [[nodiscard]]
  static constexpr ehl::Result_t<Net, sys_errc::ErrorCode, EHP> make()
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

  template<AddressFamily AF, SocketType ST, SocketProtocol SP, auto EHP = ehl::Policy::Exception>
  [[nodiscard]]
  ehl::Result_t<Socket<AF, ST, SP>, sys_errc::ErrorCode, EHP> socket() const noexcept(EHP != ehl::Policy::Exception)
  {
    constexpr int af = static_cast<int>(AF);
    constexpr int st = static_cast<int>(ST);
    constexpr int sp = static_cast<int>(SP);

    auto r = ::socket(af, st, sp);

    EHL_THROW_IF(r == PP_IFE(CPPS_WIN_IMPL)(INVALID_SOCKET)(-1), sys_errc::last_error());

    return Socket<AF, ST, SP>(r);
  }

private:
  constexpr Net() noexcept = default;
};

} //namespace cpps

#undef CPPS_WIN_IMPL
#undef CPPS_POSIX_IMPL
