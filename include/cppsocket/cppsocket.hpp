#pragma once

#include "socket.hpp"

namespace cpps
{

struct Net
{
  template<auto EHP = ehl::Policy::Exception>
  [[nodiscard]]
  static HPP_IF(HPP_POSIX_IMPL)(constexpr) ehl::Result_t<Net, sys_errc::ErrorCode, EHP> make()
    noexcept(EHP != ehl::Policy::Exception || HPP_POSIX_IMPL)
  {
#if HPP_WIN_IMPL
    WSAData wsaData;
    int r = WSAStartup(MAKEWORD(2, 2), &wsaData);

    EHL_THROW_IF(r != 0, sys_errc::ErrorCode(r));
#endif

    return Net{};
  }

#if HPP_WIN_IMPL
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
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_none, default_connection_settings>, sys_errc::ErrorCode, EHP>
  client_socket() const noexcept(EHP != ehl::Policy::Exception)
  {
    details::socket_resource sfd = ::socket((int)SI.address_family, (int)SI.type, (int)SI.protocol);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    return Socket<SI, inv_none, default_connection_settings>(std::move(sfd));
  }

  template<SocketInfo SI, ConnectionSettings SCS = default_connection_settings, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_connect, SCS>, sys_errc::ErrorCode, EHP>
  client_socket(const Address<SI.address_family>& dest_addr) const noexcept(EHP != ehl::Policy::Exception)
  {
    details::socket_resource sfd = ::socket((int)SI.address_family, (int)SI.type, (int)SI.protocol);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    int r = ::connect(sfd, details::to_sockaddr_ptr(&dest_addr), sizeof(dest_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_connect, SCS>(std::move(sfd));
  }

  template<SocketInfo SI, ConnectionSettings SCS = default_connection_settings, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_bind_connect, SCS>, sys_errc::ErrorCode, EHP>
  client_socket(const Address<SI.address_family>& bind_addr, const Address<SI.address_family>& dest_addr)
    const noexcept(EHP != ehl::Policy::Exception)
  {
    details::socket_resource sfd = ::socket((int)SI.address_family, (int)SI.type, (int)SI.protocol);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    int r;

    r = ::bind(sfd, details::to_sockaddr_ptr(&bind_addr), sizeof(bind_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    r = ::connect(sfd, details::to_sockaddr_ptr(&dest_addr), sizeof(dest_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_bind_connect, SCS>(std::move(sfd));
  }

  template<SocketInfo SI, auto EHP = ehl::Policy::Exception>
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_bind, default_connection_settings>, sys_errc::ErrorCode, EHP>
  server_socket(const Address<SI.address_family>& bind_addr)
    const noexcept(EHP != ehl::Policy::Exception)
  {
    details::socket_resource sfd = ::socket((int)SI.address_family, (int)SI.type, (int)SI.protocol);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    int r = ::bind(sfd, details::to_sockaddr_ptr(&bind_addr), sizeof(bind_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_bind, default_connection_settings>(std::move(sfd));
  }

  template<SocketInfo SI, auto EHP = ehl::Policy::Exception> requires (SI.type == SocketType::Stream)
  [[nodiscard]] ehl::Result_t<Socket<SI, inv_bind_listen, default_connection_settings>, sys_errc::ErrorCode, EHP>
  server_socket(const Address<SI.address_family>& bind_addr, unsigned max_connections)
    const noexcept(EHP != ehl::Policy::Exception)
  {
    details::socket_resource sfd = ::socket((int)SI.address_family, (int)SI.type, (int)SI.protocol);

    EHL_THROW_IF(sfd.is_invalid(), sys_errc::last_error());

    int r;

    r = ::bind(sfd, details::to_sockaddr_ptr(&bind_addr), sizeof(bind_addr));

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    r = ::listen(sfd, max_connections);

    EHL_THROW_IF(r != 0, sys_errc::last_error());

    return Socket<SI, inv_bind_listen, default_connection_settings>(std::move(sfd));
  }

private:
  constexpr Net() noexcept = default;
};

} //namespace cpps
