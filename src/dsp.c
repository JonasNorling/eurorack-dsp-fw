#include "dsp.h"
#include "audio.h"
#include "utils.h"
#include "audio_util.h"
#include "analog_in.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    float min[2];
    float max[2];
} statistics_t;

static statistics_t in_stats, out_stats;

static void update_statistics(statistics_t *s, frame_t v)
{
    s->min[0] = MIN(s->min[0], v.s[0]);
    s->max[0] = MAX(s->max[0], v.s[0]);
    s->min[1] = MIN(s->min[1], v.s[1]);
    s->max[1] = MAX(s->max[1], v.s[1]);
}

void dsp_dump_stats(void)
{
    printf("In range: %d%%..%d%% %d%%..%d%%  Out range: %d%%..%d%% %d%%..%d%%. "
        "Knobs: %d%% %d%% %d%%, CV: %d%% %d%% %d%%\r\n",
        100 * (int)in_stats.min[0] / SAMPLE_MIN,
        100 * (int)in_stats.max[0] / SAMPLE_MAX,
        100 * (int)in_stats.min[1] / SAMPLE_MIN,
        100 * (int)in_stats.max[1] / SAMPLE_MAX,
        100 * (int)out_stats.min[0] / SAMPLE_MIN,
        100 * (int)out_stats.max[0] / SAMPLE_MAX,
        100 * (int)out_stats.min[1] / SAMPLE_MIN,
        100 * (int)out_stats.max[1] / SAMPLE_MAX,
        (int)(analog_in_get(0) * 100),
        (int)(analog_in_get(1) * 100),
        (int)(analog_in_get(2) * 100),
        (int)(analog_in_get(3) * 100),
        (int)(analog_in_get(4) * 100),
        (int)(analog_in_get(5) * 100));
    memset(&in_stats, 0, sizeof(in_stats));
    memset(&out_stats, 0, sizeof(out_stats));
}

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
    const float knobs[] = {
        analog_in_get(0),
    };

    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        update_statistics(&in_stats, in[i]);
        out[i].s[0] = 3.0f * knobs[0]*knobs[0] * saturate_tube(in[i].s[0]);
        out[i].s[1] = 3.0f * knobs[0]*knobs[0] * saturate_tube(in[i].s[1]);
        update_statistics(&out_stats, out[i]);
    }
}
