#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int main()
{
  int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = 6969, .sin_addr = {83994816}, .sin_zero = {}};
  int r;
  r = bind(sfd, (const struct sockaddr*)&addr, sizeof(addr));
  r = listen(sfd, 10);
  struct sockaddr_in new_addr;
  socklen_t new_addrlen = sizeof(new_addr);
  r = accept(sfd, (struct sockaddr*)&new_addr, &new_addrlen);
  close(sfd);
}
