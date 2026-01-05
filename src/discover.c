#include "frame.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
  lifx_frame_t frame = {
      .header =
          {
              .size = FRAME_HEADER_SIZE,
              .tagged = 1,
              .source = 1234,
              .sequence = 1,
              .acknowledgement = 0,
              .response = 0,
              .type = GetService,
          },
      .payload = {},
  };

  uint8_t p[FRAME_HEADER_SIZE] = {0};
  uint8_t *packet = p;
  int size;
  size = lifx_encode_frame(&frame, &packet, FRAME_HEADER_SIZE);
  if (size == -1) {
    fprintf(stderr, "failed to encode packet.");
    exit(EXIT_FAILURE);
  }

  int res, recvSize, sfd;
  struct in_addr in_addr;
  inet_pton(AF_INET, "192.168.1.255", &in_addr);
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr = in_addr;
  addr.sin_port = htons(56700);
  sfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sfd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int yes = 1;
  res = setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
  if (res == -1) {
    perror("setsockopt");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  res = connect(sfd, (struct sockaddr *)&addr, sizeof(addr));
  if (res == -1) {
    perror("connect");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  res = send(sfd, packet, size, 0);
  if (res == -1) {
    perror("send");
    close(sfd);
    exit(EXIT_FAILURE);
  }
  printf("Send packet");

  close(sfd);

  return 0;
}
