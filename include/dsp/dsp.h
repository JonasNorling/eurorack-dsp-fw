#pragma once
#include "audio.h"

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out);
void dsp_dump_stats(void);
