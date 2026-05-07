#pragma once
#include "audio.h"

void lpg_main(const frame_t * const restrict in, frame_t * const restrict out);
void lfo_main(const frame_t * const restrict in, frame_t * const restrict out);

void dsp_dump_stats(void);
