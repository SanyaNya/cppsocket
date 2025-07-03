#include <iostream>
#include <cppsocket/cppsocket.hpp>

struct Packet
{
  cpps::uint32_t i[4];

  constexpr bool is_valid() const noexcept
  {
    return i[0] == 1 && i[1] == 2 && i[2] == 3 && i[3] == 4;
  }

  friend std::ostream& operator<<(std::ostream& os, const Packet& p)
  {
    return os << "{" << p.i[0] << ", " << p.i[1] << ", " << p.i[2] << ", " << p.i[3] << "}";
  }
};

int main() try
{
  constexpr auto EHP = ehl::Policy::Exception;
  auto net = cpps::Net::make<EHP>();

  constexpr cpps::AddressIPv4 addr("127.0.0.1", 6969);
  auto sock = net.server_socket<cpps::SI_IPv4_TCP, EHP>(addr, 10);

  auto [conn, conn_addr] = sock.accept<cpps::default_connection_settings, EHP>();

  Packet p = conn.recv<Packet, EHP>();
  std::cout << "Recv packet: " << p << std::endl;

  conn.send<EHP>(p);
  std::cout << "Send packet: " << p << std::endl;
}
catch(const sys_errc::ErrorCode& err)
{
#if defined(WIN32) || defined(__MINGW32__)
  std::wcout
#else
  std::cout
#endif
  << "Error: " << err.message() << std::endl;
}
