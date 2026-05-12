#include "voices.h"
#include "analog_out.h"
#include "gpio.h"

#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

#define VOICE_COUNT 2
#define CHANNEL_COUNT 16

typedef struct {
    bool active;
    bool sustained;
    int channel;
    int note;
    int serial_number;
} voice_t;

typedef struct {
    voice_t voice[VOICE_COUNT];
    float pitch_bend[CHANNEL_COUNT];
    bool sustain[CHANNEL_COUNT];
} voices_state_t;
static voices_state_t s_state;
static int s_next_serial_number = 0;  // I don't think it'll overflow soon

static unsigned note_to_dac_value(int note, float pitch_bend)
{
    // DAC output 0..0xfff -> -5 to +5V or so. Middle C (note 60) ends up around 0V.
    const float tuning = 1.35f;  // Not sure why this isn't very close to 1.0
    const float steps_per_note = tuning * 0xfff / 10.0f / 12;
    return steps_per_note * (note + pitch_bend * 2 - 12);
}

static void update_dac(void)
{
    unsigned dac[3] = {};
    for (int i = 0; i < 3 && i < VOICE_COUNT; i++) {
        voice_t *v = &s_state.voice[i];
        dac[i] = note_to_dac_value(v->note, s_state.pitch_bend[v->channel]);
    }
    analog_out_set(dac[0], dac[1], dac[2]);
}

static void voice_on(voice_t *voice)
{
    printf("voice on %d\r\n", voice->note);
    int this_voice_index = 0;
    for (int i = 0; i < VOICE_COUNT; i++) {
        if (&s_state.voice[i] == voice) {
            this_voice_index = i;
        }
    }
    gpio_set(PIN_GATE1 + this_voice_index, true);
    gpio_set_led(this_voice_index, true);
    update_dac();
}

static void voice_release(voice_t *voice)
{
    printf("voice release %d\r\n", voice->note);
    voice->active = false;
    voice->sustained = false;
    int this_voice_index = 0;
    for (int i = 0; i < VOICE_COUNT; i++) {
        if (&s_state.voice[i] == voice) {
            this_voice_index = i;
        }
    }
    gpio_set(PIN_GATE1 + this_voice_index, false);
    gpio_set_led(this_voice_index, false);
    update_dac();
}

static voice_t *get_next_voice(void)
{
    voice_t *oldest_voice = NULL;
    int oldest_serial = INT_MAX;

    for (int i = 0; i < VOICE_COUNT; i++) {
        voice_t *voice = &s_state.voice[i];
        if (!voice->active && !voice->sustained) {
            voice->active = true;
            voice->serial_number = s_next_serial_number++;
            return voice;
        }
        if (voice->serial_number < oldest_serial) {
            oldest_voice = voice;
            oldest_serial = voice->serial_number;
        }
    }

    // No voice available, steal the oldest one
    voice_release(oldest_voice);
    oldest_voice->active = true;
    oldest_voice->serial_number = s_next_serial_number++;
    return oldest_voice;
}

void voices_note_event(int channel, int note, int velocity)
{
    if (velocity == 0) {
        // Note off event
        for (int i = 0; i < VOICE_COUNT; i++) {
            voice_t *voice = &s_state.voice[i];
            if ((voice->active || voice->sustained) && voice->channel == channel && voice->note == note) {
                voice->active = false;
                if (!voice->sustained) {
                    voice_release(voice);
                }
            }
        }
    }
    else {
        // Look for if it's already playing (probably sustained)
        for (int i = 0; i < VOICE_COUNT; i++) {
            voice_t *voice = &s_state.voice[i];
            if ((voice->active || voice->sustained) && voice->channel == channel && voice->note == note) {
                return;
            }
        }

        voice_t *voice = get_next_voice();
        voice->channel = channel;
        voice->note = note;
        voice->sustained = s_state.sustain[voice->channel];
        voice_on(voice);
    }
}

void voices_pb_event(int channel, int bend)
{
    printf("bend [%d] %d\r\n", channel, bend);
    s_state.pitch_bend[channel] = bend * (1.0f / 8192);  // Normalize to ±1
    update_dac();
}

void voices_cc_event(int channel, int cc, int value)
{
    printf("cc [%d] %d=%d\r\n", channel, cc, value);
    switch (cc) {
        case 64:  // Sustain pedal
            const bool sustain = value >= 64;
            s_state.sustain[channel] = sustain;
            for (int i = 0; i < VOICE_COUNT; i++) {
                voice_t *voice = &s_state.voice[i];
                if (voice->channel == channel) {
                    if (!voice->active && voice->sustained && !sustain) {
                        voice_release(voice);
                    }
                    else if (voice->active || voice->sustained) {
                        voice->sustained = s_state.sustain[voice->channel];
                    }
                }
            }
            break;
    }
}

void voices_at_event(int channel, int note, int value)
{
    (void)note;
    printf("at [%d] %d\r\n", channel, value);
}
