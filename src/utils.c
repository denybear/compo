/** @file utils.c
 *
 * @brief Contains all-purpose functions used by other parts of the program.
 *
 */

#include "types.h"
#include "globals.h"
#include "process.h"
#include "utils.h"
#include "led.h"
#include "song.h"
#include "disk.h"
#include "useless.h"


// convert pad midi number to bar number : ie 0x00-0x77 to 0-63
uint8_t midi2bar (uint8_t midi) {

	uint8_t bar;

	bar = (((midi & 0xF0) >> 4) * 8 ) + (midi & 0x0F);
	return (bar);
}


// convert bar number to pad midi number : ie 0-63 to 0x00-0x77
uint8_t bar2midi (uint8_t bar) {

	uint8_t midi;

	midi = ((bar >> 3) << 4) + (bar & 0x7);
	return (midi);
}


// convert song instrument to midi channel
uint8_t instr2chan (uint8_t instr, int mode) {

	const uint8_t midi_export [8] = {9, 0, 1, 2, 3, 4, 5, 6};		// 1st instrument is drum channel
	const uint8_t fluidsynth [8] = {9, 0, 1, 2, 3, 4, 5, 6};		// 1st instrument is drum channel
	const uint8_t qsynth [8] = {8, 0, 2, 4, 6, 10, 12, 14};			// 1st instrument is drum channel; make sure channel 8 is configured as drum in qsynth
	
	if (mode == QSYNTH)	return (qsynth [instr]);
	if (mode == FLUIDSYNTH)	return (fluidsynth [instr]);
	return (midi_export [instr]);									// midi export and the rest
}


// returns TRUE is instrument is drum, FALSE if not; depending on the channel mode
int is_drum (uint8_t instr, int mode) {

	if (mode == QSYNTH) {											// specific case for qsynth
		if (instr2chan (instr, mode) == 8) return TRUE;
		else return FALSE;
	}

	if (instr2chan (instr, mode) == 9) return TRUE;					// all the other midi modes
	else return FALSE;
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
		case CLK:
			index = &clk_list_index;
			list = &clk_list[0][0];
			break;
		case KBD_CLK:
			index = &kbd_clk_list_index;
			list = &kbd_clk_list[0][0];
			break;
	}

	for (i = 0; i < 3; i++) {
		// add to the list
		list [((*index) * 3) + i] = buffer [i];
	}
	// increment index and check boundaries
	if (*index >= (LIST_ELT-1)) *index = 0;
	else (*index)++;
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
		case CLK:
			index = &clk_list_index;
			list = &clk_list[0][0];
			break;
		case KBD_CLK:
			index = &kbd_clk_list_index;
			list = &kbd_clk_list[0][0];
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


// determines the length of midi bytes to send, and call jack_midi_event_write() to send them
int midi_write (void *port_buffer, jack_nframes_t time, jack_midi_data_t *buffer) {

	int lg;		// length of data to be sent

	// if buffer is not empty, then send as midi out event
	if (buffer [0] | buffer [1] | buffer [2]) {
		switch (buffer [0] & 0xF0) {
			case MIDI_PC:
				lg = 2;
				break;
			case MIDI_1BYTE:
				lg = 1;
				break;
			default:
				lg = 3;
				break;
		}
		// send to midi
		return (jack_midi_event_write (port_buffer, time, buffer, lg));
	}

	return 0;
}


// compute BBT based on nframes, tempo (BPM), frame rate, etc.
// based on new_pos value:
// 0 : compute BBT based on previous values of BBT & fram rate
// 1 : compute BBT, and sets position as ui_current_bar, 0, 0
// requires jack_position_t * which will contain the BBT information
// returns TRUE if a clock signal shall be sent, FALSE otherwise
int compute_bbt (jack_nframes_t nframes, jack_position_t *pos, int new_pos)
{
	double ticks_per_bar;		// number of ticks per bar
	uint32_t start_bar;
	static int clock_tick, previous_clock_tick;		// number of ticks for midi clock (from 0-23 for quarter note, 0-95 per bar)
	if (new_pos) {

		pos->frame_rate = jack_get_sample_rate(client);			// set frame rate (sample rate) to the BBT structure
		pos->valid = JackPositionBBT;
		pos->beats_per_bar = time_beats_per_bar;
		pos->beat_type = time_beat_type;
		pos->ticks_per_beat = time_ticks_per_beat;
		pos->beats_per_minute = (int) (time_beats_per_minute * time_bpm_multiplier);

		// set BBT to bar,0,0; this is in the case of "play"
		pos->bar = (ui_current_page * 64) + ui_current_bar;
		pos->padding[0] = pos->bar;		// save starting bar in padding 
		pos->beat = 0;
		pos->tick = 0;
		pos->bar_start_tick = 0.0;

		// for sending midi clock
		clock_tick = 0;
		previous_clock_tick = -1;
		return (FALSE);					// clock signal will be sent at next call
	}
	else {

		// Compute BBT info based on previous period.
		// (1) pos->ticks_per_beat * pos->beats_per_minute : number of ticks per minutes
		// (2) (pos->frame_rate * 60) / nframes : number of frames per minute
		// (1)/(2) = number of ticks per frame; (1)/(2) is equal to (1) * inv(2) 
		pos->bar_start_tick += (pos->ticks_per_beat * pos->beats_per_minute * nframes / (pos->frame_rate * 60));

		// computes BTT, based on float bar_start_tick (number of ticks since play is pressed)
		ticks_per_bar = pos->beats_per_bar * pos->ticks_per_beat;
		pos->tick = (int) pos->bar_start_tick % (int) ticks_per_bar;	// tick number within the bar
		pos->beat = pos->tick / (int) time_ticks_per_beat;				// beat number within the bar
		pos->bar = pos->padding [0] + ((int) pos->bar_start_tick / (int) ticks_per_bar);		// bar number
		// check bar boundaries; go to bar 0 if we reach last bar
		if (pos->bar >= 512) pos->bar = pos->bar % 512;		// 512 = 64 bar * 8 pages; we loop after 512 bars

		// determine if clock signal shall be sent
		clock_tick = (int) ((pos->tick * 24 * 4) / (int) ticks_per_bar);		// 24 ticks per quarter note (hence 24 * 4)
		if (clock_tick == previous_clock_tick) return (FALSE);
		previous_clock_tick = clock_tick;
		return (TRUE);
	}
}


// quantize a tick to the nearest value; tick could be of any value
uint32_t quantize (uint32_t tick, int quant) {
	int i;
	int step;			// number of ticks per "quantized" step
	uint32_t div, rem;	// temporary variables for calculation

	if (quant == FREE_TIMING) return tick;	// no quantization required, leave

	step = (int) (time_ticks_per_beat / quant);		// step is a number of ticks per each "quantized" step
	div = tick / step;
	rem = tick % step;		// remaining of integer division: we are going to adjust this

	// adjust remaining to match with quantized value
	if (rem < (int) (step / 2)) rem = 0;
	else rem = step;

	// send quantized value of tick
	return ((div * step) + rem);
}


// returns the smallest possible number of ticks for a value of quantizer
uint32_t min_time (int quant) {
	if ((quant == 0) || (quant == FREE_TIMING)) return 1;
	return (int)(time_ticks_per_beat / quant);
}


// quantize note, based on other notes that are in the song already
// returns TRUE to indicate if the note shall be played straight ahead (ie. note has been quantized "in the past") or FALSE to indicate it shall be played later on
// 2 quantizer values can be provided: one for notes on, one for notes off
int quantize_note (int quant_noteon, int quant_noteoff, note_t *note) {

	int i, found;
	uint32_t tick_on, qtick_on;			// temp structure to store tick info
	uint32_t tick_off, qtick_off;		// temp structure to store tick info
	int tick_difference, qtick_difference;

	// go through the whole song backwards and check whether there is another note with the same instrument
	i = song_length;
	found = FALSE;
	while (i > 0) {
		i--;
		if (((note->status == MIDI_NOTEON) && (song[i].instrument == note->instrument) && (song[i].status == MIDI_NOTEON)) || 
			((note->status == MIDI_NOTEOFF) && (song[i].instrument == note->instrument) && (song[i].status == MIDI_NOTEON) && (song[i].key == note->key))){
			// another note (with same instrument) has been found; leave loop
			found = TRUE;
			break;
		}
	}

	if (found) {
		// a previous note-on has been found with the same instrument, and its index is i; quantize the time difference between the 2 notes
		note2tick (song [i], &tick_on, FALSE);			// number of ticks from BBT (0,0,0)
		note2tick (song [i], &qtick_on, TRUE);			// number of quantized ticks from BBT (0,0,0)
		note2tick (*note, &tick_off, FALSE);			// number of ticks from BBT (0,0,0)
		tick_difference = tick_off - qtick_on;

		if (tick_difference < 0) {
			// negative time difference; align qtick of new note with the qtick of previous note
			tick_difference = 0;
		}

		if (note->status == MIDI_NOTEON) {
			qtick_difference = quantize (tick_difference, quant_noteon);				// quantized time difference between the 2 notes-on
		}
		else {																			// note-off
			qtick_difference = quantize (tick_difference, quant_noteoff);				// quantized time difference between the note-on and note-off
			if (qtick_difference == 0) qtick_difference = min_time (quant_noteoff);		// no timing difference between note-off and previous note-on: add the smallest time difference possible, based on quantizer
			qtick_difference--; 														// decrement qtick difference to avoid note-off to be on a new beat/bar
		}
		qtick_off = qtick_difference + qtick_on;										// add to quantized BBT of previous note, and store to quantized BBT of current note
		tick2note (qtick_off, note, TRUE);												// store to quantized BBT of current note
	}
	else {
		// no previous note-on has been found with the same instrument; just simple quantize the note
		note2tick (*note, &tick_off, FALSE);												// number of ticks from BBT (0,0,0)
		if (note->status == MIDI_NOTEON) qtick_off = quantize (tick_off, quant_noteon);		// quantized notes-on
		else qtick_off = quantize (tick_off, quant_noteoff);								// quantized note-off
		tick2note (qtick_off, note, TRUE);													// store to qBBT of the song's note
	}

	// test if quantized note is to be played in the past or in the future
	if (qtick_off <= tick_off) return TRUE;			// note should have been played in the past: we play it straight
	return FALSE;									// play note later
}


// convert BBT values of a note to number of ticks from BBT (0,0,0)
// quantized indicates whether BBT info is in BBT structure or qBBT
void note2tick (note_t note, uint32_t *tick, int quantized) {

	if (quantized) *tick = ((note.qbar * (int) (time_ticks_per_beat * time_beats_per_bar)) + note.qtick);
	else *tick = ((note.bar * (int) (time_ticks_per_beat * time_beats_per_bar)) + note.tick);
}


// convert number of ticks from BBT (0,0,0) to BBT values of a note 
// quantized indicates whether BBT info is in BBT structure or qBBT
void tick2note (uint32_t tick, note_t *note, int quantized) {

	int ticks_per_bar;

	ticks_per_bar = (int) (time_ticks_per_beat * time_beats_per_bar);

	if (quantized) {
		note->qbar = tick / ticks_per_bar;
		note->qtick = tick % ticks_per_bar;
		note->qbeat = note->qtick / (int) (time_ticks_per_beat);
	}
	else {
		note->bar = tick / ticks_per_bar;
		note->tick = tick % ticks_per_bar;
		note->beat = note->tick / (int) (time_ticks_per_beat);
	}
}


// set a midi instrument to one channel by sending midi commands
// https://www.recordingblogs.com/wiki/midi-program-change-message
void set_instrument (int i, int instr) {

	uint8_t buffer [4], chan;
	
	chan = instr2chan (i, midi_mode);
	if (is_drum (i, midi_mode)) return;			// no instrument set for drum channel

	buffer [0] = MIDI_PC | chan;
	buffer [1] = instr;
	buffer [2] = 0;

	push_to_list (OUT, buffer);		// put in midi send buffer to assign instruments to midi channels
}


// set a midi instrument to each channel by sending midi commands, according to instruments list
void set_instruments () {

	int i;
	
	for (i = 0; i < 8; i++) set_instrument (i, instrument_list [i]);
}


// set volume to one midi channel
void set_volume (int i, int vol) {

	uint8_t buffer [4];

	buffer [0] = MIDI_CC | instr2chan (i, midi_mode);
	buffer [1] = 0x07;					// 0x07 is Volume Control Change
	buffer [2] = vol;					// volume goes from 0 to 127

	push_to_list (OUT, buffer);			// put in midi send buffer to assign volume to midi channels
}


// init volume to each midi channel, according to volume list
void set_volumes () {

	int i;
	
	for (i = 0; i < 8; i++) set_volume (i, volume_list [i]);
}


// based on instrument number, and on the status of the other instruments (muted, solo), indicates whether the instrument should be played or not
int should_play (int instr) {

	int i;

	// muted
	if (ui_instruments [instr] == BLACK) return FALSE;
	if (ui_instruments [instr] == LO_BLACK) return FALSE;

	// solo
	if (ui_instruments [instr] == HI_RED) return TRUE;
	if (ui_instruments [instr] == LO_RED) return TRUE;

	// non-defined state (green or yellow or amber); we should look at the other instruments to check whether one of them is solo
	for (i = 0; i < 8; i++) {
		if (ui_instruments [i] == HI_RED) return FALSE;
		if (ui_instruments [i] == LO_RED) return FALSE;
	}

	// if no other instrument is solo, then we should play
	return TRUE;
}