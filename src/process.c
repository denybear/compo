/** @file process.c
 *
 * @brief The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 */

#include "types.h"
#include "globals.h"
#include "process.h"
#include "utils.h"
#include "led.h"


// number of midi clock signal sent per quarter note; from 0 to 23
int midi_clock_num;
// previous index pulse: used to see whether we changed quarter note (beat)
int previous_index_pulse;


// main process callback called at capture of (nframes) frames/samples
int process ( jack_nframes_t nframes, void *arg )
{
	jack_nframes_t h;
	int i,j,k;
	int mute = OFF;
	void *midiin;
	void *midiout;
	jack_midi_event_t in_event;
	jack_midi_data_t buffer[5];				// midi out buffer for lighting the pad leds and for midi clock
	int dest, row, col, on_off;				// variables used to manage lighting of the pad leds


	/***************************************/
	/* First, process MIDI in (KBD) events */
	/***************************************/

	// Get midi buffers
	midiin = jack_port_get_buffer(midi_KBD_in, nframes);

	// process MIDI IN events
	for (i=0; i< jack_midi_get_event_count(midiin); i++) {
		if (jack_midi_event_get (&in_event, midiin, i) != 0) {
			fprintf ( stderr, "Missed in event\n" );
			continue;
		}
		// call processing function
		kbd_midi_in_process (&in_event,nframes);
	}


	/*******************************************/
	/* Second, process MIDI out events (music) */
	/*******************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_out, nframes);

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests


	/****************************************/
	/* Third, process MIDI out (KBD) events */
	/****************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_KBD_out, nframes);

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests


	/***************************************/
	/* Fourth, process MIDI in (UI) events */
	/***************************************/

	// Get midi buffers
	midiin = jack_port_get_buffer(midi_UI_in, nframes);

	// process MIDI IN events
	for (i=0; i< jack_midi_get_event_count(midiin); i++) {
		if (jack_midi_event_get (&in_event, midiin, i) != 0) {
			fprintf ( stderr, "Missed in event\n" );
			continue;
		}
		// call processing function
		ui_midi_in_process (&in_event,nframes);
	}


	/***************************************/
	/* Fifth, process MIDI out (UI) events */
	/***************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_UI_out, nframes);

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests
	while (pull_from_list (UI, buffer)) {
		// if buffer is not empty, then send as midi out event
		if (buffer [0] | buffer [1] | buffer [2]) {
			jack_midi_event_write (midiout, 0, buffer, 3);
		}
	}



	return 0;
}


// process callback called to process midi_in events from KBD in realtime
int kbd_midi_in_process (jack_midi_event_t *event, jack_nframes_t nframes) {
}

// process callback called to process midi_in events from UI in realtime
int ui_midi_in_process (jack_midi_event_t *event, jack_nframes_t nframes) {

	int i;
	int cmd, key, vel;
	int temp;

	cmd = event->buffer [0];
	key = event->buffer [1];
	vel = event->buffer [2];

	// check which event has been received
	switch (cmd) {
		case MIDI_CC:
			// an "instrument" pad has been pressed
			// do any work only if "press on", not "press off" (release of the pad)
			if (!vel) break;

			key = key - 0x68;		// get pad number from midi message

			// set color according to previous color of the pad (green = not muted; red = muted)
			switch (ui_instruments [key]) {
				case HI_GREEN:
					// the pad was selected before; next move is put to red (mute)
					ui_instruments [key] = HI_RED;
					break;
				case HI_RED:
					// the pad was selected before but was muted; next move is put to hi green (active)
					ui_instruments [key] = HI_GREEN;
					break;
				case LO_GREEN:
					// the pad was not selected before; next move is put to hi green (active)
					ui_instruments [key] = HI_GREEN;
					break;
				case LO_RED:
					// the pad was not selected before but was muted; next move is put to hi red (active but muted)
					ui_instruments [key] = HI_RED;
					break;
				default:
					break;
			}

			// unlight previous pad, if previous pad is different from pressed pad
			if (key != ui_current_instrument) {
				// set new color according to previous color... we could put more colors here !
				if (ui_instruments [ui_current_instrument] == HI_GREEN) ui_instruments [ui_current_instrument] = LO_GREEN;
				else ui_instruments [ui_current_instrument] = LO_RED;
				// unlight previous pad
				led_ui_instrument (ui_current_instrument);
			}
			
			// light new pad
			ui_current_instrument = key;		// ui_current_instrument is set to new pad
			led_ui_instrument (ui_current_instrument);

			// display a full new page of bars for this instrument
			led_ui_bars (ui_current_instrument, ui_current_page);
			break;

		case MIDI_NOTEOFF:
			// a "page" pad has been pressed
			if ((key & 0x0F) == 0x08) {
				// do nothing
			}
			// bar pad has been pressed
			else {
				key = midi2bar (key);	// convert pad number to position in table

				// select functionality
				if (key == ui_first_selected_bar) {
					// we released the first pad selected
					// selection is valid; set last selected pad if necessary
					if (ui_last_selected_bar == -1) ui_last_selected_bar = key;
					ui_select_in_progress = FALSE;		// selection is fully valid
				}
			}
			break;

		case MIDI_NOTEON:
			// a "page" pad has been pressed
			if ((key & 0x0F) == 0x08) {
				key = (key & 0xF0) >> 4;
				// do some work only if pressed pad is different from previous
				if (key != ui_current_page) {
					// unlight previous pad 
					ui_pages [ui_current_page] = LO_GREEN;
					led_ui_page (ui_current_page);
				
					// light new pad
					ui_current_page = key;		// ui_current_page is set to new pad
					ui_pages [ui_current_page] = HI_GREEN;
					led_ui_page (ui_current_page);

					// display a full new page of bars for this instrument
					led_ui_bars (ui_current_instrument, ui_current_page);
				}
			}
			// bar pad has been pressed
			else {
				key = midi2bar (key);	// convert pad number to position in table

				// check whether we are in the middle of a selection or it selection is done

				if (!ui_select_in_progress) {
					// no select operation is in progress: pressed pad is going to be the starting point of selection 
					// start selection : set 1st selected bar
					ui_first_selected_bar = key;
					ui_current_bar = key;
					ui_select_in_progress = FALSE;
					// light on
					// SAVE PREVIOUS LIGHT !!!
					ui_bars [ui_current_instrument][ui_current_page][key] = HI_GREEN;
					led_ui_bar (ui_current_instrument, ui_current_page, key);
				}
				else {
					// seelction in progress : pressed pad is going to be the end point of selection
					ui_last_selected_bar = key;
					// light on
					// SAVE PREVIOUS LIGHT !!!
					// attention si la selection est faite à l'envers !!!
						for (i=ui_first_selected_bar ; i < ui_last_selected_bar; i++) {
							ui_bars [ui_current_instrument][ui_current_page][i] = HI_GREEN;
							led_ui_bar (ui_current_instrument, ui_current_page, i);
						}
				}




				ui_bars [ui_current_instrument][ui_current_page][key] = HI_RED;
				ui_current_bar = key;
				led_ui_bar (ui_current_instrument, ui_current_page, ui_current_bar);
			}
			break;

		default:
			break;
	}
}


// time callback called at after "process" callback, and used to compute BBT
void timebbt (jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos, void *arg)
{
	if (new_pos) new_pos = 1;	// if new_pos is !=0, then force its value to 1
	// compute BBT
	compute_bbt (nframes, pos, new_pos);
}

