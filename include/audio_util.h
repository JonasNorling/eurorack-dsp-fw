#pragma once
#include <math.h>
#include "audio.h"

#define CLAMP(v, min, max) (v) > (max) ? (max) : ((v) < (min) ? (min) : (v))

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
