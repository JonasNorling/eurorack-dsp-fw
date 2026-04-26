#include "dsp.h"
#include "audio.h"

#include <string.h>  // memcpy

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
    memcpy(out, in, sizeof(frame_t) * FRAMES_PER_BLOCK);
}
