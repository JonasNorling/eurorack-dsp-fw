#pragma once
#include <math.h>
#include <stdbool.h>
#include "audio.h"

#define CLAMP(v, min, max) (v) > (max) ? (max) : ((v) < (min) ? (min) : (v))
#define NYQUIST (SAMPLE_RATE / 2)
#define RAMP(v, start, end) ((end) * (v) + (start * (1.0f-(v))))

/**
 * Convert a frequency to radians for making filters
 */
#define HZ2OMEGA(f) ((f) * (3.1415926f / NYQUIST))

static inline int32_t float_to_codec(float v)
{
    return CLAMP(v, SAMPLE_MIN, SAMPLE_MAX);
}

static inline float saturate_tube(float _x)
{
    // Clippy, tube-like distortion
    // References : Posted by Partice Tarrabia and Bram de Jong
    const float depth = 0.5;
    const float k = 2.0f * depth / (1.0f - depth);
    const float b = SAMPLE_MAX;
    const float x = _x / b;
    const float ret = b * (1 + k) * x / (1 + k * fabsf(x));
    if (ret < SAMPLE_MIN) return SAMPLE_MIN;
    if (ret > SAMPLE_MAX) return SAMPLE_MAX;
    return ret;
}

// Return true if one of the samples is beyond int16 range
static inline bool will_clip(const frame_t *v, size_t count)
{
    for (size_t s = 0; s < count; s++) {
        if (v[s].s[0] <= SAMPLE_MIN || v[s].s[0] >= SAMPLE_MAX ||
            v[s].s[1] <= SAMPLE_MIN || v[s].s[1] >= SAMPLE_MAX) {
            return true;
        }
    }
    return false;
}

typedef struct {
    float v;
} quick_filter_state;

static inline float q_lowpass(quick_filter_state *state, float factor, float value)
{
    state->v = factor * value + (1.0f - factor) * state->v;
    return state->v;
}

static inline float q_highpass(quick_filter_state *state, float factor, float value)
{
    state->v = factor * value + (1.0f - factor) * state->v;
    return value - state->v;
}

/**
 * Pick a non-integer position from a lookup table or delay line
 */
static inline float linterpolate(const float* table, size_t tablelen, float pos)
{
    const unsigned intpos = (unsigned)pos;
    const float s0 = table[intpos % tablelen];
    const float s1 = table[(intpos + 1) % tablelen];
    const float frac = pos - intpos;
    return s0 * (1.0f - frac) + s1 * frac;
}
