#pragma once
#include <stdint.h>

typedef int16_t sample_t;
#define FRAMES_PER_BLOCK 2
#define SAMPLE_RATE 48000

typedef struct {
    sample_t s[2];  // left & right channel
} frame_t;
