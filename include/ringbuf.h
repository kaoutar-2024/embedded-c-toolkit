/**
 * ringbuf.h - fixed-capacity byte ring buffer (no dynamic allocation).
 *
 * The caller provides the storage array, so the buffer can live in static
 * memory on a microcontroller. Not interrupt-safe by itself: if one side runs
 * in an ISR, protect push/pop with a critical section.
 */
#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buf;
    size_t cap;
    size_t head;  /* next write index */
    size_t tail;  /* next read index  */
    size_t count; /* bytes currently stored */
} ringbuf_t;

void ringbuf_init(ringbuf_t *rb, uint8_t *storage, size_t capacity);
void ringbuf_clear(ringbuf_t *rb);

/** Returns false (and stores nothing) if the buffer is full. */
bool ringbuf_push(ringbuf_t *rb, uint8_t byte);
/** Returns false if the buffer is empty. */
bool ringbuf_pop(ringbuf_t *rb, uint8_t *byte);

size_t ringbuf_count(const ringbuf_t *rb);
bool ringbuf_is_empty(const ringbuf_t *rb);
bool ringbuf_is_full(const ringbuf_t *rb);

#endif /* RINGBUF_H */
