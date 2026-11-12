#include "frame.h"

#include <string.h>

uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* CRC over the header + payload bytes of a frame. */
static uint16_t frame_crc(const frame_t *f)
{
    uint8_t buf[3 + FRAME_MAX_DATA];
    buf[0] = (uint8_t)(f->id >> 8);
    buf[1] = (uint8_t)(f->id & 0xFFu);
    buf[2] = f->dlc;
    memcpy(&buf[3], f->data, f->dlc);
    return crc16_ccitt(buf, (size_t)3 + f->dlc);
}

size_t frame_encode(const frame_t *f, uint8_t *out, size_t out_cap)
{
    if (f->id > FRAME_MAX_ID || f->dlc > FRAME_MAX_DATA) {
        return 0;
    }
    const size_t total = (size_t)1 + 2 + 1 + f->dlc + 2;
    if (out_cap < total) {
        return 0;
    }

    size_t n = 0;
    out[n++] = FRAME_SOF;
    out[n++] = (uint8_t)(f->id >> 8);
    out[n++] = (uint8_t)(f->id & 0xFFu);
    out[n++] = f->dlc;
    memcpy(&out[n], f->data, f->dlc);
    n += f->dlc;

    const uint16_t crc = frame_crc(f);
    out[n++] = (uint8_t)(crc >> 8);
    out[n++] = (uint8_t)(crc & 0xFFu);
    return n;
}

void frame_parser_init(frame_parser_t *p)
{
    memset(p, 0, sizeof(*p));
    p->state = PS_WAIT_SOF;
}

frame_parse_result_t frame_parser_feed(frame_parser_t *p, uint8_t byte,
                                       frame_t *out)
{
    switch (p->state) {
    case PS_WAIT_SOF:
        if (byte == FRAME_SOF) {
            p->state = PS_ID_HI;
        }
        return FRAME_PARSE_INCOMPLETE;

    case PS_ID_HI:
        p->id_hi = byte;
        p->state = PS_ID_LO;
        return FRAME_PARSE_INCOMPLETE;

    case PS_ID_LO:
        p->frame.id = (uint16_t)(((uint16_t)p->id_hi << 8) | byte);
        if (p->frame.id > FRAME_MAX_ID) {
            p->state = PS_WAIT_SOF;
            return FRAME_PARSE_FORMAT_ERROR;
        }
        p->state = PS_DLC;
        return FRAME_PARSE_INCOMPLETE;

    case PS_DLC:
        if (byte > FRAME_MAX_DATA) {
            p->state = PS_WAIT_SOF;
            return FRAME_PARSE_FORMAT_ERROR;
        }
        p->frame.dlc = byte;
        p->data_idx = 0;
        p->state = (byte == 0) ? PS_CRC_HI : PS_DATA;
        return FRAME_PARSE_INCOMPLETE;

    case PS_DATA:
        p->frame.data[p->data_idx++] = byte;
        if (p->data_idx >= p->frame.dlc) {
            p->state = PS_CRC_HI;
        }
        return FRAME_PARSE_INCOMPLETE;

    case PS_CRC_HI:
        p->crc_hi = byte;
        p->state = PS_CRC_LO;
        return FRAME_PARSE_INCOMPLETE;

    case PS_CRC_LO: {
        const uint16_t rx = (uint16_t)(((uint16_t)p->crc_hi << 8) | byte);
        p->state = PS_WAIT_SOF;
        if (rx != frame_crc(&p->frame)) {
            return FRAME_PARSE_CRC_ERROR;
        }
        *out = p->frame;
        return FRAME_PARSE_OK;
    }
    }
    p->state = PS_WAIT_SOF;
    return FRAME_PARSE_FORMAT_ERROR;
}
