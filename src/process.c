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
	double bpm;								// temp variable for tap tempo




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
				// play note only if note should be played (mute, solo, etc)
				if (should_play (notes_to_play [i].instrument)) push_to_list (OUT, buffer); 	// send midi command out to play note
			}
		}

		// play metronome
		if (is_metronome) {
			// read metronome to determine whether there are some notes to play
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
		ui_current_bar = time_position.bar % 64;				// set ui current bar value between (0-63)
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
		case NUM_ENTER:	// PLAY
		case SNUM_ENTER:
			// status variable
			is_play = is_play ? FALSE : TRUE;
			// reset BBT position
			compute_bbt (nframes, &time_position, TRUE);
			// copy BBT to previous position as well
			memcpy (&previous_time_position, &time_position, sizeof (jack_position_t));
			break;
		case NUM_DOT:	// RECORD
		case SNUM_DOT:
			// make sure no selection is in progress to enable record
			// status variable
			is_record = is_record ? FALSE : TRUE;
			// if play in progress, red-ify current bar otherwise red-ify current selection
			if (is_play) led_ui_select (ui_current_bar, ui_current_bar);
			else led_ui_select (ui_limit1, ui_limit2);
			break;
		case NUM_4:	// TAP TEMPO
		case SNUM_4:
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
				bpm = (int) ((jack_get_sample_rate (client) * 60.0) / (tap2 - tap1));

				// check that BPM is above a certain value to allow tempo change
				if (bpm >= 40) {
					time_beats_per_minute = bpm;
					time_position.beats_per_minute = time_beats_per_minute;
					time_bpm_multiplier = 1.0;		// reset bpm multiplier to initial value
					// prepare for next call
					tap1 = tap2;
				}
				else {
					// if tempo is too low, consider this is a first press
					tap1 = jack_last_frame_time(client);
					tap2 = 0;
				}
			}
			break;
		case NUM_5:	// TEMPO -
		case SNUM_5:
			if (time_bpm_multiplier <= 0.1) break;		// if low boundary reached, do nothing
			time_bpm_multiplier -= 0.1;
			time_position.beats_per_minute = (int) (time_beats_per_minute * time_bpm_multiplier);
			break;
		case NUM_6:	// TEMPO +
		case SNUM_6:
			if (time_bpm_multiplier >= 3.0) break;		// if high boundary reached, do nothing
			time_bpm_multiplier += 0.1;
			time_position.beats_per_minute = (int) (time_beats_per_minute * time_bpm_multiplier);
			break;
		case NUM_BACK:	// METRONOME ON/OFF
		// case SNUM_BACK:
			is_metronome = is_metronome ? FALSE : TRUE;
			break;
		case NUM_1:	// COPY
		case SNUM_1:
			bar_process (COPY);
			break;
		case NUM_2:	// CUT
		case SNUM_2:
			bar_process (CUT);
			break;
		case NUM_0:	// PASTE
		case SNUM_0:
			bar_process (PASTE);
			break;
		case NUM_000:	// COLOR
		// case SNUM_000:
			// empty numpad buffer, as "000" on numpad returns C3A0C3A0C3A0
//			getch();	// 0xA0
//			getch();	// 0xC3
//			getch();	// 0xA0
//			getch();	// 0xC3
//			getch();	// 0xA0
			bar_process (COLOR);
			break;
		case NUM_7:	// INSERT BARS
		case SNUM_7:
			bar_process (INSERT);
			break;
		case NUM_8:	// REMOVE BARS
		case SNUM_8:
			bar_process (REMOVE);
			break;
		case NUM_9:	// TRANSPO -
		case SNUM_9:
			transpo_process (ui_current_instrument, MINUS);
			break;
		case NUM_PLUS:	// TRANSPO +
		case SNUM_PLUS:
			transpo_process (ui_current_instrument ,PLUS);
			break;
		case NUM_SLASH:	// LOAD
			break;
		case NUM_MINUS:	// SAVE
			break;
		case SNUM_SLASH:	// QUANTISATION
			break;
		case SNUM_MINUS:	// VELOCITY
			break;
		default:
			break;
	}

	flushinp ();	// flush all the other numpad inputs; very useful to avoid key repeats and "000" key issue (which returns C3A0C3A0C3A0)

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
	// play note only if note should be played (mute, solo, etc)
	if (should_play (ui_current_instrument)) push_to_list (OUT, buffer);	// put in midisend buffer to play the note straight !


	// in case recording is on
	if (is_record && is_play) {
		// fill-in note structure
		note.instrument = ui_current_instrument;
		note.bar = time_position.bar;
		note.beat = time_position.beat;
		note.tick = quantize (time_position.tick, quantizer);
		// if note is to be played in the future, indicate we have played it already
		if (note.tick >= time_position.tick) note.already_played = TRUE;
		else note.already_played = FALSE;
		// check whether tick is on next bar or not
		if (note.tick == 0xFFFFFFFF) {
			// check boundaries of bar
			if (time_position.bar < 511) note.bar = time_position.bar + 1;
			else note.bar = 0;
			note.beat = 0;
			note.tick = 0;
			note.already_played = TRUE;
		}
		note.status = buffer [0] & 0xF0;		// midi command only, no midi channel (it is set at play time)
		note.key = buffer [1];
		note.vel = buffer [2];
		// add quantized note to song
		write_to_song (note);

		// we have recorded something in the bar : set bar to a color
		if (ui_bars [ui_current_instrument][note.bar / 64][note.bar % 64] == BLACK) {
			ui_bars [ui_current_instrument][note.bar / 64][note.bar % 64] = LO_YELLOW;
		}

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

			// set color according to previous color of the pad (green = not muted; red = solo; black = muted; yellow = choice of instrument (1/2); amber = choice of instrument (2/2))
			switch (ui_instruments [key]) {
				case HI_GREEN:
					// the pad was selected before; next move is put to black (mute)
					ui_instruments [key] = BLACK;
					break;
				case BLACK:
					// the pad was selected before but was muted; next move is put to hi red (solo)
					ui_instruments [key] = HI_RED;
					break;
				case HI_RED:
					// the pad was selected before but was solo; next move is put to hi yellow (active)
					ui_instruments [key] = HI_GREEN;
					break;
				case HI_YELLOW:
					// the pad was selected before (choice of instr); next move is put to hi amber (active)
					ui_instruments [key] = HI_AMBER;
					break;
				case HI_AMBER:
					// the pad was selected before (choice of instr); next move is put to hi green (active)
					ui_instruments [key] = HI_GREEN;
					break;
				case LO_GREEN:
					// the pad was not selected before; next move is put to hi green (active)
					ui_instruments [key] = HI_GREEN;
					break;
				case LO_BLACK:	// LO_BLACK is a dummy value to show that pad in inactive
					// the pad was not selected before; next move is put to black (active but muted)
					ui_instruments [key] = BLACK;
					break;
				case LO_RED:
					// the pad was not selected before but was muted; next move is put to hi red (active but muted)
					ui_instruments [key] = HI_RED;
					break;
				case LO_YELLOW:
					// the pad was not selected before (choice of instr); next move is put to active
					ui_instruments [key] = HI_YELLOW;
					break;
				case LO_AMBER:
					// the pad was not selected before (choice of instr); next move is put to active
					ui_instruments [key] = HI_AMBER;
					break;
				default:
					break;
			}

			// unlight previous pad, if previous pad is different from pressed pad
			if (key != ui_current_instrument) {
				// set new color according to previous color

				switch (ui_instruments [ui_current_instrument]) {
					case HI_GREEN:
						ui_instruments [ui_current_instrument] = LO_GREEN;
						break;
					case BLACK:
						ui_instruments [ui_current_instrument] = LO_BLACK;
						break;
					case HI_RED:
						ui_instruments [ui_current_instrument] = LO_RED;
						break;
					case HI_YELLOW:
						ui_instruments [ui_current_instrument] = LO_YELLOW;
						break;
					case HI_AMBER:
						ui_instruments [ui_current_instrument] = LO_AMBER;
						break;
					default:
						break;
				}
				// unlight previous pad
				led_ui_instrument (ui_current_instrument);
			}
			
			// light new pad
			ui_current_instrument = key;		// ui_current_instrument is set to new pad
			led_ui_instrument (ui_current_instrument);

			// display a full new page of bars for this instrument
			// except in case of change of instrument
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


// process callback called in case of copy, cut, paste, insert, delete, color
void bar_process (int mode) {

	int i, j;
	int page, bar, blimit1, blimit2, limit1, limit2;		// temp variables

		// if play in process, do nothing
		if (is_play) return;
		// if keypresses in progress, do nothing
		if ((ui_limit1_pressed) || (ui_limit2_pressed)) return;
			
		blimit1 = ui_limit1 + (ui_current_page * 64);		// determine start bar number from page num
		blimit2 = ui_limit2 + (ui_current_page * 64);		// determine end bar number from page num

		switch (mode) {
			case COPY:
				copy (blimit1, blimit2, ui_current_instrument);		// copy and put in copy buffer
				// copy color of bars to specific buffer
				j = 0;
				for (i = blimit1; i < blimit2; i++) {
					page = i / 64;									// compute page number
					bar = i % 64;									// compute bar number
					led_copy_buffer [j++] = ui_bars [ui_current_instrument][page][bar];
				}
				led_copy_length = j;
/*
				j = 0;
				for (i = ui_limit1; i < ui_limit2; i++) {
					led_copy_buffer [j++] = ui_bars [ui_current_instrument][ui_current_page][i];
				}
				led_copy_length = j;
*/
				break;
			case CUT:
				cut (blimit1, blimit2, ui_current_instrument);		// cut and put in copy buffer
				// copy color of bars to specific buffer and clear bars that have been cut in the UI
				j = 0;
				for (i = blimit1; i < blimit2; i++) {
					page = i / 64;									// compute page number
					bar = i % 64;									// compute bar number
					led_copy_buffer [j++] = ui_bars [ui_current_instrument][page][bar];
					ui_bars [ui_current_instrument][page][bar] = BLACK;
				}
				led_copy_length = j;
/*
				j = 0;
				for (i = ui_limit1; i < ui_limit2; i++) {
					led_copy_buffer [j++] = ui_bars [ui_current_instrument][ui_current_page][i];
					ui_bars [ui_current_instrument][ui_current_page][i] = BLACK;
				}
				led_copy_length = j;
*/
				break;
			case PASTE:
				// if copy buffer is empty, do nothing
				if (copy_length == 0) return;
			
				paste (blimit1, ui_current_instrument);				// paste from copy buffer
				// copy color of bars to match what has been copied
				for (i = 0; i < led_copy_length; i++) {
					page = (i + blimit1) / 64;									// compute page number
					bar = (i + blimit1) % 64;									// compute bar number
					// check we are not out of boundaries
					if (page < 8) ui_bars [ui_current_instrument][page][bar] = led_copy_buffer [i];
				}
/*
				for (i = 0; i < led_copy_length; i++) {
					// check whether we are in the same page or we should change page
					// according to memory structure, it should not be necessary; but you never know
					// here : we limit UI changes on 2 pages, as this is the max necessary according to UI
					if ((ui_current_bar + i) < 64) {
						ui_bars [ui_current_instrument][ui_current_page][ui_current_bar + i] = led_copy_buffer [i];
					}
					else {
						// copy if page number is within boundaries
						if (ui_current_page < 7) ui_bars  [ui_current_instrument][ui_current_page + 1][(ui_current_bar + i) % 64] = led_copy_buffer [i];
					}
				}
*/
				break;
			case INSERT:
			case REMOVE:
				// insert or remove X bars at the given position
				if (mode == INSERT) {		// INSERT bars
					limit1 = blimit1;
					limit2 = blimit2;
				}
				else {						// REMOVE bars
					limit1 = blimit2;
					limit2 = blimit1;
				}

				// first we cut all the bars of song from limit (beg or end of selection) position onwards
				cut (limit1, 512, ui_current_instrument);			// cut and put in copy buffer
				// copy color of bars to specific buffer and clear bars until end of the song
				j = 0;
				for (i = limit1; i < 512; i++) {
					page = i / 64;									// compute page number
					bar = i % 64;									// compute bar number
					led_copy_buffer [j++] = ui_bars [ui_current_instrument][page][bar];
					ui_bars [ui_current_instrument][page][bar] = BLACK;
				}
				led_copy_length = j;

				// then we paste from new position onwards
				paste (limit2, ui_current_instrument);				// paste from copy buffer
				// copy color of bars to match what has been copied
				for (i = 0; i < led_copy_length; i++) {
					page = (i + limit2) / 64;						// compute page number
					bar = (i + limit2) % 64;						// compute bar number
					// check we are not out of boundaries (in case of insert; no issue with remove)
					if (page < 8) ui_bars [ui_current_instrument][page][bar] = led_copy_buffer [i];
				}
				break;
			case COLOR:
				// change color of bars to their next color
				for (i = ui_limit1; i < ui_limit2; i++) {
					ui_bars [ui_current_instrument][ui_current_page][i] = color_ui_bar (ui_bars [ui_current_instrument][ui_current_page][i]);
				}
				break;
			default:
				break;
		}

		// display new bars on the UI; redisplay the whole page, it is easier
		led_ui_bars (ui_current_instrument, ui_current_page);
		// display cursor on bar
		// at this point, ui_current_bar should ALWAYS be set to selection limit1 (left limit) 
		led_ui_select (ui_current_bar, ui_current_bar);
}


void transpo_process (int instr, int mode) {

	int i;

	// read through all the song
	for (i = 0; i < song_length; i++){
		// check if note has the right instrument
		if (song [i].instrument == instr) {
			if (mode == PLUS) {
				// Plus 1/2 tone (check boundaries)
				if (song [i].key < 0x7F) song [i].key++;
			}
			else {
				// Minus 1/2 tone (check boundaries)
				if (song [i].key > 0x00) song [i].key--;
			}
		}
	}
	return;
}