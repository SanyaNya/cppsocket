#include <iostream>
#include <cppsocket/cppsocket.hpp>

struct Packet
{
  cpps::uint32_t i[4];

  friend std::ostream& operator<<(std::ostream& os, const Packet& p)
  {
    return os << "{" << p.i[0].underlying_value() << ", " << p.i[1].underlying_value() << ", " << p.i[2].underlying_value() << ", " << p.i[3].underlying_value() << "}";
  }
};

int main() try
{
  constexpr auto EHP = ehl::Policy::Exception;
  auto net = cpps::Net::make<EHP>();

  constexpr auto addr = cpps::Address<cpps::AddressFamily::IPv4>::make<EHP>("127.0.0.1", 6969);
  auto sock = net.server_socket<cpps::SI_IPv4_TCP, EHP>(addr, 10);

  auto conn = sock.accept<cpps::default_connection_settings, EHP>();

  Packet p = conn.recv<Packet, EHP>();
  std::cout << "Received packet: " << p << std::endl;

  conn.send<Packet, EHP>(p);
  std::cout << "Send packet: " << p << std::endl;
}
catch(const sys_errc::ErrorCode& err)
{
  std::cout << "Error: " << err.message() << std::endl;
}
