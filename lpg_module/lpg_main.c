#include "dsp.h"
#include "audio.h"
#include "utils.h"
#include "dsp/audio_util.h"
#include "analog_in.h"
#include "dsp/biquad.h"
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

typedef struct {
	bq_state lp_filter_state;
	quick_filter_state trigger_hp_filter;
	slope_state trigger_slope_limit;

	float env_value;  // Current value 0..1 of the envelope
} lpg_state_t;
static lpg_state_t lpg_state[2];

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
	lpg_state_t *state,
	const float * const restrict in,
	float * const restrict out,
	float trigger_pulse,
	float envelope_open,
	float decay_time_ms,
	float filt_vca_mix,
	float q_factor
)
{
	// Attack
	state->env_value = state->env_value + trigger_pulse * 0.025f * FRAMES_PER_BLOCK;

	// Decay
	float decay_step = MS_PER_FRAME / decay_time_ms;
	if (state->env_value > 0.5) {
		// Initial fast decay
		decay_step *= 2;
	}
	else if (state->env_value < 0.1) {
		// Final long tail
		decay_step *= 0.5f;
	}
	state->env_value = CLAMP(state->env_value - decay_step, 0.0f, 1.0f);

	const float env_open_sq = envelope_open * envelope_open + state->env_value * state->env_value;
	float cutoff = RAMP(env_open_sq * env_open_sq, HZ2OMEGA(5), HZ2OMEGA(20000));
	cutoff = CLAMP(cutoff, 0.0f, M_PIf);
	q_factor *= sqrtf(cutoff);  // Rein in resonance at low frequency, it got unpleasantly boomy
	bq_make_lowpass(&state->lp_filter_state.coeffs, cutoff, q_factor);

	float filter_out[FRAMES_PER_BLOCK];
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		float s = in[i];
		if (cutoff < HZ2OMEGA(40)) {
			s *= cutoff/HZ2OMEGA(40);
		}
        filter_out[i] = bq_process(s, &state->lp_filter_state);
    }

	// Cross fade between VCA, VCA*VCF, and VCF behaviour
	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		// FIXME: Change gain (and possibly filter) smoothly over whole buffer, this might get too steppy
		const float filter_mix = CLAMP(filt_vca_mix * 2.0f, 0.0f, 1.0f);  // CCW 0 - 1 - 1 CW
		const float gain_mix = CLAMP(filt_vca_mix * 2.0f - 1.0f, 0.0f, 1.0f);  // CCW 0 - 0 - 1 CW
		out[i] = RAMP(filter_mix, in[i], filter_out[i]) * RAMP(gain_mix, env_open_sq, 1.0f);
	}
}

void lpg_main(const frame_t * const restrict in, frame_t * const restrict out)
{
	const float pot[] = {
		analog_in_get(0),  // VCA-VCF balance
		analog_in_get(1),  // Left channel decay time
		analog_in_get(2),  // Right channel decay time
		analog_in_get(3),  // Filter Q factor
	};
    // Make CV inputs ±1.0 centered at 0V
	const float cv[] = {
		analog_in_get(4) * 2.0f - 1.0f,  // Left channel open
		analog_in_get(5) * 2.0f - 1.0f,  // Left channel trigger
		analog_in_get(6) * 2.0f - 1.0f,  // Right channel open
		analog_in_get(7) * 2.0f - 1.0f,  // Right channel trigger
	};

	const float filt_vca_mix = pot[0];
	const float decay_time_ms[2] = {
		RAMP(pot[1] * pot[1] * pot[1], 10.0f, 5000.0f),
		RAMP(pot[2] * pot[2] * pot[2], 10.0f, 5000.0f)
	};
	const float q_factor = RAMP(pot[3], 0.1f, 10.0f);
	const float envelope_open[2] = {cv[0], cv[2]};
	const float trigger_cv[2] = {cv[1], cv[3]};

	const bool trigger_button = !gpio_get(PIN_BUTTON_1);
	const float trig_slope = MS_PER_FRAME / 3.0f;  // Minimum 3 ms ramp on trigger

	float trigger_input[2];
	float trigger_pulse[2];

	for (int ch = 0; ch < 2; ch++) {
		trigger_input[ch] = slope_limit(
			&lpg_state[ch].trigger_slope_limit,
			trig_slope,
			trigger_cv[ch] + trigger_button);
		trigger_pulse[ch] = CLAMP(
			2.0f * q_highpass(&lpg_state[ch].trigger_hp_filter, 0.5f * MS_PER_FRAME, trigger_input[ch]),
			0.0f, 1.0f);

		float in_buf[FRAMES_PER_BLOCK];
		float out_buf[FRAMES_PER_BLOCK];
		for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
			in_buf[i] = in[i].s[ch];
		}

		_lpg_main(
			&lpg_state[ch],
			in_buf,
			out_buf,
			trigger_pulse[ch],
			envelope_open[ch],
			decay_time_ms[ch],
			filt_vca_mix,
			q_factor
		);

		for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
			out[i].s[ch] = saturate_soft(out_buf[i]);
		}
	}

    gpio_set_led(0, trigger_pulse[0] > 0.01f);
	gpio_set_led(1, trigger_input[0] > 0.5f);
    gpio_set_led(3, trigger_pulse[1] > 0.01f);
    gpio_set_led(4, trigger_input[1] > 0.5f);
    analog_out_set(trigger_pulse[0] * 4095, 0, trigger_pulse[1] * 4095);

	for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
		update_statistics(&in_stats, in[i]);
        update_statistics(&out_stats, out[i]);
    }
}
