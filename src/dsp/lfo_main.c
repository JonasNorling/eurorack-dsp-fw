#include "dsp/dsp.h"
#include "audio.h"
#include "analog_in.h"
#include "gpio.h"
#include "analog_out.h"
#include "dsp/lut_fn.h"

typedef struct {
    float phase;
    float out_value;
} channel_state_t;

static channel_state_t s_channel_state[3];

void lfo_main(const frame_t * const restrict in, frame_t * const restrict out)
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
		analog_in_get(6) * 2.0f - 1.0f,
		analog_in_get(7) * 2.0f - 1.0f,
	};

    for (int c = 0; c < 3; c++) {
        const float f = pot[c]*pot[c]*pot[c] + cv[c]*cv[c];
        s_channel_state[c].out_value = lut_sin(s_channel_state[c].phase);
        s_channel_state[c].phase += MS_PER_FRAME * f;  // f=1 -> 159 Hz
        if (s_channel_state[c].phase > 2*M_PIf) {
            s_channel_state[c].phase = 0.0f;
        }
    }

    gpio_set_led(3, pot[0] > 0.5);
    gpio_set_led(4, pot[1] > 0.5);
    gpio_set_led(5, pot[2] > 0.5);

    analog_out_set(
        s_channel_state[0].out_value * 0x7ff + 0x7ff,
        s_channel_state[1].out_value * 0x7ff + 0x7ff,
        s_channel_state[2].out_value * 0x7ff + 0x7ff);
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        out[i].s[0] = in[i].s[0];
        out[i].s[1] = in[i].s[1];
    }
}
