# embedded-c-toolkit

Small, dependency-free C99 building blocks for microcontroller firmware: a ring buffer, an input debouncer, a PID controller, a table-driven state machine, and a CAN-style serial frame encoder/parser with CRC-16.

Everything is hardware-independent and is built and tested on a PC, so the logic can be verified before it is ported to a target such as an STM32 (ARM Cortex-M), ESP32 or Arduino. No dynamic allocation and no global state anywhere in the library.

## Modules

| Module | Files | What it does |
|---|---|---|
| Ring buffer | `ringbuf.h/.c` | Fixed-capacity byte FIFO on caller-provided storage; ideal for UART RX/TX queues. |
| Debouncer | `debounce.h/.c` | Counter-based debounce for buttons and switches, called at a fixed sample rate. |
| PID controller | `pid.h/.c` | Discrete PID with output clamping, integral anti-windup, and derivative on measurement (no derivative kick). |
| State machine | `fsm.h/.c` | Table-driven FSM: `(from, event) -> to`; unmatched events are ignored. |
| Frame + CRC | `frame.h/.c` | Encoder and byte-by-byte streaming parser for a CAN-style frame; CRC-16/CCITT-FALSE; detects corruption, resynchronises after noise. |

### Frame format

```
| 0x7E | id_hi | id_lo | dlc | data[0..dlc-1] | crc_hi | crc_lo |
```

- `id`: 11-bit identifier (like a standard CAN ID), big-endian
- `dlc`: payload length, 0 to 8 bytes
- `crc`: CRC-16/CCITT-FALSE over `id`, `dlc` and `data`

The parser reports `OK`, `CRC_ERROR`, `FORMAT_ERROR` (invalid id or dlc), or `INCOMPLETE` (need more bytes), so it can be fed directly from a UART interrupt or a ring buffer.

## Build and test

Requirements: a C compiler (gcc or clang) and `make`.

```bash
make test       # build with -Wall -Wextra -Wpedantic -Werror and run the unit tests
make sanitize   # same tests under AddressSanitizer + UBSan
make demo       # PID step-response demo, writes build/pid_step.csv
```

Current status: **58 checks across 16 tests, all passing** with gcc 13 and clang, also clean under ASan/UBSan. CI runs both compilers on every push (`.github/workflows/ci.yml`).

## Usage examples

### Receive frames from a UART byte stream

```c
#include "frame.h"
#include "ringbuf.h"

static uint8_t rx_storage[64];
static ringbuf_t rx_fifo;
static frame_parser_t parser;

void app_init(void) {
    ringbuf_init(&rx_fifo, rx_storage, sizeof rx_storage);
    frame_parser_init(&parser);
}

/* In the UART RX interrupt: ringbuf_push(&rx_fifo, received_byte);
 * (guard the buffer with a critical section if main also pops from it) */

void app_poll(void) {
    uint8_t b;
    frame_t f;
    while (ringbuf_pop(&rx_fifo, &b)) {
        if (frame_parser_feed(&parser, b, &f) == FRAME_PARSE_OK) {
            /* use f.id, f.dlc, f.data[] */
        }
    }
}
```

### Send a frame

```c
frame_t f = { .id = 0x123, .dlc = 2, .data = { 0x01, 0xFF } };
uint8_t wire[FRAME_MAX_ENCODED];
size_t n = frame_encode(&f, wire, sizeof wire);   /* 0 if the frame is invalid */
/* transmit wire[0..n-1] over UART */
```

### Speed control loop

```c
pid_ctrl_t pid;
pid_init(&pid, 2.0f, 4.0f, 0.0f, -10.0f, 10.0f);  /* kp, ki, kd, out_min, out_max */

/* every 10 ms from a timer tick */
float u = pid_update(&pid, target_speed, measured_speed, 0.01f);
```

## PID demo result

`make demo` simulates a first-order plant (time constant 0.5 s) under PI control at 100 Hz (kp = 2, ki = 4) for a unit step:

| Metric | Result |
|---|---|
| Overshoot | 0.0 % |
| 2 % settling time | 0.97 s |

These numbers come from the simulation in `examples/pid_demo.c`; the plant model and gains are in that file if you want to change them.

## Project layout

```
include/   public headers
src/       implementation
tests/     unit tests (dependency-free harness in test_main.c)
examples/  PID step-response demo
```

## Design notes and limitations

- `ringbuf` is **not interrupt-safe** by itself. If a producer runs in an ISR and a consumer in the main loop, wrap the calls in a critical section or replace it with a lock-free single-producer/single-consumer variant.
- The frame format has **no byte stuffing**. A `0x7E` inside a frame is legal; the parser only searches for the start byte while idle, so after an error it may need to skip a few bytes before it resynchronises.
- `pid_update` uses `float`. On Cortex-M cores without an FPU, consider a fixed-point version.
- Tested on a PC only; porting to a target is not included.

## License

MIT, see [LICENSE](LICENSE).
