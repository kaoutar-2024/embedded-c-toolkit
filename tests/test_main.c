/* Minimal dependency-free unit tests. Build and run with `make test`. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "debounce.h"
#include "frame.h"
#include "fsm.h"
#include "pid.h"
#include "ringbuf.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        g_checks++;                                                         \
        if (!(cond)) {                                                      \
            g_failures++;                                                   \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                   \
    } while (0)

#define RUN(test)                                                           \
    do {                                                                    \
        int before = g_failures;                                            \
        test();                                                             \
        printf("[%s] %s\n", g_failures == before ? " OK " : "FAIL", #test); \
    } while (0)

/* ---------------------------------------------------------------- ringbuf */

static void test_ringbuf_fifo_order(void)
{
    uint8_t storage[4];
    ringbuf_t rb;
    ringbuf_init(&rb, storage, sizeof storage);

    CHECK(ringbuf_is_empty(&rb));
    CHECK(ringbuf_push(&rb, 1));
    CHECK(ringbuf_push(&rb, 2));
    CHECK(ringbuf_push(&rb, 3));
    CHECK(ringbuf_count(&rb) == 3);

    uint8_t v = 0;
    CHECK(ringbuf_pop(&rb, &v) && v == 1);
    CHECK(ringbuf_pop(&rb, &v) && v == 2);
    CHECK(ringbuf_pop(&rb, &v) && v == 3);
    CHECK(!ringbuf_pop(&rb, &v));
}

static void test_ringbuf_full_and_wraparound(void)
{
    uint8_t storage[3];
    ringbuf_t rb;
    ringbuf_init(&rb, storage, sizeof storage);

    CHECK(ringbuf_push(&rb, 10));
    CHECK(ringbuf_push(&rb, 20));
    CHECK(ringbuf_push(&rb, 30));
    CHECK(ringbuf_is_full(&rb));
    CHECK(!ringbuf_push(&rb, 40)); /* rejected, nothing overwritten */

    uint8_t v = 0;
    CHECK(ringbuf_pop(&rb, &v) && v == 10);
    CHECK(ringbuf_push(&rb, 50)); /* wraps around the end of storage */
    CHECK(ringbuf_pop(&rb, &v) && v == 20);
    CHECK(ringbuf_pop(&rb, &v) && v == 30);
    CHECK(ringbuf_pop(&rb, &v) && v == 50);
    CHECK(ringbuf_is_empty(&rb));
}

/* --------------------------------------------------------------- debounce */

static void test_debounce_ignores_short_glitch(void)
{
    debounce_t d;
    debounce_init(&d, 3, false);

    CHECK(debounce_update(&d, true) == false);  /* 1 sample  */
    CHECK(debounce_update(&d, true) == false);  /* 2 samples */
    CHECK(debounce_update(&d, false) == false); /* glitch ends, counter reset */
    CHECK(debounce_update(&d, true) == false);
    CHECK(debounce_update(&d, true) == false);
    CHECK(debounce_update(&d, true) == true);   /* 3 in a row: accepted */
}

static void test_debounce_release(void)
{
    debounce_t d;
    debounce_init(&d, 2, true);
    CHECK(debounce_update(&d, false) == true);
    CHECK(debounce_update(&d, false) == false);
    CHECK(debounce_update(&d, false) == false);
}

/* -------------------------------------------------------------------- pid */

static void test_pid_proportional_only(void)
{
    pid_ctrl_t p;
    pid_init(&p, 2.0f, 0.0f, 0.0f, -100.0f, 100.0f);
    CHECK(fabsf(pid_update(&p, 10.0f, 4.0f, 0.01f) - 12.0f) < 1e-5f);
}

static void test_pid_output_is_clamped(void)
{
    pid_ctrl_t p;
    pid_init(&p, 100.0f, 0.0f, 0.0f, -1.0f, 1.0f);
    CHECK(pid_update(&p, 10.0f, 0.0f, 0.01f) == 1.0f);
    CHECK(pid_update(&p, -10.0f, 0.0f, 0.01f) == -1.0f);
}

static void test_pid_integral_anti_windup(void)
{
    pid_ctrl_t p;
    pid_init(&p, 0.0f, 10.0f, 0.0f, -1.0f, 1.0f);
    for (int i = 0; i < 1000; i++) {
        pid_update(&p, 100.0f, 0.0f, 0.01f); /* huge sustained error */
    }
    CHECK(p.integral <= 1.0f);
    /* After the error flips sign it must recover quickly, not unwind a
     * huge accumulated integral. */
    float out = 0.0f;
    for (int i = 0; i < 30; i++) {
        out = pid_update(&p, -100.0f, 0.0f, 0.01f);
    }
    CHECK(out < 0.0f);
}

static void test_pid_converges_on_first_order_plant(void)
{
    /* plant: x' = (-x + u) / tau, integrated with forward Euler */
    pid_ctrl_t p;
    pid_init(&p, 2.0f, 4.0f, 0.0f, -10.0f, 10.0f);
    const float tau = 0.5f, dt = 0.001f, target = 1.0f;
    float x = 0.0f;
    for (int i = 0; i < 10000; i++) { /* 10 simulated seconds */
        float u = pid_update(&p, target, x, dt);
        x += dt * (-x + u) / tau;
    }
    CHECK(fabsf(x - target) < 0.01f);
}

/* -------------------------------------------------------------------- fsm */

enum { ST_IDLE, ST_RUNNING, ST_FAULT };
enum { EV_START, EV_STOP, EV_ERROR, EV_RESET };

static void test_fsm_transitions(void)
{
    static const fsm_transition_t table[] = {
        {ST_IDLE, EV_START, ST_RUNNING},
        {ST_RUNNING, EV_STOP, ST_IDLE},
        {ST_RUNNING, EV_ERROR, ST_FAULT},
        {ST_FAULT, EV_RESET, ST_IDLE},
    };
    fsm_t f;
    fsm_init(&f, table, sizeof table / sizeof table[0], ST_IDLE);

    CHECK(!fsm_dispatch(&f, EV_STOP));  /* not valid in IDLE: ignored */
    CHECK(fsm_state(&f) == ST_IDLE);
    CHECK(fsm_dispatch(&f, EV_START) && fsm_state(&f) == ST_RUNNING);
    CHECK(fsm_dispatch(&f, EV_ERROR) && fsm_state(&f) == ST_FAULT);
    CHECK(!fsm_dispatch(&f, EV_START)); /* must reset first */
    CHECK(fsm_dispatch(&f, EV_RESET) && fsm_state(&f) == ST_IDLE);
}

/* ------------------------------------------------------------- frame / crc */

static void test_crc16_known_vector(void)
{
    /* Standard check value for CRC-16/CCITT-FALSE. */
    const uint8_t msg[] = "123456789";
    CHECK(crc16_ccitt(msg, 9) == 0x29B1);
}

static frame_parse_result_t feed_all(frame_parser_t *p, const uint8_t *bytes,
                                     size_t n, frame_t *out)
{
    frame_parse_result_t r = FRAME_PARSE_INCOMPLETE;
    for (size_t i = 0; i < n; i++) {
        r = frame_parser_feed(p, bytes[i], out);
    }
    return r;
}

static void test_frame_roundtrip(void)
{
    frame_t tx = {.id = 0x123, .dlc = 4, .data = {0xDE, 0xAD, 0xBE, 0xEF}};
    uint8_t wire[FRAME_MAX_ENCODED];
    size_t n = frame_encode(&tx, wire, sizeof wire);
    CHECK(n == 1 + 2 + 1 + 4 + 2);

    frame_parser_t p;
    frame_parser_init(&p);
    frame_t rx;
    memset(&rx, 0, sizeof rx);
    CHECK(feed_all(&p, wire, n, &rx) == FRAME_PARSE_OK);
    CHECK(rx.id == 0x123 && rx.dlc == 4);
    CHECK(memcmp(rx.data, tx.data, 4) == 0);
}

static void test_frame_zero_length_payload(void)
{
    frame_t tx = {.id = 0x7FF, .dlc = 0};
    uint8_t wire[FRAME_MAX_ENCODED];
    size_t n = frame_encode(&tx, wire, sizeof wire);
    CHECK(n == 6);

    frame_parser_t p;
    frame_parser_init(&p);
    frame_t rx;
    CHECK(feed_all(&p, wire, n, &rx) == FRAME_PARSE_OK);
    CHECK(rx.id == 0x7FF && rx.dlc == 0);
}

static void test_frame_detects_corruption(void)
{
    frame_t tx = {.id = 0x100, .dlc = 2, .data = {0x11, 0x22}};
    uint8_t wire[FRAME_MAX_ENCODED];
    size_t n = frame_encode(&tx, wire, sizeof wire);

    wire[4] ^= 0x01; /* flip one payload bit */

    frame_parser_t p;
    frame_parser_init(&p);
    frame_t rx;
    CHECK(feed_all(&p, wire, n, &rx) == FRAME_PARSE_CRC_ERROR);
}

static void test_frame_resync_after_garbage(void)
{
    frame_t tx = {.id = 0x055, .dlc = 1, .data = {0x42}};
    uint8_t wire[FRAME_MAX_ENCODED + 3];
    wire[0] = 0x00; /* line noise before the real frame */
    wire[1] = 0xFF;
    wire[2] = 0x13;
    size_t n = frame_encode(&tx, &wire[3], FRAME_MAX_ENCODED);

    frame_parser_t p;
    frame_parser_init(&p);
    frame_t rx;
    CHECK(feed_all(&p, wire, n + 3, &rx) == FRAME_PARSE_OK);
    CHECK(rx.id == 0x055 && rx.data[0] == 0x42);
}

static void test_frame_rejects_bad_header(void)
{
    frame_parser_t p;
    frame_t rx;

    /* dlc = 9 is invalid */
    const uint8_t bad_dlc[] = {FRAME_SOF, 0x00, 0x10, 9};
    frame_parser_init(&p);
    CHECK(feed_all(&p, bad_dlc, sizeof bad_dlc, &rx) == FRAME_PARSE_FORMAT_ERROR);

    /* id = 0x0800 does not fit in 11 bits */
    const uint8_t bad_id[] = {FRAME_SOF, 0x08, 0x00};
    frame_parser_init(&p);
    CHECK(feed_all(&p, bad_id, sizeof bad_id, &rx) == FRAME_PARSE_FORMAT_ERROR);

    /* the encoder refuses invalid frames too */
    frame_t f = {.id = 0x800, .dlc = 0};
    uint8_t out[FRAME_MAX_ENCODED];
    CHECK(frame_encode(&f, out, sizeof out) == 0);
}

static void test_frame_back_to_back(void)
{
    frame_t a = {.id = 1, .dlc = 1, .data = {0xAA}};
    frame_t b = {.id = 2, .dlc = 2, .data = {0xBB, 0xCC}};
    uint8_t wire[2 * FRAME_MAX_ENCODED];
    size_t n = frame_encode(&a, wire, sizeof wire);
    n += frame_encode(&b, wire + n, sizeof wire - n);

    frame_parser_t p;
    frame_parser_init(&p);
    frame_t rx;
    int ok = 0;
    for (size_t i = 0; i < n; i++) {
        if (frame_parser_feed(&p, wire[i], &rx) == FRAME_PARSE_OK) {
            ok++;
            CHECK(ok == 1 ? rx.id == 1 : rx.id == 2);
        }
    }
    CHECK(ok == 2);
}

int main(void)
{
    RUN(test_ringbuf_fifo_order);
    RUN(test_ringbuf_full_and_wraparound);
    RUN(test_debounce_ignores_short_glitch);
    RUN(test_debounce_release);
    RUN(test_pid_proportional_only);
    RUN(test_pid_output_is_clamped);
    RUN(test_pid_integral_anti_windup);
    RUN(test_pid_converges_on_first_order_plant);
    RUN(test_fsm_transitions);
    RUN(test_crc16_known_vector);
    RUN(test_frame_roundtrip);
    RUN(test_frame_zero_length_payload);
    RUN(test_frame_detects_corruption);
    RUN(test_frame_resync_after_garbage);
    RUN(test_frame_rejects_bad_header);
    RUN(test_frame_back_to_back);

    printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
