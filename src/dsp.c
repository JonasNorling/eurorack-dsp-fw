#include "dsp.h"
#include "audio.h"
#include "utils.h"
#include "audio_util.h"
#include "analog_in.h"
#include "biquad.h"
#include "gpio.h"
#include "analog_out.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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
        "Knobs: %d%% %d%% %d%% %d%%, CV: %d%% %d%% %d%% %d%%\r\n",
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
        (int)(analog_in_get(5) * 100),
        (int)(analog_in_get(6) * 100),
        (int)(analog_in_get(7) * 100));
    memset(&in_stats, 0, sizeof(in_stats));
    memset(&out_stats, 0, sizeof(out_stats));
}

static struct {
	bq_state lp_filter_state[2];
	quick_filter_state trigger_filter;

	float env_attack_target_value;  // Stop attack phase when reaching this value
	float env_value;  // Current value 0..1 of the envelope
} lpg_state;

/*
 * Lowpass gate implementation.
 *
 * The general behaviour is to have a filter+amplifier combo that can be controlled
 * by a common CV input from an envelope. There is also an internal attack-decay (AD)
 * envelope that can be triggered by a pulse to generate a "ping" kind of sound where
 * the gate (filter+amplifier) opens up quickly and then closes down over time.
 *
 * Parameters:
 *   - envelope_open: 0..1. Amount to open the VCA and VCF.
 * 
 *   - filt_vca_mix: Crossfade between VCA (pure gain) and VCF (pure LP filter)
 *     behaviour. At 0 the output is from a VCA controlled by the envelope input,
 *     at 1 the output is from a VCF controlled by the envelope input. At 0.5 the
 *     VCA and VCF are connected in series.
 *
 * Envelope generator:
 * The envelope is modelled as a value that rises and falls linearly, taking steps
 * that are the inverse of the rise (attack) and fall (decay) times. This was done
 * to make it easy to fit different gain and filter curves to the envelope value and
 * have attack/decay times specified in milliseconds, which would be harder to do
 * with an exponential decay kind of implementation.
 *  
 * FIXME: Dynamic trigger, open different amounts based on shape of pulse
 * FIXME: Some stored energy: make it quicker to open if recently triggered (0.5s or so)
 * FIXME: Boing mode: make the filter open up instead of close down
 * FIXME: Wavefolder and distortion and nicer warmer clipping. Filter Q will push us above clipping.
 * FIXME: Try adding some diffusion or comb (allpass filter) when nearly closed
 */

static void _lpg_main(
	const frame_t * const restrict in,
	frame_t * const restrict out,
	bool trigger,
	float envelope_open,
	float attack_time_ms,
	float decay_time_ms,
	float filt_vca_mix,
	float q_factor
)
{
	if (trigger) {
		lpg_state.env_attack_target_value = 1.0f;
	}
	if (lpg_state.env_attack_target_value != 0.0f) {
		// Attack phase
		const float attack_step = MS_PER_FRAME / attack_time_ms;
		lpg_state.env_value = CLAMP(lpg_state.env_value + attack_step, 0.0f, 1.0f);
		if (lpg_state.env_value >= lpg_state.env_attack_target_value) {
			// End attack phase
			lpg_state.env_attack_target_value = 0.0f;
		}
	}
	else {
		// Decay phase
		const float decay_step = MS_PER_FRAME / decay_time_ms;
		lpg_state.env_value = CLAMP(lpg_state.env_value - decay_step, 0.0f, 1.0f);
	}

    (void)envelope_open;
	//const float env_open_sq = envelope_open * envelope_open;
	const float env_open_sq = lpg_state.env_value * lpg_state.env_value;
	float cutoff = CLAMP(env_open_sq * env_open_sq, 0.001f, 3.1f);
	bq_make_lowpass(&lpg_state.lp_filter_state[0].coeffs, cutoff, q_factor);
	lpg_state.lp_filter_state[1].coeffs = lpg_state.lp_filter_state[0].coeffs;

	frame_t filter_out[FRAMES_PER_BLOCK];
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        filter_out[i].s[0] = bq_process(in[i].s[0], &lpg_state.lp_filter_state[0]);
        filter_out[i].s[1] = bq_process(in[i].s[1], &lpg_state.lp_filter_state[1]);
    }

	// Cross fade between VCA, VCA*VCF, and VCF behaviour
	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		// FIXME: Change gain (and possibly filter) smoothly over whole buffer, this might get too steppy
		const float filter_mix = CLAMP(filt_vca_mix * 2.0f, 0.0f, 1.0f);  // CCW 0 - 1 - 1 CW
		const float gain_mix = CLAMP(filt_vca_mix * 2.0f - 1.0f, 0.0f, 1.0f);  // CCW 0 - 0 - 1 CW
		out[i].s[0] = RAMP(filter_mix, in[i].s[0], filter_out[i].s[0]) * RAMP(gain_mix, env_open_sq, 1.0f);
		out[i].s[1] = RAMP(filter_mix, in[i].s[1], filter_out[i].s[1]) * RAMP(gain_mix, env_open_sq, 1.0f);
	}
}

void dsp_do(const frame_t * const restrict in, frame_t * const restrict out)
{
	const float pot[] = {
		analog_in_get(0),
		analog_in_get(1),
		analog_in_get(2),
		analog_in_get(3),
	};
    // Make CV inputs ±1.0 centered at 0V
	const float cv[] = {
		analog_in_get(4) * 2.0f - 1.0f,
		analog_in_get(5) * 2.0f - 1.0f,
	};

	const float attack_time_ms = RAMP(pot[1] * pot[1] * pot[1], 2.0f, 500.0f);
	const float filt_vca_mix = pot[0];
	const float decay_time_ms = RAMP(pot[2] * pot[2] * pot[2], 10.0f, 5000.0f);
	const float q_factor = RAMP(pot[3], 0.1f, 10.0f);

	const float trigger_pulse = CLAMP(q_highpass(&lpg_state.trigger_filter, 0.05, cv[0]), 0.0f, 1.0f);
	const float envelope_open = CLAMP(cv[1], 0.0f, 1.0f);

    gpio_set_led(3, trigger_pulse > 0.01f);
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        update_statistics(&in_stats, in[i]);
    }

	_lpg_main(
		in,
		out,
		trigger_pulse > 0.3f || !gpio_get(PIN_BUTTON_1),
		envelope_open,
		attack_time_ms,
		decay_time_ms,
		filt_vca_mix,
		q_factor
	);

    gpio_set_led(4, will_clip(out, FRAMES_PER_BLOCK));
    gpio_set_led(5, cv[0] > 0.5f);
    analog_out_set(trigger_pulse * 4095, pot[0] * 4095, lpg_state.env_value * 4095);
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        update_statistics(&out_stats, out[i]);
    }
}
