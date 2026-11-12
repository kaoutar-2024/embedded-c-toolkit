#include "fsm.h"

void fsm_init(fsm_t *f, const fsm_transition_t *table, size_t table_len,
              int initial_state)
{
    f->table = table;
    f->table_len = table_len;
    f->state = initial_state;
}

bool fsm_dispatch(fsm_t *f, int event)
{
    for (size_t i = 0; i < f->table_len; i++) {
        if (f->table[i].from == f->state && f->table[i].event == event) {
            f->state = f->table[i].to;
            return true;
        }
    }
    return false;
}

int fsm_state(const fsm_t *f) { return f->state; }
