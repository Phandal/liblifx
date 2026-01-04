#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "frame.h"

#define IP "192.168.1.18"
#define PORT 56700

void payload_print(const lifx_payload_t *payload, lifx_message_type type) {
  if (payload == NULL) {
    return;
  }

  switch (type) {
  case StateLabel: {
    printf("label: %s\n", payload->state_label_payload.label);
    break;
  }
  case EchoResponse: {
    printf("echoing: %s\n", payload->echo_response_payload.echoing);
    break;
  }
  case Acknowledgement: {
    printf("NO PAYLOAD\n");
    break;
  }
  default:
    printf("Unimplemented payload print type: '%d'\n", type);
  }
}

void frame_print(const lifx_frame_t *frame) {
  /* if (frame == NULL) { */
  /*   return; */
  /* } */

  printf("FRAME HEADER:\n");
  printf("size: %d\n", frame->header.size);
  printf("tagged: %d\n", frame->header.tagged);
  printf("source: %d\n", frame->header.source);
  printf("FRAME ADDRESS:\n");
  printf("target: ");
  for (int i = 0; i < 8; ++i) {
    printf("%02X", frame->header.target[i]);
  }
  printf("\n");
  printf("response: %d\n", frame->header.response);
  printf("acknowledgement: %d\n", frame->header.acknowledgement);
  printf("sequence: %d\n", frame->header.sequence);
  printf("Protocol Header:\n");
  printf("type: %d\n", frame->header.type);

  printf("PAYLOAD:\n");
  payload_print(&frame->payload, frame->header.type);
}

int main(void) {
  /* lifx_header_t header = { */
  /*     .size = FRAME_HEADER_SIZE + 2, */
  /*     .tagged = 0, */
  /*     .source = 1234, */
  /*     .target = {0xD0, 0x73, 0xD5, 0x30, 0x9D, 0x57, 0, 0}, */
  /*     .response = 0, */
  /*     .acknowledgement = 1, */
  /*     .sequence = 1, */
  /*     .type = SetPower, */
  /* }; */
  /* lifx_payload_t payload = { */
  /*     .set_power_payload = */
  /*         { */
  /*             .level = 65535, */
  /*         }, */
  /* }; */

  /* lifx_header_t header = { */
  /*     .size = FRAME_HEADER_SIZE + 13, */
  /*     .tagged = 0, */
  /*     .source = 1234, */
  /*     .target = {0xD0, 0x73, 0xD5, 0x30, 0x9D, 0x57, 0, 0}, */
  /*     .response = 0, */
  /*     .acknowledgement = 1, */
  /*     .sequence = 1, */
  /*     .type = SetColor, */
  /* }; */
  /* lifx_payload_t payload = { */
  /*     .set_color_payload = */
  /*         { */
  /*             .hue = 21845, */
  /*             .saturation = 65535, */
  /*             .brightness = 65535, */
  /*             .kelvin = 3500, */
  /*             .duration = 0, */
  /*         }, */
  /* }; */

  /* lifx_header_t header = { */
  /*     .size = FRAME_HEADER_SIZE, */
  /*     .tagged = 0, */
  /*     .source = 1234, */
  /*     .target = {0xD0, 0x73, 0xD5, 0x30, 0x9D, 0x57, 0, 0}, */
  /*     .response = 1, */
  /*     .acknowledgement = 0, */
  /*     .sequence = 1, */
  /*     .type = GetLabel, */
  /* }; */
  /* lifx_payload_t payload = {0}; */

  lifx_header_t header = {
      .size = FRAME_HEADER_SIZE + 64,
      .tagged = 0,
      .source = 1234,
      .target = {0xD0, 0x73, 0xD5, 0x30, 0x9D, 0x57, 0, 0},
      .response = 1,
      .acknowledgement = 0,
      .sequence = 1,
      .type = EchoRequest,
  };
  lifx_payload_t payload = {
      .echo_request_payload =
          {
              .echoing = "Hello World",
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
  int res, recvSize, sfd;
  struct in_addr in_addr;
  inet_pton(AF_INET, IP, &in_addr);
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr = in_addr;
  addr.sin_port = htons(PORT);
  sfd = socket(AF_INET, SOCK_DGRAM, 0);
  res = connect(sfd, (struct sockaddr *)&addr, sizeof(addr));
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
  printf("Sent packet. Please check the light!\n");

  uint8_t ip[FRAME_SIZE_MAX] = {0};
  uint8_t *inboundPacket = ip;
  if ((recvSize = recv(sfd, inboundPacket, FRAME_SIZE_MAX, 0)) == -1) {
    perror("recv");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  printf("Received packet length: %d\n", recvSize);
  lifx_frame_t inboundFrame;
  res = lifx_decode_frame(&inboundFrame, &inboundPacket, recvSize);
  if (res == -1) {
    fprintf(stderr, "failed to decode frame\n");
    close(sfd);
    exit(EXIT_FAILURE);
  }

  /* for (int i = 0; i < res; ++i) { */
  /*   printf("%02X", inboundPacket[i]); */
  /* } */
  /* printf("\n"); */

  frame_print(&inboundFrame);

  close(sfd);
  return 0;
}
