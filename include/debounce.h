/**
 * debounce.h - counter-based input debouncer.
 *
 * Call debounce_update() at a fixed rate (e.g. every 1 ms from a timer tick).
 * The output only changes after the raw input has differed from the current
 * stable value for `threshold` consecutive samples.
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t threshold; /* consecutive samples needed to accept a change */
    uint16_t counter;
    bool stable;
} debounce_t;

void debounce_init(debounce_t *d, uint16_t threshold, bool initial_state);

/** Feed one raw sample; returns the debounced state. */
bool debounce_update(debounce_t *d, bool raw);

#endif /* DEBOUNCE_H */
