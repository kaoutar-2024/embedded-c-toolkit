/**
 * frame.h - CAN-style serial frame encoder / streaming parser with CRC-16.
 *
 * Wire format (all multi-byte fields big-endian):
 *
 *   | 0x7E | id_hi | id_lo | dlc | data[0..dlc-1] | crc_hi | crc_lo |
 *
 *   id  : 11-bit identifier (0x000..0x7FF)
 *   dlc : payload length, 0..8
 *   crc : CRC-16/CCITT-FALSE over id_hi, id_lo, dlc and data
 *
 * There is no byte stuffing, so a 0x7E inside a frame is legal; the parser
 * resynchronises by only looking for the start byte while idle.
 */
#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>
#include <stdint.h>

#define FRAME_SOF 0x7Eu
#define FRAME_MAX_DATA 8u
#define FRAME_MAX_ID 0x7FFu
#define FRAME_MAX_ENCODED (1u + 2u + 1u + FRAME_MAX_DATA + 2u)

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[FRAME_MAX_DATA];
} frame_t;

typedef enum {
    FRAME_PARSE_INCOMPLETE = 0, /* need more bytes */
    FRAME_PARSE_OK,             /* a valid frame was written to *out */
    FRAME_PARSE_CRC_ERROR,      /* frame complete but checksum mismatch */
    FRAME_PARSE_FORMAT_ERROR    /* invalid id or dlc */
} frame_parse_result_t;

typedef enum {
    PS_WAIT_SOF = 0,
    PS_ID_HI,
    PS_ID_LO,
    PS_DLC,
    PS_DATA,
    PS_CRC_HI,
    PS_CRC_LO
} frame_parse_state_t;

typedef struct {
    frame_parse_state_t state;
    frame_t frame;
    uint8_t id_hi;
    uint8_t data_idx;
    uint8_t crc_hi;
} frame_parser_t;

uint16_t crc16_ccitt(const uint8_t *data, size_t len);

/** Encodes into out[]. Returns the encoded length, or 0 on invalid input. */
size_t frame_encode(const frame_t *f, uint8_t *out, size_t out_cap);

void frame_parser_init(frame_parser_t *p);

/** Feed one received byte. *out is only written when OK is returned. */
frame_parse_result_t frame_parser_feed(frame_parser_t *p, uint8_t byte,
                                       frame_t *out);

#endif /* FRAME_H */
