/**
 * fsm.h - small table-driven finite state machine.
 *
 * States and events are plain integers (use your own enums). A transition
 * table lists (from, event) -> to. Events with no matching row are ignored.
 */
#ifndef FSM_H
#define FSM_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int from;
    int event;
    int to;
} fsm_transition_t;

typedef struct {
    const fsm_transition_t *table;
    size_t table_len;
    int state;
} fsm_t;

void fsm_init(fsm_t *f, const fsm_transition_t *table, size_t table_len,
              int initial_state);

/** Returns true if the event caused a transition. */
bool fsm_dispatch(fsm_t *f, int event);

int fsm_state(const fsm_t *f);

#endif /* FSM_H */
