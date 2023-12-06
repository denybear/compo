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
			list = &ui_list[0][0];
			break;
		case KBD:
			index = &kbd_list_index;
			list = &kbd_list[0][0];
			break;
		case OUT:
			index = &out_list_index;
			list = &out_list[0][0];
			break;
	}

	for (i = 0; i < 3; i++) {
		// add to the list
		list [((*index) * 3) + i] = buffer [i];
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

	int i, *index;
	uint8_t *list;

	switch (device) {
		case UI:
			index = &ui_list_index;
			list = &ui_list[0][0];
			break;
		case KBD:
			index = &kbd_list_index;
			list = &kbd_list[0][0];
			break;
		case OUT:
			index = &out_list_index;
			list = &out_list[0][0];
			break;
	}

	// check to which list we should add

	// check if we have requests to be pulled; if not, leave
	if (*index == 0) return 0;

	// pull first element from list
	for (i = 0; i < 3; i++) {
		// pull from the list
		buffer [i] = list [i];
	}
	// decrement index
	(*index)--;
	// move the rest of the list 1 item backwards
	memmove (list, list + 3, (*index) * 3);

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
	double ticks_per_bar;		// number of ticks per bar
	double temp;

	if (new_pos) {

		pos->frame_rate = jack_get_sample_rate(client);			// set frame rate (sample rate) to the BBT structure
		pos->valid = JackPositionBBT;
		pos->beats_per_bar = time_beats_per_bar;
		pos->beat_type = time_beat_type;
		pos->ticks_per_beat = time_ticks_per_beat;
		pos->beats_per_minute = time_beats_per_minute;

		// set BBT to (ui_current_bar),1,1; this is in the case of "play"
		pos->bar = ui_current_bar;
		pos->beat = 0;
		pos->tick = 0;
		pos->bar_start_tick = 0.0;
	}
	else {

		/* Compute BBT info based on previous period. */
		// (1) pos->ticks_per_beat * pos->beats_per_minute : number of ticks per minutes
		// (2) (pos->frame_rate * 60) / nframes : number of frames per minute
		// (1)/(2) = number of ticks per frame; (1)/(2) is equal to (1) * inv(2) 
		pos->bar_start_tick += (pos->ticks_per_beat * pos->beats_per_minute * nframes / (pos->frame_rate * 60));

		// computes BTT, based on float bar_start_tick (number of ticks since play is pressed)
		ticks_per_bar = pos->beats_per_bar * pos->ticks_per_beat;
		pos->tick = (int) pos->bar_start_tick % (int) ticks_per_bar;	// tick number within the bar
		pos->beat = pos->tick / (int) time_ticks_per_beat;				// beat number within the bar
		pos->bar = ui_current_bar + ((int) pos->bar_start_tick / (int) ticks_per_bar);		// bar number
	}
}


// create a table to help with quantization computing
// npb indicates number of notes per bar: 4 = Quarter, 8 = 8th, 16 = 16th, 32 = 32th
// table contains only 1 bar, this is enough
// table length is NPB + 1
void create_quantization_table (uint32_t *table, int npb) {
	int i;
	uint32_t time, increment, offset;

	table [0] = 0;				// special case for element 0

	increment = (4 * time_ticks_per_beat) / npb;	// increment between 2 notes, in tick; shall be multiplied by 4 as "quarter" note is actually 1 beat
	offset = increment / 2;							// offset is the start value

	for (i=0; i < npb; i++) {
		table [i + 1] = offset + (increment * i);
	}
}


// quantize a tick to the nearest value, according to quantizaton table
uint32_t quantize (uint32_t tick, int quant) {

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
