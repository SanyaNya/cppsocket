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
#include "strict_enum/strict_enum.hpp"
#include "helper_macros.hpp"

namespace cpps
{

STRICT_ENUM(InitError)
(
PP_IF(CPPS_WIN_IMPL)
(
  SystemNotReady = WSASYSNOTREADY,
  ProcessLimit   = WSAEPROCLIM,
  //VersionNotSupported = WSAVERNOTSUPPORTED, //Only happens with dll
  //OperationInProgress = WSAEINPROGRESS,     //Only happens with winsock 1.1
  //InvalidArg          = WSAEFAULT           //Only happens if pointer to WSAData is invalid
)
);

STRICT_ENUM(GenericInitError)
(
  SystemNotReady PP_IF(CPPS_WIN_IMPL)(= WSASYSNOTREADY),
  ProcessLimit   PP_IF(CPPS_WIN_IMPL)(= WSAEPROCLIM),
);

struct Net
{
  template<auto EHP = ehl::Policy::Exception>
  static constexpr ehl::Result_t<Net, InitError, EHP> make() 
    noexcept(EHP != ehl::Policy::Exception || CPPS_POSIX_IMPL)
  {
#if CPPS_WIN_IMPL
    WSAData wsaData;
    int r = WSAStartup(MAKEWORD(2, 2), &wsaData);

    EHL_THROW_IF(r != 0, static_cast<InitError>(r));
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

private:
  constexpr Net() noexcept = default;
};

} //namespace cpps

#undef CPPS_WIN_IMPL
#undef CPPS_POSIX_IMPL
