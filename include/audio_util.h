#pragma once
#include "audio.h"

#define CLAMP(v, min, max) (v) > (max) ? (max) : ((v) < (min) ? (min) : (v))

static inline int32_t float_to_codec(float v)
{
    return CLAMP(v, SAMPLE_MIN, SAMPLE_MAX);
}
