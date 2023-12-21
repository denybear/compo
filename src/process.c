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
#include "song.h"


// main process callback called at capture of (nframes) frames/samples
int process ( jack_nframes_t nframes, void *arg )
{
	int i,j,k;
	void *midiin;
	void *midiout;
	jack_midi_event_t in_event;
	jack_midi_data_t buffer[5];				// midi out buffer for lighting the pad leds and for midi clock
	int dest, row, col, on_off;				// variables used to manage lighting of the pad leds
	char ch;								// used to read keys from UI keyboard (not music MIDI keyboard)
	note_t *notes_to_play;
	int lg, bar, page;						// temp variables




	/*************************************/
	/* Step 0, compute BBT and play song */
	/*************************************/
	if (is_play) {
		// compute BBT
		compute_bbt (nframes, &time_position, FALSE);

		// read song to determine whether there are some notes to play
		notes_to_play = read_from_song (previous_time_position.bar, previous_time_position.tick, time_position.bar, time_position.tick, &lg);

		// go through the notes that we shall play
		for (i=0; i<lg; i++) {
			if (notes_to_play [i].already_played == TRUE) {
				// note was already played "live", no need to replay it this time; but shall be replayed next time
				notes_to_play [i].already_played = FALSE;
			}
			else {
				// note was not played live; play it
				buffer [0] = (notes_to_play [i].status) | (instr2chan (notes_to_play [i].instrument));
				buffer [1] = notes_to_play [i].key;
				buffer [2] = notes_to_play [i].vel;
				// send midi command out to play note
				push_to_list (OUT, buffer);
			}
		}

		// play metronome
		if (is_metronome) {
			// read song to determine whether there are some notes to play
			notes_to_play = read_from_metronome (previous_time_position.bar, previous_time_position.tick, time_position.bar, time_position.tick, &lg);

			// go through the notes that we shall play
			for (i=0; i<lg; i++) {
				// note was not played live; play it
				buffer [0] = (notes_to_play [i].status) | (instr2chan (notes_to_play [i].instrument));
				buffer [1] = notes_to_play [i].key;
				buffer [2] = notes_to_play [i].vel;
				// send midi command out to play note
				push_to_list (OUT, buffer);
			}
		}

		// do the UI: cursor shall progress with the bar
		// we shall adjust the bar number to map to a (page, bar) couple
		// we will display cursor only if required, ie. change of bar
		if (previous_time_position.bar != time_position.bar) {
			// change of bar, we move the cursor along to the new bar
			// note that we change UI based on new time_position, while we are still processing
			// events that occured between previous_time_position and time_position.
			// This is to simplify code
			page = (int) (time_position.bar / 64);
			bar = time_position.bar % 64;

			// check whether we are on the same page, or we need to move to a new page
			if (page != ui_current_page) {
				// display new page

				// unlight previous pad 
				ui_pages [ui_current_page] = LO_GREEN;
				led_ui_page (ui_current_page);
				
				// light new pad
				ui_current_page = page;		// ui_current_page is set to new pad
				ui_pages [ui_current_page] = HI_GREEN;
				led_ui_page (ui_current_page);

				// display a full new page of bars for this instrument
				led_ui_bars (ui_current_instrument, ui_current_page);
			}

			// display cursor on bar
			led_ui_select (bar, bar);
		}
		ui_current_bar = time_position.bar % 64;		// set ui current bar value between (0-63)
//		ui_current_bar = previous_time_position.bar % 64;		// set ui current bar value between (0-63)

		// copy current BBT position to previous BBT position
		memcpy (&previous_time_position, &time_position, sizeof (jack_position_t));
	}


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

	//go through the list of midi requests
	while (pull_from_list (OUT, buffer)) {
		// send midi stream
		midi_write (midiout, 0, buffer);
	}


	/****************************************/
	/* Third, process MIDI out (KBD) events */
	/****************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_KBD_out, nframes);

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests
	while (pull_from_list (KBD, buffer)) {
		// send midi stream
		midi_write (midiout, 0, buffer);
	}


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
		// send midi stream
		midi_write (midiout, 0, buffer);
	}


	/**************************************/
	/* Last, process keyboard (UI) events */
	/**************************************/

	ch = getch();
//if (ch !=0xFF) printf ("char: %02X\n", ch);
	switch (ch) {
		case 'p':	// PLAY
			// status variable
			is_play = is_play ? FALSE : TRUE;
			// reset BBT position
			compute_bbt (nframes, &time_position, TRUE);
			// copy BBT to previous position as well
			memcpy (&previous_time_position, &time_position, sizeof (jack_position_t));
			break;
		case 'r':	// RECORD
			// make sure no selection is in progress to enable record
			// WE SHOULD DO BETTER THAN THIS
			if ((!ui_limit1_pressed) && (!ui_limit2_pressed)) {
				// status variable
				is_record = is_record ? FALSE : TRUE;
				led_ui_select (ui_current_bar, ui_current_bar);
			}
			break;
		case 't':	// TAP TEMPO
			if (tap1 == 0) {
				tap1 = jack_last_frame_time(client);
				tap2 = 0;
			}
			else tap2 = jack_last_frame_time(client);
			
			// determine timing between taps, and therefore tempo
			if ((tap1 != 0) && (tap2 != 0)) {
				// formula to determine BPM
				// 48000 * 60 samples --> 1 min
				// 1 beat = X samples --> X / (48000 * 60) min
				// 1 min --> ((48000 * 60) / X) beats = BPM 
				time_beats_per_minute = (int) ((jack_get_sample_rate (client) * 60.0) / (tap2 - tap1));
				time_position.beats_per_minute = time_beats_per_minute;
				// prepare for next call
				tap1 = tap2;
			}
			break;
		case 'm':	// METRONOME ON/OFF
			is_metronome = is_metronome ? FALSE : TRUE;
		default:
			break;
	}

	return 0;
}


// process callback called to process midi_in events from KBD in realtime
int kbd_midi_in_process (jack_midi_event_t *event, jack_nframes_t nframes) {

	uint8_t buffer [4];
	note_t note;


	buffer [0] = event->buffer [0];
	buffer [1] = event->buffer [1];
	buffer [2] = event->buffer [2];

	// play the music straight
	// get midi channel from instrument number, and assign it to midi command
	buffer [0] = (buffer [0] & 0xF0) | (instr2chan (ui_current_instrument));
	push_to_list (OUT, buffer);			// put in midisend buffer to play the note straight !


	// in case recording is on
	if (is_record && is_play) {
		// fill-in note structure
		note.already_played = TRUE;
		note.instrument = ui_current_instrument;
		note.bar = time_position.bar;
		note.beat = time_position.beat;
		note.tick = quantize (time_position.tick, quantizer);
		// check whether tick is on next bar or not
		if (note.tick == 0xFFFFFFFF) {
			// check boundaries of bar
			if (time_position.bar < 511) note.bar = time_position.bar + 1;
			else note.bar = 0;
			note.beat = 0;
			note.tick = 0;
		}
		note.status = buffer [0] & 0xF0;		// midi command only, no midi channel (it is set at play time)
		note.key = buffer [1];
		note.vel = buffer [2];
		// add quantized note to song
		write_to_song (note);

		// we don't do any UI yet as UI is done in the "play" section
	}
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
				if (key == ui_limit1) {
					// we released the first pad selected
					ui_limit1_pressed = FALSE;		// first limit valid
				}
				if (key == ui_limit2) {
					// we released the last pad selected
					ui_limit2_pressed = FALSE;		// last bar valid
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
				if ((!ui_limit1_pressed) && (!ui_limit2_pressed)) {
					//if selection has been done previously (no selection in progress), pressing a pad resets everything
					//assign pressed pad to limit1
					ui_limit1 = key;
					ui_limit2 = key;
					ui_limit1_pressed = TRUE;
					ui_limit2_pressed = FALSE;
					// display between limit 1 and 2
					ui_current_bar = led_ui_select (ui_limit1, ui_limit2);
				}
				else {
					if ((ui_limit1_pressed) && (!ui_limit2_pressed)) {
						//limit1 is assigned already; assign limit2
						ui_limit2 = key;
						ui_limit2_pressed = TRUE;
						// display between limit 1 and 2
						ui_current_bar = led_ui_select (ui_limit1, ui_limit2);
					}
					else {
						if ((!ui_limit1_pressed) && (ui_limit2_pressed)) {
							//limit2 is assigned already; assign limit1
							ui_limit1 = key;
							ui_limit1_pressed = TRUE;
							// display between limit 1 and 2
							ui_current_bar = led_ui_select (ui_limit1, ui_limit2);
						}
					}
				}
			}
			break;

		default:
			break;
	}
}


