#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "frame.h"

#define BROADCAST "255.255.255.255"
#define HOST "0.0.0.0"
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
  struct in_addr ip;
  if (inet_pton(AF_INET, HOST, &ip) != 1) {
    fprintf(stderr, "failed to convert ip address to network '%s'\n", HOST);
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr = ip;

  struct in_addr b_addr;
  if (inet_pton(AF_INET, BROADCAST, &b_addr) != 1) {
    fprintf(stderr, "failed to convert ip address to network '%s'\n",
            BROADCAST);
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in broadcast = {0};
  broadcast.sin_family = AF_INET;
  broadcast.sin_addr = b_addr;
  broadcast.sin_port = htons(PORT);

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

  int sfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sfd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
  printf("created socket!\n");

  int yes = 1;
  if (setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  printf("set the broadcast flag!\n");

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }
  printf("bound the socket to a port!\n");

  uint8_t p[FRAME_SIZE_MAX] = {0};
  uint8_t *packet = p;
  int size;
  size = lifx_encode_frame(&frame, &packet, FRAME_SIZE_MAX);
  if (size == -1) {
    fprintf(stderr, "failed to encode lifx packet\n");
    exit(EXIT_FAILURE);
  }
  printf("encoded the packet with size %d!\n", size);

  if (sendto(sfd, packet, size, 0, (struct sockaddr *)&broadcast,
             sizeof(broadcast)) != size) {
    fprintf(stderr, "failed to broadcast packet\n");
    exit(EXIT_FAILURE);
  }
  printf("broadcasted packet!\n");

  struct pollfd pfds = {
      .fd = sfd,
      .events = POLLIN,
  };

  puts("");
  while (1) {
    int ready = poll(&pfds, 1, 500); /* Wait 500 miliseconds for an event */
    if (ready == -1) {
      perror("poll");
      break;
    }

    if (ready == 0) {
      break;
    }

    uint8_t inbound_p[FRAME_SIZE_MAX] = {0};
    uint8_t *inbound_packet = inbound_p;
    struct sockaddr_storage storage = {0};
    socklen_t storage_len = sizeof(storage);
    char recv_ip[INET_ADDRSTRLEN] = {0};
    uint16_t recv_port = 0;

    int size = recvfrom(sfd, inbound_packet, FRAME_SIZE_MAX, 0,
                        (struct sockaddr *)&storage, &storage_len);
    if (size == -1) {
      perror("recvfrom");
      exit(EXIT_FAILURE);
    }

    if (storage.ss_family == AF_INET) {
      struct sockaddr_in *addr = (struct sockaddr_in *)&storage;
      if (inet_ntop(AF_INET, &addr->sin_addr, recv_ip, INET6_ADDRSTRLEN) ==
          NULL) {
        perror("inet_ntop ipv4");
        exit(EXIT_FAILURE);
      }
      recv_port = ntohs(addr->sin_port);
    } else {
      struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&storage;
      if (inet_ntop(AF_INET6, &addr->sin6_addr, recv_ip, INET6_ADDRSTRLEN) ==
          NULL) {
        perror("inet_ntop ipv6");
        exit(EXIT_FAILURE);
      }
      recv_port = ntohs(addr->sin6_port);
    }
    printf("received message from %s on port %d\n", recv_ip, recv_port);

    lifx_frame_t inbound_frame = {0};
    if (lifx_decode_frame(&inbound_frame, &inbound_packet, size) == -1) {
      fprintf(stderr, "failed to decode lifx packet\n");
      exit(EXIT_FAILURE);
    }
    frame_print(&inbound_frame);
    puts("");
  }

  close(sfd);
  return 0;
}
