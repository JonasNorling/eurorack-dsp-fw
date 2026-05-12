#pragma once

void voices_note_event(int channel, int note, int velocity);
void voices_pb_event(int channel, int bend);
void voices_cc_event(int channel, int cc, int value);
void voices_at_event(int channel, int note, int value);
