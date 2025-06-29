#include <iostream>
#include "cppsocket/cppsocket.hpp"

struct Packet
{
  cpps::uint32_t i[4];

  constexpr bool is_valid() const noexcept
  {
    return
      i[0].underlying_value() == 1 &&
      i[1].underlying_value() == 2 &&
      i[2].underlying_value() == 3 &&
      i[3].underlying_value() == 4;
  }

  friend std::ostream& operator<<(std::ostream& os, const Packet& p)
  {
    return os << "{" << p.i[0].underlying_value() << ", " << p.i[1].underlying_value() << ", " << p.i[2].underlying_value() << ", " << p.i[3].underlying_value() << "}";
  }
};

int main() try
{
  auto net = cpps::Net::make();

  constexpr cpps::AddressIPv4 addr("127.0.0.1", 6969);
  auto sock = net.client_socket<cpps::SI_IPv4_TCP>(addr);

  Packet p{{1, 2, 3, 4}};
  sock.send(p);
  std::cout << "Send packet: " << p << std::endl;

  Packet r = sock.recv<Packet>();
  std::cout << "Recv packet: " << r << std::endl;
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
