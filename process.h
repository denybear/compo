/** @file process.h
 *
 * @brief This file defines prototypes of functions inside process.c
 *
 */

int process ( jack_nframes_t, void *);
int kbd_midi_in_process (jack_midi_event_t *, jack_nframes_t);
int ui_midi_in_process (jack_midi_event_t *, jack_nframes_t);
void bar_process (int);
void transpo_process (int, int);
void start_playing ();
void stop_playing ();
