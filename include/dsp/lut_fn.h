#include "lut_data.h"
#include "audio_util.h"
#include "utils.h"
#include "math.h"
#include "assert.h"

static inline float lut_sin(float v)
{
    float pos = ((ARRAY_SIZE(lut_sin_table) - 1) / TWOPI) * v;
    // The interpolation function will take wrap the position to
    // keep in in the table.
    return linterpolate(lut_sin_table, ARRAY_SIZE(lut_sin_table), pos);
}

static inline float lut_cos(float v)
{
    return lut_sin(v + M_PI_2f);
}