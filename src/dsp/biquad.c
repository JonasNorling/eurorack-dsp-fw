#include "dsp/biquad.h"
#include "dsp/lut_fn.h"
#include <math.h>

/*
 * Some ideas from:
 * Cookbook formulae for audio EQ biquad filter coefficients
 * by Robert Bristow-Johnson  <rbj@audioimagination.com>
 * http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
 */
void bq_make_lowpass(bq_coeffs* c, float w0, float q)
{
    /* The cookbook says:
            b0 =  (1 - cos(w0))/2
            b1 =   1 - cos(w0)
            b2 =  (1 - cos(w0))/2
            a0 =   1 + alpha
            a1 =  -2*cos(w0)
            a2 =   1 - alpha
     */

    const float alpha = lut_sin(w0)/(2.0f*q);
    const float a0 = 1.0f + alpha;
    const float a0inv = 1.0f / a0;
    const float cosw0 = lut_cos(w0);
    c->a1 = -2.0f * cosw0 * a0inv;
    c->a2 = (1.0f - alpha) * a0inv;
    c->b1 = (1.0f - cosw0) * a0inv;
    c->b0 = c->b1 * 0.5f;
    c->b2 = c->b0;
}

void bq_make_bandpass(bq_coeffs* c, float w0, float q)
{
    /* The cookbook says:
            b0 =   sin(w0)/2  =   Q*alpha
            b1 =   0
            b2 =  -sin(w0)/2  =  -Q*alpha
            a0 =   1 + alpha
            a1 =  -2*cos(w0)
            a2 =   1 - alpha
     */

    const float alpha = sinf(w0)/(2.0f*q);
    const float a0 = 1.0f + alpha;
    const float a0inv = 1.0f / a0;
    const float cosw0 = cosf(w0);
    c->a1 = -2.0f * cosw0 * a0inv;
    c->a2 = (1.0f - alpha) * a0inv;
    c->b0 = q * alpha * a0inv;
    c->b1 = 0;
    c->b2 = -c->b0;
}
