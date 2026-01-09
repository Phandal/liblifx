#include "frame.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define BROADCAST "255.255.255.255"
#define HOST "localhost"
#define PORT 56700
#define LOCAL_PORT 8080

void payload_print(const lifx_payload_t *payload, lifx_message_type type) {
  if (payload == NULL) {
    return;
  }

  switch (type) {
  case StateLabel: {
    printf("label: %s\n", payload->state_label_payload.label);
    break;
  }
  case StateService: {
    printf("service: %d\n", payload->state_service_payload.service);
    printf("port: %d\n", payload->state_service_payload.port);
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

int start_server() {
  int res, sfd;
  struct sockaddr_in server_addr;
  struct in_addr in_addr = {0};
  inet_pton(AF_INET, HOST, &in_addr);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr = in_addr;
  server_addr.sin_port = htons(LOCAL_PORT);
  sfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sfd == -1) {
    perror("socket");
    return -1;
  }

  int yes = 1;
  res = setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
  if (res == -1) {
    perror("setsockopt");
    close(sfd);
    return -1;
  }

  res = bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (res == -1) {
    perror("bind");
    close(sfd);
    return -1;
  }

  return sfd;
}

int send_packet(int sfd, lifx_frame_t *frame) {

  uint8_t p[FRAME_HEADER_SIZE] = {0};
  uint8_t *packet = p;
  int size;
  size = lifx_encode_frame(frame, &packet, FRAME_HEADER_SIZE);
  if (size == -1) {
    fprintf(stderr, "failed to encode packet\n");
    return -1;
  }

  int res;
  struct in_addr in_addr;
  inet_pton(AF_INET, BROADCAST, &in_addr);
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr = in_addr;
  addr.sin_port = htons(PORT);

  res = sendto(sfd, packet, size, 0, (struct sockaddr *)&addr, sizeof(addr));
  if (res == -1) {
    perror("sendto");
    return -1;
  }

  return res;
}

int get_lights(int sfd, lifx_frame_t **frame, int n) {
  if (frame == NULL) {
    return -1;
  }

  int res;

  uint8_t p[FRAME_SIZE_MAX] = {0};
  uint8_t *packet = p;
  res = recvfrom(sfd, packet, FRAME_SIZE_MAX, 0, NULL, NULL);
  if (res == -1) {
    perror("recvfrom");
    return -1;
  }

  puts("Recieved:");
  for (int i = 0; i < res; ++i) {
    printf("%02X", packet[i]);
  }
  puts("");

  res = lifx_decode_frame(*frame, &packet, res);
  if (res == -1) {
    fprintf(stderr, "failed to decode packet\n");
    return -1;
  }

  return 1;
}

int main(void) {
  lifx_frame_t frame = {
      .header =
          {
              .size = FRAME_HEADER_SIZE,
              .tagged = 1,
              .source = 1234,
              .sequence = 1,
              .acknowledgement = 0,
              .response = 1,
              .type = GetService,
          },
      .payload = {},
  };

  int sfd = start_server();
  if (sfd == -1) {
    fprintf(stderr, "failed to start server\n");
    exit(EXIT_FAILURE);
  }
  printf("Started server\n");

  int res = send_packet(sfd, &frame);
  if (res == -1) {
    fprintf(stderr, "failed to send packet\n");
    close(sfd);
    exit(EXIT_FAILURE);
  }
  printf("Sent packet\n");

  lifx_frame_t inboundFrame = {0};
  lifx_frame_t *inboundFrames = &inboundFrame;
  res = get_lights(sfd, &inboundFrames, 1);
  if (res == -1) {
    fprintf(stderr, "failed to get lights\n");
    close(sfd);
    exit(EXIT_FAILURE);
  }
  close(sfd);

  frame_print(&inboundFrame);

  return 0;
}
