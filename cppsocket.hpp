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

#include "ehl.hpp"

namespace cpps
{

enum class InitError
{
  SystemNotReady      = WSASYSNOTREADY,
  //VersionNotSupported = WSAVERNOTSUPPORTED, //Only happens with dll
  //OperationInProgress = WSAEINPROGRESS,     //Only happens with winsock 1.1
  ProcessLimit        = WSAEPROCLIM,
  //InvalidArg          = WSAEFAULT           //Only happens if pointer to WSAData is invalid
};

struct Net
{
#if CPPS_WIN_IMPL
  ~Net()
  {
    WSACleanup();
    //WSANOTINITIALISED can`t be returned becasuse of class init invariant`
    //WSAEINPROGRESS only returned in winsock 1.1
    //Just ignore WSAENETDOWN error because dectructing means we don`t need sockets working anymore
    //Otherwise if we need again sockets later, then init function will fail
  }
#endif

private:
  template<auto EHP>
  friend constexpr ehl::Result_t<Net, InitError, EHP> make_net() noexcept(EHP != ehl::Policy::Exception || CPPS_POSIX_IMPL);

  constexpr Net() noexcept = default;
};

template<auto EHP = ehl::Policy::Exception>
constexpr ehl::Result_t<Net, InitError, EHP> make_net() noexcept(EHP != ehl::Policy::Exception || CPPS_POSIX_IMPL)
{
#if CPPS_WIN_IMPL
  WSAData wsaData;
  int r = WSAStartup(MAKEWORD(2, 2), &wsaData);

  [[assume(r != WSAVERNOTSUPPORTED && r != WSAEINPROGRESS && r != WSAEFAULT)]];
  EHL_THROW_IF(r != 0, static_cast<InitError>(r));
#endif

  return Net{};
}

} //namespace cpps

#undef CPPS_WIN_IMPL
#undef CPPS_POSIX_IMPL
