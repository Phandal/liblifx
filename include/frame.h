#ifndef FRAME_H
#define FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define FRAME_HEADER_SIZE 36
#define FRAME_PROTOCOL 1024
#define FRAME_ADDRESSABLE 1
#define FRAME_ORIGIN 0

typedef enum {
  GetService = 2,
  SetColor = 102,
} lifx_message_type;

typedef struct {
  uint16_t level;
} lifx_set_power_t;

typedef struct {
  uint16_t hue;
  uint16_t saturation;
  uint16_t brightness;
  uint16_t kelvin;
  uint32_t duration;
} lifx_set_color_t;

typedef union {
  lifx_set_power_t set_power_payload;
  lifx_set_color_t set_color_payload;
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

#ifdef __cplusplus
}
#endif

#endif /* FRAME_H */
