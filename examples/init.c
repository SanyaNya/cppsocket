#include <stdio.h>
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
  if(sfd == -1){ perror("socket"); return 0; }
  struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = 6969, .sin_addr = {83994816}, .sin_zero = {}};
  int r;
  r = bind(sfd, (const struct sockaddr*)&addr, sizeof(addr));
  if(r != 0) { close(sfd); perror("bind"); return 0; }
  r = listen(sfd, 10);
  if(r != 0) {close(sfd); perror("listen"); return 0; }
  struct sockaddr_in new_addr;
  socklen_t new_addrlen = sizeof(new_addr);
  r = accept(sfd, (struct sockaddr*)&new_addr, &new_addrlen);
  if(r == -1) { close(sfd); perror("accept"); return 0; }
  close(sfd);
}
