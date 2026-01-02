#include "frame.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint8_t *const *buf;
  const int capacity;
  int size;
} lifx_packet_t;

int write_packet(lifx_packet_t *packet, uint64_t v, int n) {
  if (packet->size + n > packet->capacity) {
    fprintf(stderr, "[WARN] overwrite packet\n");
    exit(EXIT_FAILURE);
  }

  uint8_t write = 0;

  for (int i = 0; i < n; ++i) {
    write = v >> (8 * i);
    (*packet->buf)[packet->size + i] = write;
  }
  packet->size += n;

  return packet->size;
}

int write_uint8(lifx_packet_t *packet, uint8_t v) {
  return write_packet(packet, v, 1);
}

int write_uint16(lifx_packet_t *packet, uint16_t v) {
  return write_packet(packet, v, 2);
}

int write_uint32(lifx_packet_t *packet, uint32_t v) {
  return write_packet(packet, v, 4);
}

int write_uint64(lifx_packet_t *packet, uint64_t v) {
  return write_packet(packet, v, 8);
}

int encode_set_color_payload(lifx_packet_t *packet,
                             const lifx_set_color_t *payload) {
  write_uint8(packet, 0); // Reserved
  write_uint16(packet, payload->hue);
  write_uint16(packet, payload->saturation);
  write_uint16(packet, payload->brightness);
  write_uint16(packet, payload->kelvin);
  write_uint32(packet, payload->duration);
  return packet->size;
}

int encode_payload(lifx_packet_t *packet, lifx_message_type type,
                   const lifx_payload_t *payload) {
  switch (type) {
  case SetColor:
    return encode_set_color_payload(packet, &payload->set_color_payload);
  default:
    break;
  }

  return packet->size;
}

int lifx_encode_frame(const lifx_frame_t *frame, uint8_t *const *buf,
                      const size_t n) {
  if (frame == NULL || buf == NULL || *buf == NULL) {
    return -1;
  }

  if (frame->header.size < FRAME_HEADER_SIZE) {
    return -1;
  }

  lifx_packet_t packet = {
      .buf = buf,
      .capacity = n,
      .size = 0,
  };

  /* Frame Header */
  write_uint16(&packet, frame->header.size);
  uint16_t protocol = 0;
  protocol |= frame->header.protocol;
  protocol |= frame->header.addressable << 12;
  protocol |= frame->header.tagged << 13;
  protocol |= frame->header.origin << 14;
  write_uint16(&packet, protocol);
  write_uint32(&packet, frame->header.source);

  /* Frame Address */
  for (int i = 0; i < 8; ++i) {
    write_uint8(&packet, frame->header.target[i]);
  }
  for (int i = 0; i < 6; ++i) { // Reserved Bytes
    write_uint8(&packet, 0);
  }
  uint8_t flags = 0;
  flags |= frame->header.response;
  flags |= frame->header.acknowledgement < 1;
  write_uint8(&packet, flags);
  write_uint8(&packet, frame->header.sequence);

  /* Protocol Header */
  write_uint64(&packet, 0); // Reserved Bytes
  write_uint16(&packet, frame->header.type);
  write_uint16(&packet, 0); // Reserved Bytes

  /* Payload */
  encode_payload(&packet, frame->header.type, &(frame->payload));

  return packet.size;
}
