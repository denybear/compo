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
		case CLK:
			index = &clk_list_index;
			list = &clk_list[0][0];
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
		clock_tick = (int) ((pos->tick * 24) / (int) ticks_per_bar);		// 24 ticks per quarter note
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


// go through the whole song and quantize the notes; notes-on are quantized according to quant_noteon parameter
// duration between notes-on and notes-off are quantized according to quant_noteoff parameter
void quantize_song (int quant_noteon) {

	int i;
	note_t note;						// temp structure to store BBT info
	uint32_t tick_on, qtick_on;			// temp structure to store tick info
	uint32_t tick_off, qtick_off;		// temp structure to store tick info
	int tick_difference, qtick_difference;

	// how quantization is done:
	// first note of song is quantized according its absolute timing in the song
	// each next note is quantized according its relative timing compared to previous note
	// (said differently, time difference between the 2 consecutive notes is quantized with quant_noteon value)
	// a processing is done for note-off, to make sure a new bar never starts with note-off (useful for copy-paste). This consist in decrementing quantized value for note-offs

	if (song_length == 0) return;		// make sure song exists
	
	// quantize first note found
	note2tick (song [0], &tick_on, FALSE);			// number of ticks from BBT (0,0,0)
	qtick_on = quantize (tick_on, quant_noteon);	// quantized number of ticks
	if ((qtick_on != 0) && (song [0].status == MIDI_NOTEOFF)) qtick_on--;		// in case 1st note is note-off, then decrement so note_off is at the end of a beat; not to start of a beat
	tick2note (qtick_on, &song [0], TRUE);			// store to qBBT of the song's note
	
	// quantize the rest of the song, based on time difference with the previous note
	for (i = 1; i < song_length; i ++) {

		// determine delta time between midi event and previous midi event
		note2tick (song [(i-1)], &tick_on, FALSE);			// number of ticks from BBT (0,0,0)
		note2tick (song [(i-1)], &qtick_on, TRUE);			// number of quantized ticks from BBT (0,0,0)
		note2tick (song [i], &tick_off, FALSE);				// number of ticks from BBT (0,0,0)
		tick_difference = tick_off - tick_on;

		if (tick_difference < 0) {
			// negative time difference; this should never happen
			fprintf ( stderr, "Quantization ERROR - negative time difference between 2 consecutive notes\n");
			tick_difference = 0;
		}

		qtick_difference = quantize (tick_difference, quant_noteon);	// quantized time difference between the 2 notes-on
		qtick_off = qtick_difference + qtick_on;						// add to quantized BBT of previous note, and store to quantized BBT of current note

		// adjust quantized timing depending on whether we have notes on or notes off
		// to make sure any note off is at the end of a beat
		if (quant_noteon != FREE_TIMING) {		// adjust only if not in free timing mode
			if (qtick_on == 0) {		// specific case if qtick_on is 0; as previous notes are all aligned on 0, whether previous note is note-off or note-on
				if (song [i].status == MIDI_NOTEOFF) qtick_off--;
			}
			else {
				if ((song [(i-1)].status == MIDI_NOTEOFF) && (song [i].status == MIDI_NOTEON)) qtick_off++;
				if ((song [(i-1)].status == MIDI_NOTEON) && (song [i].status == MIDI_NOTEOFF)) qtick_off--;
			}
		}

		tick2note (qtick_off, &song [i], TRUE);		// store to quantized BBT of current note
	}	
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
// midi send is optimized (no midi command sent if no change in instruments)
// https://www.recordingblogs.com/wiki/midi-program-change-message
void set_instrument (int i, int instr) {

	uint8_t buffer [4], chan;
	
	if (instrument_list [i] != instr) {
		instrument_list [i] = instr;
		chan = instr2chan (i);
		if (chan != 9) {
			// non-drum instruments will get program change
			buffer [0] = MIDI_PC | chan;
			buffer [1] = instrument_list [i];
			buffer [2] = 0;

			push_to_list (OUT, buffer);			// put in midi send buffer to assign instruments to midi channels
		}
	}
}


// set a midi instrument to each channel by sending midi commands
// midi send is optimized (no midi command sent if no change in instruments)
void set_instruments (int *instr) {

	int i;
	
	for (i = 0; i < 8; i++) set_instrument (i, instr [i]);
}


// set volume to one midi channel
// midi send is optimized (no volume command sent if no change in instruments)
void set_volume (int i, int vol) {

	uint8_t buffer [4];

	if (volume_list [i] != vol) {
		volume_list [i] = vol;

		buffer [0] = MIDI_CC | instr2chan (i);
		buffer [1] = 0x07;					// 0x07 is Volume Control Change
		buffer [2] = vol;					// volume goes from 0 to 127

		push_to_list (OUT, buffer);			// put in midi send buffer to assign volume to midi channels
	}
}


// init volume to each midi channel
// midi send is optimized (no volume command sent if no change in instruments)
void set_volumes (int *vol) {

	int i;
	
	for (i = 0; i < 8; i++) set_volume (i, vol [i]);
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