#include <iostream>
#include <cppsocket/cppsocket.hpp>

int main() try
{
  constexpr auto EHP = ehl::Policy::Exception;
  auto net = cpps::Net::make<EHP>();
  constexpr auto addr = cpps::Address<cpps::AddressFamily::IPv4>::make<EHP>("192.168.1.50", 6969);
  auto sock = net.socket<cpps::SI_IPv4_TCP, EHP>(addr, 10);
  auto conn = sock.accept<EHP>();
}
catch(const sys_errc::ErrorCode& err)
{
  std::cout << err.message() << std::endl;
}
