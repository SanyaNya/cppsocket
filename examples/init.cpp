#include <iostream>
#include <cppsocket/cppsocket.hpp>

struct Packet
{
  cpps::uint32_t i[4];
};

int main() try
{
  constexpr auto EHP = ehl::Policy::Exception;
  auto net = cpps::Net::make<EHP>();
  constexpr auto addr = cpps::Address<cpps::AddressFamily::IPv4>::make<EHP>("127.0.0.1", 6969);
  auto sock = net.socket<cpps::SI_IPv4_TCP, EHP>(addr, 10);
  auto conn = sock.accept<cpps::default_connection_settings, EHP>();
  Packet p = conn.recv<Packet, EHP>();
}
catch(const sys_errc::ErrorCode& err)
{
  std::cout << err.message() << std::endl;
}
