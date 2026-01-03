#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "frame.h"

#define IP "192.168.1.18"
#define PORT 56700

int main(void) {
  lifx_header_t header = {
      .size = 49,
      .tagged = 0,
      .source = 1234,
      .target = {0xD0, 0x73, 0xD5, 0x30, 0x9D, 0x57, 0, 0},
      .response = 0,
      .acknowledgement = 1,
      .sequence = 1,
      .type = SetColor,
  };

  lifx_payload_t payload = {
      .set_color_payload =
          {
              .hue = 21845,
              .saturation = 65535,
              .brightness = 65535,
              .kelvin = 3500,
              .duration = 0,
          },
  };

  lifx_frame_t frame = {
      .header = header,
      .payload = payload,
  };

  uint8_t p[header.size];
  uint8_t *packet = p;

  int size = lifx_encode_frame(&frame, &packet, header.size);
  if (size == -1) {
    fprintf(stderr, "failed to encode frame\n");
    exit(EXIT_FAILURE);
  }

  /* for (int i = 0; i < size; ++i) { */
  /*   printf("%2d: %02X\n", i, packet[i]); */
  /* } */

  printf("Sending packet to light at %s:%d\n", IP, PORT);
  struct in_addr in_addr;
  inet_pton(AF_INET, IP, &in_addr);
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr = in_addr;
  addr.sin_port = htons(PORT);
  int sfd = socket(AF_INET, SOCK_DGRAM, 0);
  int res = connect(sfd, (struct sockaddr *)&addr, sizeof(addr));
  if (res == -1) {
    perror("connect");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  if (send(sfd, packet, frame.header.size, 0) == -1) {
    perror("send");
    close(sfd);
    exit(EXIT_FAILURE);
  }
  printf("Sent packet. Please check the light is green!\n");

  close(sfd);
}
