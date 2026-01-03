#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "frame.h"

#define IP "192.168.1.18"
#define PORT 56700

void frame_print(const FILE *fd, const lifx_frame_t frame) {
  printf("FRAME HEADER:\n");
  printf("size: %d\n", frame.header.size);
  printf("tagged: %d\n", frame.header.tagged);
  printf("source: %d\n", frame.header.source);
  printf("FRAME ADDRESS:\n");
  printf("target: ");
  for (int i = 0; i < 8; ++i) {
    printf("%02X", frame.header.target[i]);
  }
  printf("\n");
  printf("response: %d\n", frame.header.response);
  printf("acknowledgement: %d\n", frame.header.acknowledgement);
  printf("sequence: %d\n", frame.header.sequence);
  printf("Protocol Header:\n");
  printf("type: %d\n", frame.header.type);
}

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

  uint8_t ip[FRAME_SIZE_MAX] = {0};
  uint8_t *inboundPacket = ip;
  if ((res = recv(sfd, inboundPacket, FRAME_SIZE_MAX, 0)) == -1) {
    perror("recv");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  printf("Received packet length: %d\n", res);
  lifx_frame_t inboundFrame;
  lifx_decode_frame(&inboundFrame, &inboundPacket, res);

  /* for (int i = 0; i < res; ++i) { */
  /*   printf("%02X", inboundPacket[i]); */
  /* } */
  /* printf("\n"); */

  frame_print(stdout, inboundFrame);

  close(sfd);
  return 0;
}
