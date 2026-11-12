#include "debounce.h"

void debounce_init(debounce_t *d, uint16_t threshold, bool initial_state)
{
    d->threshold = threshold;
    d->counter = 0;
    d->stable = initial_state;
}

bool debounce_update(debounce_t *d, bool raw)
{
    if (raw == d->stable) {
        d->counter = 0;
    } else {
        d->counter++;
        if (d->counter >= d->threshold) {
            d->stable = raw;
            d->counter = 0;
        }
    }
    return d->stable;
}
