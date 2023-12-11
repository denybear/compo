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
		// check bar boundaries; go to bar 0 if we reach last bar
		if (pos->bar > 512) pos->bar = pos->bar % 512;		// 512 = 64 bar * 8 pages; we loop after 512 bars
	}
}


// create tables to help with quantization computing
// npb indicates number of notes per bar: 1 = Quarter, 2 = 8th, 3 = 16th, 4 = 32th
// 2 tables contaning only 1 bar, this is enough
// table length is NPB + 1
// one table contains timing ranges of notes, allowing to determine the right timing
// the other table contains the correct timing to be returned
void create_quantization_tables () {
	const int pow [5] = {0, 4, 8, 16, 32};	// number of notes per bar
	int i, j, nbp;
	uint32_t time, increment, offset;

	for (j = 0; j < 5; j++) {			// go from free-timing to 4th, 8th, 16th, 32th

		nbp = pow [j];					// convert type of note into number of divisions 
		quantization_range [j][0] = 0;	// special case for element 0

		if (nbp !=0) increment = time_beats_per_bar * time_ticks_per_beat / nbp;		// increment between 2 notes, in tick
		else increment = 0;											// in case of free-timing, table will not be used anyway

		offset = increment / 2;										// offset is the start value

		for (i=0; i < (nbp + 1); i++) {								// go through a complete bar
			// (npb + 1) allows to have an extra value in table, which is useful to determine if a note belongs to the next bar
			// example : NPB = 1 (quarter note), PPQ = 480
			// range table is: 0, 240, 720, 1200, 1680, 2160
			// value table is: 0, 480, 960, 1440, 1920
			// the last values of tables mean note is on the next bar (at PPQ=480, 1920 is start of next bar; 2160 does not exist in theory)
			quantization_range [j][i + 1] = offset + (increment * i);
			quantization_value [j][i] = increment * i;
		}

		if (nbp !=0) quantization_value [j][i-1] = 0xFFFFFFFF;	// force last value of table to FFFFFFFF to indicate it is on next bar
	}
}


// quantize a tick to the nearest value, according to quantization table
// in case tick is on next bar, return 0xFFFFFFFF (as it s defined in quantization table)
uint32_t quantize (uint32_t tick, int quant) {
	const int pow [5] = {0, 4, 8, 16, 32};	// number of notes per bar
	int i, nbp;

	nbp = pow [quant];					// convert type of note into number of divisions 

	if (quant == FREE_TIMING) return tick;

	for (i=0; i < (nbp + 1); i++) {		// go through a complete bar
		if ((tick >= quantization_range [quant][i]) && (tick < quantization_range [quant][i+1])) return (quantization_value [quant][i]);
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


// write a note to song structure; insert it to the right place
// song structure is sorted by bar, beat, tick; then by instrument
// this means the song structure is sorted every time a new note is written
void write_to_song (note_t note) {

	int i;
	int bar_limit1, bar_limit2;
	int tick_limit1, tick_limit2;
	int instrument_limit1, instrument_limit2;

	// special case if song is too large : just do nothing and leave
	if (song_length >= SONG_SIZE) return;

	// special case if song is empty; we insert note straight ahead and leave
	if (song_length == 0) {
		memcpy (&song [0], &note, sizeof (note));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}

	////////////
	// check bar
	////////////
	// go through the song to locate the right bar; stop if we found the right bar or next bar
	for (i=0; i<song_length; i++) {
		if (song [i].bar >= note.bar) {
			bar_limit1 = i;		// limit1 is either the first "same" bar, or first "higher" bar
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit1 is at the end of the song
	if (i==song_length) bar_limit1 = i;

	for (i=0; i<song_length; i++) {
		if (song [i].bar > note.bar) {
			bar_limit2 = i;		// limit2 is the first "higher" bar
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the end of the song
	if (i==song_length) bar_limit2 = i;

	// in case limit1 == limit2, it means there is no similar bar in the song yet; we can insert the note and leave
	if (bar_limit1 == bar_limit2) {
		// move rest of song 1 note ahead (to the right)
		memmove (&song [bar_limit1 + 1], &song [bar_limit1], (song_length - bar_limit1) * sizeof (note));		// memmove to prevent memory overlapping
		// copy note in the empty space
		memcpy (&song [bar_limit1], &note, sizeof (note));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}

	/////////////
	// check tick
	/////////////
	// there is no need to check beat, as tick goes from 0 to max_tick in a bar, it is not reset to 0 at each beat
	// note to insert is between bar_limit1 and bar_limit2
	// go through the song (between limit1 and limit 2) to locate the right tick; stop if we found the right tick or next tick
	for (i=bar_limit1; i<bar_limit2; i++) {
		if (song [i].tick >= note.tick) {
			tick_limit1 = i;		// limit1 is either the first "same" tick, or first "higher" tick in the bar
			break;
		}
	}
	// test if we reached bar boundary without success: in this case, limit1 is at the bar boundary
	if (i==bar_limit2) tick_limit1 = i;

	for (i=bar_limit1; i<bar_limit2; i++) {
		if (song [i].tick > note.tick) {
			tick_limit2 = i;		// limit2 is the first "higher" tick of the bar
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the boundary
	if (i==bar_limit2) tick_limit2 = i;

	// in case limit1 == limit2, it means there is no similar tick in the bar yet; we can insert the note and leave
	if (tick_limit1 == tick_limit2) {
		// move rest of song 1 note ahead (to the right)
		memmove (&song [tick_limit1 + 1], &song [tick_limit1], (song_length - tick_limit1) * sizeof (note));		// memmove to prevent memory overlapping
		// copy note in the empty space
		memcpy (&song [tick_limit1], &note, sizeof (note));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}

	//////////////
	// check instr
	//////////////
	// at this stage, note should be between tick_limit1 (first note bearing the same tick) and tick_limit2 (first note bearing the next, higher tick)
	// now we want to sort the notes by instrument, because we can!
	for (i=tick_limit1; i<tick_limit2; i++) {
		if (song [i].instrument >= note.instrument) {
			instrument_limit1 = i;		// limit1 is either the first "same" instr, or first "higher" instr in the tick
			break;
		}
	}
	// test if we reached tick boundary without success: in this case, limit1 is at the tick boundary
	if (i==tick_limit2) instrument_limit1 = i;

	for (i=tick_limit1; i<tick_limit2; i++) {
		if (song [i].instrument > note.instrument) {
			instrument_limit2 = i;		// limit2 is the first "higher" instrument of the tick
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the boundary
	if (i==bar_limit2) instrument_limit2 = i;

	// in any case, we insert the note at limit1; either it is the right instrument already, either there is no other note with the same instrument and we can insert anyway
	if (instrument_limit1 == instrument_limit1) {		// useless, but this is to keep the same structure as the previous 2 other sections
		// move rest of song 1 note ahead (to the right)
		memmove (&song [instrument_limit1 + 1], &song [instrument_limit1], (song_length - instrument_limit1) * sizeof (note));		// memmove to prevent memory overlapping
		// copy note in the empty space
		memcpy (&song [instrument_limit1], &note, sizeof (note));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}

	return;
}


// read notes from song structure, which are located between bar, tick_limit1 and bar, tick_limit2
// *out is a pointer to list of notes falling in this category
// returns number of notes in the list (0 if no notes in the list) 
int read_from_song (u_int16_t b_limit1, u_int16_t t_limit1, u_int16_t b_limit2, u_int16_t t_limit2, note_t* out) {

	int i;
	int bar_limit1, bar_limit2;
	int tick_limit1, tick_limit2;


	////////////
	// check bar
	////////////
	// go through the song to locate the right bar; stop if we found the right bar or next bar
	for (i=0; i<song_length; i++) {
		if (song [i].bar >= b_limit1) {
			bar_limit1 = i;		// limit1 is either the first "same" bar, or first "higher" bar of beginning of song part to play 
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit1 is at the end of the song: leave
	if (i==song_length) return 0;

	for (i=0; i<song_length; i++) {
		if (song [i].bar > b_limit2) {
			bar_limit2 = i;		// limit2 is the first "higher" bar of the end of the song part to play
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the end of the song
	if (i==song_length) bar_limit2 = i;

	/////////////
	// check tick
	/////////////
	// there is no need to check beat, as tick goes from 0 to max_tick in a bar, it is not reset to 0 at each beat
	// notes to play are between bar_limit1 and bar_limit2
	// go through the song (between limit1 and limit 2) to locate the right tick to play


// **************HERE************
// PROBLEME : plusieurs instruments sur le même tick !!!!


	for (i=bar_limit1; i<bar_limit2; i++) {
		if ((song [i].bar == b_limit1) && (song [i].tick >= t_limit1)) {
			tick_limit1 = i;		// limit1 is either the first "same" tick, or first "higher" tick in the bar of beginning of song part to play
			break;
		}
		if (song [i].bar > b_limit1) {
			tick_limit1 = i;		// limit1 is either the first "same" tick, or first "higher" tick in the bar of beginning of song part to play
			break;
		}
	}
	// test if we reached bar boundary without success: in this case there is nothing to play; we just leave
	if (i == bar_limit2) return 0;

	for (i=bar_limit1; i<bar_limit2; i++) {
		if ((song [i].bar == b_limit2) && (song [i].tick > t_limit2)) {
			tick_limit2 = i;		// limit2 is the first "higher" tick of the bar of ending of song part to play
			break;
		}
	}
	// test if we reached bar boundary without success: in this case, limit2 is set to the boundary value
	if (i == bar_limit2) tick_limit2 = i;

	///////////////
	// return value
	///////////////
	// the notes to be played sit between tick_limit1 (inclusive) and tick_limit2 (exclusive)
	// return pointer and length of list of notes
	out = &song [tick_limit1];
	return (tick_limit2 - tick_limit1);
}
