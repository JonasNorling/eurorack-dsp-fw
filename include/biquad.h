#pragma once
#include <stddef.h>

/**
 * Biquad coefficients for
 *
 *          b0 + b1*z^-1 + b2*z^-2
 *  H(z) = ------------------------
 *          a0 + a1*z^-1 + a2*z^-2
 *
 *  Where a0 has been normalized to 1.
 */
typedef struct {
    float a1, a2; // poles
    float b0, b1, b2; // zeros
} bq_coeffs;

/**
 * State for a biquad stage.
 */
typedef struct {
    bq_coeffs coeffs;
    float X[2];
    float Y[2];
} bq_state;

/**
 * Create a second-order resonant lowpass filter
 *
 * @param w0 Cutoff/resonance frequency in radians
 * @param q Resonance
 */
void bq_make_lowpass(bq_coeffs* c, float w0, float q);

/**
 * Create a second-order bandpass filter
 *
 * @param w0 Centre frequency in radians
 * @param q Bandwidth/resonance figure
 */
void bq_make_bandpass(bq_coeffs* c, float w0, float q);

/**
 * Run a biquad filter on a single sample
 */
static inline float bq_process(float in, bq_state* state)
{
    bq_coeffs *c = &state->coeffs;
    const float y = c->b0*in +
        c->b1*state->X[0] + c->b2*state->X[1] -
        c->a1*state->Y[0] - c->a2*state->Y[1];
    state->X[1] = state->X[0];
    state->X[0] = in;
    state->Y[1] = state->Y[0];
    state->Y[0] = y;
    return y;
}
