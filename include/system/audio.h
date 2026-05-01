#pragma once
#include <stdint.h>

#define FRAMES_PER_BLOCK 2
#define SAMPLE_RATE 48000
#define MS_PER_FRAME (1000.0f * FRAMES_PER_BLOCK / SAMPLE_RATE)
#define SAMPLE_MAX ((1 << 23) - 1)
#define SAMPLE_MIN (-(1 << 23))

typedef struct {
    float s[2];  // left & right channel
} frame_t;
