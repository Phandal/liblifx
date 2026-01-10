#ifndef FRAME_H
#define FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* NOTE: this will need to be updated to the actual amount */
#define PAYLOAD_MAX 64
#define FRAME_SIZE_MAX FRAME_HEADER_SIZE + PAYLOAD_MAX

#define FRAME_HEADER_SIZE 36
#define FRAME_PROTOCOL 1024
#define FRAME_ADDRESSABLE 1
#define FRAME_ORIGIN 0
#define FRAME_RESERVED 0

typedef enum {
  UDP = 1,
  RESERVED1,
  RESERVED2,
  RESERVED3,
  RESERVED4,
} lifx_service;

typedef enum {
  GetService = 2,
  StateService = 3,
  SetPower = 21,
  GetLabel = 23,
  StateLabel = 25,
  Acknowledgement = 45,
  EchoRequest = 58,
  EchoResponse = 59,
  SetColor = 102,
} lifx_message_type;

typedef struct {
  uint16_t level;
} lifx_set_power_payload_t;

typedef struct {
  uint16_t hue;
  uint16_t saturation;
  uint16_t brightness;
  uint16_t kelvin;
  uint32_t duration;
} lifx_set_color_payload_t;

typedef struct {
  uint8_t label[33];
} lifx_state_label_payload_t;

typedef struct {
  uint8_t echoing[65];
} lifx_echo_request_payload_t;

typedef struct {
  uint8_t echoing[65];
} lifx_echo_response_payload_t;

typedef struct {
  lifx_service service;
  uint32_t port;
} lifx_state_service_payload_t;

typedef union {
  lifx_state_service_payload_t state_service_payload;
  lifx_set_power_payload_t set_power_payload;
  lifx_state_label_payload_t state_label_payload;
  lifx_echo_request_payload_t echo_request_payload;
  lifx_echo_response_payload_t echo_response_payload;
  lifx_set_color_payload_t set_color_payload;
} lifx_payload_t;

typedef struct {
  /* Frame Header */
  uint16_t size;
  uint8_t tagged : 1;
  uint32_t source;

  /* Frame Address */
  uint8_t target[8];
  uint8_t response : 1;
  uint8_t acknowledgement : 1;
  uint8_t sequence;

  /* Protocol Header */
  lifx_message_type type;
} lifx_header_t;

typedef struct {
  lifx_header_t header;
  lifx_payload_t payload;
} lifx_frame_t;

/**
 * @brief Encode a lifx frame.
 *
 * Encodes a frame into a buffer that can be sent over the wire.
 *
 * @param frame
 * @param buf
 * @param n maximum amount of bytes in buf
 */
int lifx_encode_frame(const lifx_frame_t *frame, uint8_t *const *buf,
                      const size_t n);

/**
 * @brief Decode a lifx buffer.
 *
 *
 * Decodes a buffer into a lifx frame
 *  @param frame
 *  @param buffer
 *  @param n maximum size of buffer
 */
int lifx_decode_frame(lifx_frame_t *frame, uint8_t *const *buf, const size_t n);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_H */
