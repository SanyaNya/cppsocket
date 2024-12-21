#include <iostream>
#include <cppsocket/cppsocket.hpp>

int main()
{
  constexpr auto EHP = ehl::Policy::Unreachable;
  auto net = cpps::Net::make<EHP>();
  auto sock = net.socket<cpps::AddressFamily::IPv4, cpps::SocketType::Stream, cpps::SocketProtocol::TCP, EHP>();
  constexpr auto addr = cpps::Address<cpps::AddressFamily::IPv4>::make<EHP>("192.168.1.50", 6969);
  sock.bind<EHP>(addr);
  sock.listen<EHP>(10);
  auto conn = sock.accept<EHP>();
}
