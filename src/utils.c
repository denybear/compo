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


// convert pad midi number to bar number : ie 0x00-0x77 to 0-63
uint8_t midi2bar (uint8_t midi) {

	uint8_t bar;

	bar = (((midi & 0xF0) >> 4) * 8 ) + (midi & 0x0F);
	//printf ("midi: %02X, bar:%02X\n", midi, bar);
	return (bar);
}


// convert bar number to pad midi number : ie 0-63 to 0x00-0x77
uint8_t bar2midi (uint8_t bar) {

	uint8_t midi;

	midi = ((bar >> 3) << 4) + (bar & 0x7);
	//printf ("bar: %02X, midi:%02X\n", bar, midi);
	return (midi);
}


// convert song instrument to midi channel
uint8_t instr2chan (uint8_t instr) {

	const uint8_t chan [8] = {9, 0, 1, 2, 3, 4, 5, 6};		// 1st instrument is drum channel
	return (chan [instr]);
}



// add led request to the list of requests to be processed
int push_to_list (int device, uint8_t * buffer) {

	int i, *index;
	uint8_t *list;

	switch (device) {
		case UI:
			index = &ui_list_index;
			list = ui_list;
			break;
		case KBD:
			index = &kbd_list_index;
			list = kbd_list;
			break;
		case OUT:
			index = &out_list_index;
			list = out_list;
			break;
	}

	for (i = 0; i < 3; i++) {
		// add to the list
		list [(*index * 3) + i] = buffer [i];
	}
	// increment index and check boundaries
	if (*index >= (LIST_ELT-1)) *index = 0;
	else (*index)++;

/*
	// check to which list we should add
	if (device == UI) {
		for (i = 0; i < 3; i++) {
			// add to the list
			ui_list [ui_list_index] [i] = buffer [i];
		}
		// increment index and check boundaries
		if (ui_list_index >= (LIST_ELT-1)) ui_list_index = 0;
		else ui_list_index++;
	}
*/
}


// pull out led request from the list of requests to be processed (FIFO style)
// returns 0 if pull request has failed (nomore request to be pulled out)
int pull_from_list (int device, uint8_t * buffer) {

	int i;

	// check to which list we should add
	if (device == UI) {
		// check if we have requests to be pulled; if not, leave
		if (ui_list_index == 0) return 0;

		// pull first element from list
		for (i = 0; i < 3; i++) {
			// pull from the list
			buffer [i] = ui_list [0][i];
		}
		// decrement index
		ui_list_index--;
		// move the rest of the list 1 item backwards
		memmove (&ui_list [0][0], &ui_list [1][0], ui_list_index * 3);
	}
	else {
		// check if we have requests to be pulled; if not, leave
		if (kbd_list_index == 0) return 0;

		// pull first element from list
		for (i = 0; i < 3; i++) {
			// pull from the list
			buffer [i] = kbd_list [0][i];
		}

		// decrement index
		kbd_list_index--;
		// move the rest of the list 1 item backwards
		memmove (&kbd_list [0][0], &kbd_list [1][0], kbd_list_index * 3);
	}
	return 1;
}


// compute BBT based on nframes, tempo (BPM), frame rate, etc.
// based on new_pos value:
// 0 : compute BBT based on previous values of BBT & fram rate
// 1 : compute BBT based on start of Jack
// 2 : compute BBT, and sets position as 1, 1, 1
// requires jack_position_t * which will contain the BBT information
void compute_bbt (jack_nframes_t nframes, jack_position_t *pos, int new_pos)
{
	double min;			/* minutes since frame 0 */
	long abs_tick;		/* ticks since frame 0 */
	long abs_beat;		/* beats since frame 0 */

	if (new_pos) {
		pos->valid = JackPositionBBT;
		pos->beats_per_bar = time_beats_per_bar;
		pos->beat_type = time_beat_type;
		pos->ticks_per_beat = time_ticks_per_beat;
		pos->beats_per_minute = time_beats_per_minute;

		if (new_pos == 2) {
			// set BBT to 1,1,1
			pos->bar = 1;
			pos->beat = 1;
			pos->tick = 1;
			pos->bar_start_tick = 1;
		}
		else {

			/* Compute BBT info from frame number.  This is
			 * relatively simple here, but would become complex if
			 * we supported tempo or time signature changes at
			 * specific locations in the transport timeline.  I
			 * make no claims for the numerical accuracy or
			 * efficiency of these calculations. */

			min = pos->frame / ((double) pos->frame_rate * 60.0);
			abs_tick = min * pos->beats_per_minute * pos->ticks_per_beat;
			abs_beat = abs_tick / pos->ticks_per_beat;

			pos->bar = abs_beat / pos->beats_per_bar;
			pos->beat = abs_beat - (pos->bar * pos->beats_per_bar) + 1;
			pos->tick = abs_tick - (abs_beat * pos->ticks_per_beat);
			pos->bar_start_tick = pos->bar * pos->beats_per_bar * pos->ticks_per_beat;
			pos->bar++;		/* adjust start to bar 1 */
		}
	}
	else {

		/* Compute BBT info based on previous period. */
		pos->tick += (nframes * pos->ticks_per_beat * pos->beats_per_minute / (pos->frame_rate * 60));

		while (pos->tick >= pos->ticks_per_beat) {
			pos->tick -= pos->ticks_per_beat;
			if (++pos->beat > pos->beats_per_bar) {
				pos->beat = 1;
				++pos->bar;
				pos->bar_start_tick += (pos->beats_per_bar * pos->ticks_per_beat);
			}
		}
	}
printf ("%d, %d, %d, %d\n", pos->bar, pos->beat, pos->tick, pos->frame_rate);
}


// create a table to hep with quantization computing
// npb indicates number of notes per bar: 4 = Quarter, 8 = 8th, 16 = 16th, 32 = 32th
// table is big enough to cover 64 bars * 8 = 512 bars, therefore 512 * npb notes
void create_quantization_table (uint32_t *table, int npb) {
	int i;
	uint32_t time, increment, offset;

	table [0] = 0;				// special case for element 0

	increment = time_ticks_per_beat / npb * 4;	// increment between 2 notes, in tick
	offset = increment / 2;						// offset is the start value

	for (i=0; i < npb * 512 ; i++) {				// 512 bars
		table [i + 1] = offset + (increment * i);
printf ("%d, ", table [i+0]);
	}
}


// init a midi instrument to each channel
// https://www.recordingblogs.com/wiki/midi-program-change-message
void init_instruments () {

	uint8_t buffer [4], chan;
	int i;
	const uint8_t instr [8] = {0, 0, 2, 16, 33, 27, 48, 61};		// (drum), piano, elec piano, hammond organ, fingered bass, clean guitar, string ensemble, brass ensemble
	
	for (i = 0; i < 8; i++){

		chan = instr2chan (i);
		if (chan != 9) {
			// non-drum instruments will get program change
			buffer [0] = MIDI_PC | chan;
			buffer [1] = instr [i];
			buffer [2] = 0;
		}

		push_to_list (OUT, buffer);			// put in midi send buffer to assign instruments to midi channels
	}
}


// init volume to each midi channel
void init_volumes (uint8_t vol) {

	uint8_t buffer [4];
	int i;
	
	for (i = 0; i < 8; i++){

		buffer [0] = MIDI_CC | instr2chan (i);
		buffer [1] = 0x07;					// 0x07 is Volume Control Change
		buffer [2] = vol;					// volume goes from 0 to 127

		push_to_list (OUT, buffer);			// put in midi send buffer to assign volume to midi channels
	}
}
