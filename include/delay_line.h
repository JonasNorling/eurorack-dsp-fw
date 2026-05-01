#pragma once
#include <unistd.h>

typedef struct {
    int len;
    float *buffer;
    int writepos;
} delay_line_state;

static inline void delay_line_put(delay_line_state *state, float sample)
{
    state->buffer[state->writepos] = sample;
    state->writepos = (state->writepos + 1) % state->len;
}

/*
 * Get sample at a position relative to the write pointer.
 * pos_relative_write=0 - return last written sample
 * pos_relative_write=-(len-1) - return oldest sample
 * pos_relative_write=1 - return oldest sample
 */
static inline float delay_line_get(delay_line_state *state, int pos_relative_write)
{
    int pos = (unsigned)(state->writepos + pos_relative_write) % state->len;
    return state->buffer[pos];
}
