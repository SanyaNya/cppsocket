#include <iostream>
#include <cppsocket/cppsocket.hpp>

int main()
{
  auto net = cpps::Net::make<ehl::Policy::Expected>();
  if(!net)
  {
    auto ge = static_cast<cpps::GenericInitError>(net.error());
    if(ge == cpps::GenericInitError::SystemNotReady)
    {
      std::cout << "System not ready!\n";
    }
    else if(ge == cpps::GenericInitError::ProcessLimit)
    {
      std::cout << "Process limit!\n";
    }
  }
}
