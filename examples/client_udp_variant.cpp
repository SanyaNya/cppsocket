#include <iostream>
#include "cppsocket/cppsocket.hpp"

struct Packet1
{
  cpps::uint32_t i[4];

  constexpr bool is_valid() const noexcept
  {
    return i[0] == 1 && i[1] == 2 && i[2] == 3 && i[3] == 4;
  }

  friend std::ostream& operator<<(std::ostream& os, const Packet1& p)
  {
    return os << "{" << p.i[0] << ", " << p.i[1] << ", " << p.i[2] << ", " << p.i[3] << "}";
  }
};

struct Packet2
{
  cpps::ieee754_float32_t i;

  constexpr bool is_valid() const noexcept
  {
    return i == 1.25f;
  }

  friend std::ostream& operator<<(std::ostream& os, const Packet2& p)
  {
    return os << "{" << p.i << "}";
  }
};

int main() try
{
  constexpr auto EHP = ehl::Policy::Exception;
  constexpr auto CS = cpps::ConnectionSettings{.convert_byte_order = true};
  std::srand(std::time({}));
  auto net = cpps::Net::make<EHP>();

  constexpr cpps::AddressIPv4 addr("127.0.0.1", 6969);
  auto sock = net.client_socket<cpps::SI_IPv4_UDP, CS, EHP>(addr);

  using V = std::variant<Packet1, Packet2>;
  V v{std::rand() % 2 == 0 ? V{Packet1{1,2,3,4}} : V{Packet2{1.25f}}};
  sock.send<EHP>(v);
  std::visit([](const auto& p) { std::cout << "Send packet: " << p << std::endl; }, v);

  V vr = sock.recv<V, EHP>();
  std::visit([](const auto& p) { std::cout << "Recv packet: " << p << std::endl; }, vr);
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
