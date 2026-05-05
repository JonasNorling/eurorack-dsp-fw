#pragma once
#include <math.h>
#include <stdbool.h>
#include "audio.h"

#define M_PIf 3.14159265358979323846f	/* pi */
#define M_PI_2f 1.57079632679489661923f	/* pi/2 */
#define TWOPI (2.0f * M_PIf)


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

static inline float saturate_soft(float x)
{
    // This levels off just before 1.0
    if (x >= SAMPLE_MAX) {
        return SAMPLE_MAX;
    }
    if (x <= SAMPLE_MIN) {
        return SAMPLE_MIN;
    }

    x *= (1.0f/SAMPLE_MAX);
    return SAMPLE_MAX * (1.5f * x - 0.5f * x * x * x);
}

static inline float tube_distortion(float _x, float depth)
{
    // Clippy, tube-like distortion
    // References : Posted by Partice Tarrabia and Bram de Jong
    // This has the effect of squaring off the input waveform.
    // Keep depth=0..1 or suffer.
    //const float depth = 0.5f;
    const float k = 2.0f * depth / (1.0f - depth);
    const float b = SAMPLE_MAX;
    const float x = _x / b;
    const float ret = b * (1.0f + k) * x / (1.0f + k * fabsf(x));
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
