#include "lut_data.h"
#include "audio_util.h"
#include "utils.h"
#include "math.h"
#include "assert.h"

# define M_PIf		3.14159265358979323846f	/* pi */
# define M_PI_2f	1.57079632679489661923f	/* pi/2 */
#define TWOPI (2.0f * M_PIf)

static inline float lut_sin(float v)
{
    // Assuming we won't need to go beyond 2pi
    assert(v >= 0 && v <= TWOPI);
    // THe LUT includes an extra position for v=2pi
    float pos = ((ARRAY_SIZE(lut_sin_table) - 1) / TWOPI) * v;
    return linterpolate(lut_sin_table, ARRAY_SIZE(lut_sin_table), pos);
}

static inline float lut_cos(float v)
{
    v += M_PI_2f;
    if (v >= TWOPI) {
        v -= TWOPI;
    }
    return lut_sin(v);
}