#include <iostream>
#include <cppsocket/cppsocket.hpp>

int main()
{
  auto net = cpps::Net::make<ehl::Policy::Expected>();
  if(!net)
    std::cout << "System Error: " << net.error().message().data() << std::endl;
}
