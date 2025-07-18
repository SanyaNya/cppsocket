#pragma once

#include "details/platform_headers.hpp"

#include <utility>
#include <algorithm>
#include <array>
#include <span>
#include <ehl/ehl.hpp>
#include <system_errc/system_errc.hpp>
#include <strict_enum/strict_enum.hpp>
#include "details/convert_byte_order.hpp"
#include "details/socket_resource.hpp"
#include "packet.hpp"
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

  template<typename V>
  struct variant_storage;

  template<typename... Ts>
  struct variant_storage<std::variant<Ts...>>
  {
    alignas(Ts...) std::byte data[(std::max)({sizeof(Ts)...})];
  };

  template<typename T, std::size_t total_size>
  struct extra_bytes
  {
    T obj;
    std::byte bytes[total_size - sizeof(T)];
  };

  template<typename To, std::size_t N>
  static constexpr To storage_bit_cast(std::byte (&storage)[N]) noexcept
  {
    return std::bit_cast<extra_bytes<To, N>>(storage).obj;
  }

  static constexpr sys_errc::ErrorCode not_connected_err       = sys_errc::common::sockets::not_connected;
  static constexpr sys_errc::ErrorCode wrong_protocol_type_err = sys_errc::common::sockets::wrong_protocol_type;
  static constexpr sys_errc::ErrorCode invalid_argument_err    = sys_errc::common::sockets::invalid_argument;

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

    return IncomingConnection<SI, inv_connect, CS>{std::move(r), details::from_sockaddr(addr)};
  }

  template<packet_type T, auto EHP = ehl::Policy::Exception> requires (INV.connected)
  [[nodiscard]] ehl::Result_t<valid_packet<T>, sys_errc::ErrorCode, EHP> recv() noexcept(EHP != ehl::Policy::Exception)
  {
    std::conditional_t<SI.type == SocketType::Datagram, extra_byte<T>, T> t;

    //ensure all data received for stream
    constexpr int flags = SI.type == SocketType::Stream ? MSG_WAITALL : 0;
    int r = ::recv(m_handle_, reinterpret_cast<char*>(&t), sizeof(t), flags);

    //return system error or not_connected to indicate connection issue or wrong_protocol_type to indicate wrong packet size
    EHL_THROW_IF(
      r != sizeof(T),
      r < 0 ? sys_errc::last_error() :
              (r == 0 ? not_connected_err : wrong_protocol_type_err));

    T result = convert_byte_order<SCS, T>(t);

    EHL_THROW_IF(!result.is_valid(), wrong_protocol_type_err);

    return std::bit_cast<valid_packet<T>>(result);
  }

  template<packet_variant_type V, auto EHP = ehl::Policy::Exception>
    requires (INV.connected && SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<valid_packet_variant<V>, sys_errc::ErrorCode, EHP> recv() noexcept(EHP != ehl::Policy::Exception)
  {
    extra_byte<variant_storage<V>> storage;

    int size = ::recv(m_handle_, reinterpret_cast<char*>(&storage), sizeof(storage), 0);

    EHL_THROW_IF(
      !(size > 0 && size < sizeof(storage)),
      size < 0 ? sys_errc::last_error() :
              (size == 0 ? not_connected_err : wrong_protocol_type_err));

    const auto validate = [&]<typename T>()
    {
      T t = convert_byte_order<SCS>(storage_bit_cast<T>(storage.obj.data));
      return size == sizeof(T) && t.is_valid();
    };

    const auto v = [&validate]<std::size_t... Is>(std::index_sequence<Is...>)
    {
      return std::array{(validate.template operator()<std::variant_alternative_t<Is, V>>())...};
    }(std::make_index_sequence<std::variant_size_v<V>>{});

    unsigned count = 0;
    for(bool b : v) if(b) ++count;
    EHL_THROW_IF(count != 1, wrong_protocol_type_err);

    return std::bit_cast<valid_packet_variant<V>>(
      [&]<std::size_t I = 0>(this const auto& self) -> V
      {
        using T = std::variant_alternative_t<I, V>;

        if(v[I]) return convert_byte_order<SCS>(storage_bit_cast<T>(storage.obj.data));

        if constexpr(I+1 != std::variant_size_v<V>)
          return self.template operator()<I+1>();
      }());
  }

  template<auto EHP = ehl::Policy::Exception, packet_type T> requires (INV.connected)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> send(const valid_packet<T>& t)
    noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = convert_byte_order<SCS, T>(t);

    auto r = ::send(m_handle_, reinterpret_cast<const char*>(&t_copy), sizeof(T), 0);

    //return system error or wrong_protocol_type to indicate interruption of send
    EHL_THROW_IF(r != sizeof(T), r < 0 ? sys_errc::last_error() : wrong_protocol_type_err);
  }

  template<auto EHP = ehl::Policy::Exception, packet_type T> requires (INV.connected)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> send(const T& t)
    noexcept(EHP != ehl::Policy::Exception)
  {
    EHL_THROW_IF(!t.is_valid(), invalid_argument_err);

    return send<EHP, T>(std::bit_cast<valid_packet<T>>(t));
  }

  template<auto EHP = ehl::Policy::Exception, packet_variant_type V> requires (INV.connected && SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> send(const valid_packet_variant<V>& v)
    noexcept(EHP != ehl::Policy::Exception)
  {
    V v_copy = v;
    const auto s = std::visit([&](auto& p)
    {
      p = convert_byte_order<SCS>(p);
      return std::span<const char>{reinterpret_cast<const char*>(&p), sizeof(p)};
    }, v_copy);

    auto r = ::send(m_handle_, s.data(), s.size_bytes(), 0);

    //return system error or wrong_protocol_type to indicate interruption of send
    EHL_THROW_IF(r != s.size_bytes(), r < 0 ? sys_errc::last_error() : wrong_protocol_type_err);
  }

  template<auto EHP = ehl::Policy::Exception, packet_variant_type V> requires (INV.connected && SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> send(const V& v)
    noexcept(EHP != ehl::Policy::Exception)
  {
    EHL_THROW_IF(!packet_variant_validate_predicate<V>(v), invalid_argument_err);

    return send<EHP, V>(std::bit_cast<valid_packet_variant<V>>(v));
  }

  template<typename T>
  struct recvfrom_result
  {
    valid_packet<T> value;
    Address<SI.address_family> addr;
  };

  template<typename... Ts>
  struct recvfrom_result<std::variant<Ts...>>
  {
    valid_packet_variant<std::variant<Ts...>> value;
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
              (r == 0 ? not_connected_err : wrong_protocol_type_err));

    T result = convert_byte_order<CS, T>(t);

    EHL_THROW_IF(!result.is_valid(), invalid_argument_err);

    return recvfrom_result<T>{std::bit_cast<valid_packet<T>>(result), details::from_sockaddr(addr)};
  }

  template<packet_variant_type V, ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<recvfrom_result<V>, sys_errc::ErrorCode, EHP> recvfrom()
    noexcept(EHP != ehl::Policy::Exception)
  {
    extra_byte<variant_storage<V>> storage;
    details::sockaddr_type<SI.address_family> addr;
    details::socklen_type addrlen = sizeof(addr);

    int size = ::recvfrom(
      m_handle_, reinterpret_cast<char*>(&storage), sizeof(storage), 0, details::to_sockaddr_ptr(&addr), &addrlen);

    EHL_THROW_IF(
      !(size > 0 && size < sizeof(storage)),
      size < 0 ? sys_errc::last_error() :
              (size == 0 ? not_connected_err : wrong_protocol_type_err));

    const auto validate = [&]<typename T>()
    {
      T t = convert_byte_order<CS>(storage_bit_cast<T>(storage.obj.data));
      return size == sizeof(T) && t.is_valid();
    };

    const auto v = [&validate]<std::size_t... Is>(std::index_sequence<Is...>)
    {
      return std::array{(validate.template operator()<std::variant_alternative_t<Is, V>>())...};
    }(std::make_index_sequence<std::variant_size_v<V>>{});

    unsigned count = 0;
    for(bool b : v) if(b) ++count;
    EHL_THROW_IF(count != 1, wrong_protocol_type_err);

    V res = [&]<std::size_t I = 0>(this const auto& self) -> V
    {
      using T = std::variant_alternative_t<I, V>;

      if(v[I]) return convert_byte_order<CS>(storage_bit_cast<T>(storage.obj.data));

      if constexpr(I+1 != std::variant_size_v<V>)
        return self.template operator()<I+1>();
    }();

    return recvfrom_result<V>{std::bit_cast<valid_packet_variant<V>>(res), details::from_sockaddr(addr)};
  }

  template<ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception, packet_type T>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> sendto(
    const valid_packet<T>& t, const Address<SI.address_family>& addr)
      noexcept(EHP != ehl::Policy::Exception)
  {
    T t_copy = convert_byte_order<CS, T>(t);

    details::socklen_type addrlen = sizeof(addr);

    auto r = ::sendto(
      m_handle_, reinterpret_cast<const char*>(&t_copy), sizeof(T), 0, details::to_sockaddr_ptr(&addr), addrlen);

    //return system error or wrong_protocol_type to indicate interruption of send
    EHL_THROW_IF(r != sizeof(T), r < 0 ? sys_errc::last_error() : wrong_protocol_type_err);
  }

  template<ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception, packet_type T>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> sendto(const T& t, const Address<SI.address_family>& addr)
      noexcept(EHP != ehl::Policy::Exception)
  {
    EHL_THROW_IF(!t.is_valid(), invalid_argument_err);

    return sendto<CS, EHP, T>(std::bit_cast<valid_packet<T>>(t), addr);
  }

  template<ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception, packet_variant_type V>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> sendto(
    const valid_packet_variant<V>& v, const Address<SI.address_family>& addr)
      noexcept(EHP != ehl::Policy::Exception)
  {
    V v_copy = v;
    const auto s = std::visit([&](auto& p)
    {
      p = convert_byte_order<CS>(p);
      return std::span<const char>{reinterpret_cast<const char*>(&p), sizeof(p)};
    }, v_copy);

    details::socklen_type addrlen = sizeof(addr);

    auto r = ::sendto(
      m_handle_, s.data(), s.size_bytes(), 0, details::to_sockaddr_ptr(&addr), addrlen);

    //return system error or wrong_protocol_type to indicate interruption of send
    EHL_THROW_IF(r != s.size_bytes(), r < 0 ? sys_errc::last_error() : wrong_protocol_type_err);
  }

  template<ConnectionSettings CS = default_connection_settings, auto EHP = ehl::Policy::Exception, packet_variant_type V>
    requires (SI.type == SocketType::Datagram)
  [[nodiscard]] ehl::Result_t<void, sys_errc::ErrorCode, EHP> sendto(
    const V& v, const Address<SI.address_family>& addr)
      noexcept(EHP != ehl::Policy::Exception)
  {
    EHL_THROW_IF(!packet_variant_validate_predicate<V>(v), invalid_argument_err);

    return sendto<CS, EHP, V>(std::bit_cast<valid_packet_variant<V>>(v), addr);
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
