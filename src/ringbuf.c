#include "ringbuf.h"

void ringbuf_init(ringbuf_t *rb, uint8_t *storage, size_t capacity)
{
    rb->buf = storage;
    rb->cap = capacity;
    ringbuf_clear(rb);
}

void ringbuf_clear(ringbuf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool ringbuf_push(ringbuf_t *rb, uint8_t byte)
{
    if (rb->count == rb->cap) {
        return false;
    }
    rb->buf[rb->head] = byte;
    rb->head = (rb->head + 1) % rb->cap;
    rb->count++;
    return true;
}

bool ringbuf_pop(ringbuf_t *rb, uint8_t *byte)
{
    if (rb->count == 0) {
        return false;
    }
    *byte = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % rb->cap;
    rb->count--;
    return true;
}

size_t ringbuf_count(const ringbuf_t *rb) { return rb->count; }
bool ringbuf_is_empty(const ringbuf_t *rb) { return rb->count == 0; }
bool ringbuf_is_full(const ringbuf_t *rb) { return rb->count == rb->cap; }
