#include "frame.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO: Remove all failure points, as they should be handled by the user.
 * Instead find a way to signify an error */

typedef struct {
  uint8_t *const *buf;
  const int capacity;
  int cursor;
} lifx_packet_t;

int write_packet(lifx_packet_t *packet, uint64_t v, int n) {
  if ((packet->cursor + n) > packet->capacity) {
    fprintf(stderr, "[WARN] overwrite packet\n");
    exit(EXIT_FAILURE);
  }

  uint8_t write = 0;

  for (int i = 0; i < n; ++i) {
    write = v >> (8 * i);
    (*packet->buf)[packet->cursor + i] = write;
  }
  packet->cursor += n;

  return packet->cursor;
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

uint64_t read_packet(lifx_packet_t *packet, int n) {
  if ((packet->cursor + n) > packet->capacity) {
    fprintf(stderr, "[WARN] overwrite packet\n");
    exit(EXIT_FAILURE);
  }

  uint64_t output = 0;
  uint8_t value = 0;
  for (int i = 0; i < n; ++i) {
    value = (*packet->buf)[packet->cursor + i];
    output |= value << (8 * i);
  }
  packet->cursor += n;

  return output;
}

uint8_t read_uint8(lifx_packet_t *packet) { return read_packet(packet, 1); }

uint16_t read_uint16(lifx_packet_t *packet) { return read_packet(packet, 2); }

uint32_t read_uint32(lifx_packet_t *packet) { return read_packet(packet, 4); }

uint64_t read_uint64(lifx_packet_t *packet) { return read_packet(packet, 8); }

int encode_set_color_payload(lifx_packet_t *packet,
                             const lifx_set_color_payload_t *payload) {
  write_uint8(packet, FRAME_RESERVED); // Reserved
  write_uint16(packet, payload->hue);
  write_uint16(packet, payload->saturation);
  write_uint16(packet, payload->brightness);
  write_uint16(packet, payload->kelvin);
  write_uint32(packet, payload->duration);
  return packet->cursor;
}

int encode_set_power_payload(lifx_packet_t *packet,
                             const lifx_set_power_payload_t *payload) {
  write_uint16(packet, payload->level);
  return packet->cursor;
}

int encode_echo_request_payload(lifx_packet_t *packet,
                                const lifx_echo_request_payload_t *payload) {
  for (int i = 0; i < 64; ++i) {
    write_uint8(packet, payload->echoing[i]);
  }
  return packet->cursor;
}

int encode_payload(lifx_packet_t *packet, lifx_message_type type,
                   const lifx_payload_t *payload) {
  switch (type) {
  case SetPower:
    return encode_set_power_payload(packet, &payload->set_power_payload);
  case SetColor:
    return encode_set_color_payload(packet, &payload->set_color_payload);
  case EchoRequest:
    return encode_echo_request_payload(packet, &payload->echo_request_payload);
  case GetService:
  case GetLabel:
    return packet->cursor;
  default:
    fprintf(stderr, "[WARN] invalid payload type '%d'\n", type);
    exit(EXIT_FAILURE);
  }
}

int lifx_encode_frame(const lifx_frame_t *frame, uint8_t *const *buf,
                      const size_t n) {
  if (frame == NULL || buf == NULL) {
    return -1;
  }

  if (frame->header.size < FRAME_HEADER_SIZE) {
    return -1;
  }

  lifx_packet_t packet = {
      .buf = buf,
      .capacity = n,
      .cursor = 0,
  };

  /* Frame Header */
  write_uint16(&packet, frame->header.size);
  uint16_t protocol = FRAME_PROTOCOL;
  protocol |= FRAME_ADDRESSABLE << 12;
  protocol |= frame->header.tagged << 13;
  protocol |= FRAME_ORIGIN << 14;
  write_uint16(&packet, protocol);
  write_uint32(&packet, frame->header.source);

  /* Frame Address */
  for (int i = 0; i < 8; ++i) {
    write_uint8(&packet, frame->header.target[i]);
  }
  for (int i = 0; i < 6; ++i) { // Reserved Bytes
    write_uint8(&packet, FRAME_RESERVED);
  }
  uint8_t flags = 0;
  flags |= frame->header.response;
  flags |= frame->header.acknowledgement << 1;
  write_uint8(&packet, flags);
  write_uint8(&packet, frame->header.sequence);

  /* Protocol Header */
  write_uint64(&packet, FRAME_RESERVED); // Reserved Bytes
  write_uint16(&packet, frame->header.type);
  write_uint16(&packet, FRAME_RESERVED); // Reserved Bytes

  /* Payload */
  encode_payload(&packet, frame->header.type, &(frame->payload));

  return packet.cursor;
}

int decode_state_label_payload(lifx_packet_t *packet,
                               lifx_state_label_payload_t *payload) {
  int i;
  for (i = 0; i < 32; ++i) {
    payload->label[i] = read_uint8(packet);
  }
  payload->label[i] = '\0';

  return packet->cursor;
}

int decode_echo_response_payload(lifx_packet_t *packet,
                                 lifx_echo_response_payload_t *payload) {
  int i;
  for (i = 0; i < 64; ++i) {
    payload->echoing[i] = read_uint8(packet);
  }
  payload->echoing[i] = '\0';

  return packet->cursor;
}

int decode_payload(lifx_packet_t *packet, lifx_message_type type,
                   lifx_payload_t *payload) {
  switch (type) {
  case StateLabel:
    return decode_state_label_payload(packet, &payload->state_label_payload);
  case EchoResponse:
    return decode_echo_response_payload(packet,
                                        &payload->echo_response_payload);
  case Acknowledgement:
    return packet->cursor;
  default:
    fprintf(stderr, "[WARN] invalid payload type '%d'\n", type);
    exit(EXIT_FAILURE);
  }
}

int lifx_decode_frame(lifx_frame_t *frame, uint8_t *const *buf, size_t n) {
  if (frame == NULL) {
    return -1;
  }

  if (buf == NULL) {
    return -1;
  }

  lifx_packet_t packet = {
      .buf = buf,
      .capacity = n,
      .cursor = 0,
  };

  /* Frame Header */
  frame->header.size = read_uint16(&packet);
  uint16_t protocol = read_uint16(&packet);
  frame->header.tagged = (protocol & (1 << 13)) > 0;
  frame->header.source = read_uint32(&packet);

  /* Frame Address */
  for (int i = 0; i < 8; ++i) {
    frame->header.target[i] = read_uint8(&packet);
  }
  for (int i = 0; i < 6; ++i) { // Reserved Bytes
    read_uint8(&packet);
  }
  uint8_t flags = read_uint8(&packet);
  frame->header.response = (flags & 1) < 0;
  frame->header.acknowledgement = (flags & (1 << 1)) < 0;
  frame->header.sequence = read_uint8(&packet);

  /* Protocol Header */
  read_uint64(&packet); // Reserved Bytes
  frame->header.type = read_uint16(&packet);
  read_uint16(&packet); // Reserved Bytes

  /* Payload */
  decode_payload(&packet, frame->header.type, &frame->payload);

  return packet.cursor;
}
