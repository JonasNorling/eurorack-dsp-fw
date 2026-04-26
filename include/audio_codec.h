#pragma once
#include "audio.h"

int audio_codec_init(void);
int audio_run(void(*dsp_fn)(const frame_t * const in, frame_t *const out));
