/** @file useless.c
 *
 * @brief Contains functions that are no longer used but that I want to keep just in case.
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


// write a note to song structure; insert it to the right place
// song structure is sorted by bar, beat, tick; then by instrument
// this means the song structure is sorted every time a new note is written
// if quantized == TRUE, then writing is done according quantized notes, otherwise true timings are used
void useless_write_to_song (note_t note, int quantized) {

	int i;
	int bar_limit1, bar_limit2;
	int tick_limit1, tick_limit2;
	int qtick_limit1, qtick_limit2;
	int instrument_limit1, instrument_limit2;

	// special case if song is too large : just do nothing and leave
	if (song_length >= SONG_SIZE) return;

	// special case if song is empty; we insert note straight ahead and leave
	if (song_length == 0) {
		memcpy (&song [0], &note, sizeof (note_t));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}

	////////////
	// check bar
	////////////
	// go through the song to locate the right bar; stop if we found the right bar or next bar
	for (i=0; i<song_length; i++) {
		if ((quantized) && (song [i].qbar != 0xFFFF)) {		// get quantized value or not
			if (song [i].qbar >= note.qbar) {
				bar_limit1 = i;		// limit1 is either the first "same" bar, or first "higher" bar
				break;
			}
		}
		else {
			if (song [i].bar >= note.bar) {
				bar_limit1 = i;		// limit1 is either the first "same" bar, or first "higher" bar
				break;
			}
		}
	}
	// test if we reached song boundary without success: in this case, limit1 is at the end of the song
	if (i==song_length) bar_limit1 = i;

	for (i=0; i<song_length; i++) {
		if ((quantized) && (song [i].qbar != 0xFFFF)) {		// get quantized value or not
			if (song [i].qbar > note.qbar) {
				bar_limit2 = i;		// limit2 is the first "higher" bar
				break;
			}
		}
		else {
			if (song [i].bar > note.bar) {
				bar_limit2 = i;		// limit2 is the first "higher" bar
				break;
			}
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the end of the song
	if (i==song_length) bar_limit2 = i;

	// in case limit1 == limit2, it means there is no similar bar in the song yet; we can insert the note and leave
	if (bar_limit1 == bar_limit2) {
		// move rest of song 1 note ahead (to the right)
		memmove (&song [bar_limit1 + 1], &song [bar_limit1], (song_length - bar_limit1) * sizeof (note_t));		// memmove to prevent memory overlapping
		// copy note in the empty space
		memcpy (&song [bar_limit1], &note, sizeof (note_t));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}

	//////////////
	// check qtick
	//////////////
	// there is no need to check beat, as tick goes from 0 to max_tick in a bar, it is not reset to 0 at each beat
	// note to insert is between bar_limit1 and bar_limit2
	// go through the song (between limit1 and limit 2) to locate the right quantized tick; stop if we found the right quantized tick or next quantized tick
	for (i=bar_limit1; i<bar_limit2; i++) {
		if ((quantized) && (song [i].qtick != 0xFFFF)) {		// get quantized value or not
			if (song [i].qtick >= note.qtick) {
				qtick_limit1 = i;		// limit1 is either the first "same" tick, or first "higher" tick in the bar
				break;
			}
		}
		else {
			if (song [i].tick >= note.tick) {
				qtick_limit1 = i;		// limit1 is either the first "same" tick, or first "higher" tick in the bar
				break;
			}
		}
	}
	// test if we reached bar boundary without success: in this case, limit1 is at the bar boundary
	if (i==bar_limit2) qtick_limit1 = i;

	for (i=bar_limit1; i<bar_limit2; i++) {
		if ((quantized) && (song [i].qtick != 0xFFFF)) {		// get quantized value or not
			if (song [i].qtick > note.qtick) {
				qtick_limit2 = i;		// limit2 is the first "higher" tick of the bar
				break;
			}
		}
		else {
			if (song [i].tick > note.tick) {
				qtick_limit2 = i;		// limit2 is the first "higher" tick of the bar
				break;
			}
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the boundary
	if (i==bar_limit2) qtick_limit2 = i;

	// in case limit1 == limit2, it means there is no similar tick in the bar yet; we can insert the note and leave
	if (qtick_limit1 == qtick_limit2) {
		// move rest of song 1 note ahead (to the right)
		memmove (&song [qtick_limit1 + 1], &song [qtick_limit1], (song_length - qtick_limit1) * sizeof (note_t));		// memmove to prevent memory overlapping
		// copy note in the empty space
		memcpy (&song [qtick_limit1], &note, sizeof (note_t));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}


	//////////////
	// check instr
	//////////////
	// at this stage, note should be between tick_limit1 (first note bearing the same tick) and tick_limit2 (first note bearing the next, higher tick)
	// now we want to sort the notes by instrument, because we can!
	for (i=qtick_limit1; i<qtick_limit2; i++) {
		if (song [i].instrument >= note.instrument) {
			instrument_limit1 = i;		// limit1 is either the first "same" instr, or first "higher" instr in the tick
			break;
		}
	}
	// test if we reached tick boundary without success: in this case, limit1 is at the tick boundary
	if (i==qtick_limit2) instrument_limit1 = i;

	for (i=qtick_limit1; i<qtick_limit2; i++) {
		if (song [i].instrument > note.instrument) {
			instrument_limit2 = i;		// limit2 is the first "higher" instrument of the tick
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the boundary
	if (i==qtick_limit2) instrument_limit2 = i;

	// in any case, we insert the note at limit1; either it is the right instrument already, either there is no other note with the same instrument and we can insert anyway
	if (instrument_limit1 == instrument_limit1) {		// useless, but this is to keep the same structure as the previous 2 other sections
		// move rest of song 1 note ahead (to the right)
		memmove (&song [instrument_limit1 + 1], &song [instrument_limit1], (song_length - instrument_limit1) * sizeof (note_t));		// memmove to prevent memory overlapping
		// copy note in the empty space
		memcpy (&song [instrument_limit1], &note, sizeof (note_t));		// memcpy is fine as no overlapping in memory
		song_length++;
		return;
	}

	return;
}



// go through the whole song and quantize the notes; notes-on are quantized according to quant_noteon parameter
// duration between notes-on and notes-off are quantized according to quant_noteoff parameter
void useless_quantize_song_complex (int quant_noteon) {

	int i, j, prev;
	note_t note;						// temp structure to store BBT info
	uint32_t tick_on, qtick_on;			// temp structure to store tick info
	uint32_t tick_off, qtick_off;		// temp structure to store tick info
	int tick_difference, qtick_difference;

	if (song_length == 0) return;		// make sure song exists

	// how quantization is done:
	// first note-on of each instrument in the song is quantized according its absolute timing in the song
	// each next note-on of the same instrument is quantized according its relative timing compared to previous note-on
	// (said differently, time difference between the 2 consecutive notes is quantized with quant_noteon value)
	// then, song is scanned again to find the corresponding note-off to a note-on
	// Note-off is quantized according its relative timing compared to corresponding note-on (ie. time difference)
	// another processing is done for note-off, to make sure a new bar never starts with note-off (useful for copy-paste). This consist in decrementing quantized value for note-offs

	// go instrument by instrument
	for (j = 0; j <  8; j++) {
		// search for first note
		for (i = 0; i < song_length; i ++) {
			if ((song [i].instrument == j) && (song [i].status == MIDI_NOTEON)) {
				// quantize first note on
				note2tick (song [i], &tick_on, FALSE);			// number of ticks from BBT (0,0,0)
				qtick_on = quantize (tick_on, quant_noteon);	// quantized number of ticks
				tick2note (qtick_on, &song [i], TRUE);			// store to qBBT of the song's note
				break;		// leave loop
			}
		}
		if (i == song_length) continue;		// no notes found for this instrument: next loop

		// quantize the rest of the song (notes-on), based on time difference with the previous note-on
		prev = i;
		i++;
		while ( i < song_length) {
			if ((song [i].instrument == j) && (song [i].status == MIDI_NOTEON)) {
				// next note-on found !
				// determine delta time between midi event and previous midi event
				note2tick (song [prev], &tick_on, FALSE);			// number of ticks from BBT (0,0,0)
				note2tick (song [prev], &qtick_on, TRUE);			// number of quantized ticks from BBT (0,0,0)
				note2tick (song [i], &tick_off, FALSE);				// number of ticks from BBT (0,0,0)
				tick_difference = tick_off - tick_on;

				if (tick_difference < 0) {
					// negative time difference; this should never happen
					fprintf ( stderr, "Quantization ERROR - negative time difference between 2 consecutive notes-on\n");
					tick_difference = 0;
				}

				qtick_difference = quantize (tick_difference, quant_noteon);	// quantized time difference between the 2 notes-on
				qtick_off = qtick_difference + qtick_on;						// add to quantized BBT of previous note, and store to quantized BBT of current note
				tick2note (qtick_off, &song [i], TRUE);							// store to quantized BBT of current note
				prev = i;
			}
			i++;
		}
	}

	// quantize the rest of the song (notes-off), based on time difference with the corresponding note-on
	// search for note-on, then corresponding note-off
	for (i = 0; i < song_length; i ++) {
		if (song [i].status == MIDI_NOTEON) {		// note-on found
			for (j = i; j < song_length; j++) {		// go through rest of the song to find the first note-off
				if ((song [j].key == song [i].key) && (song [j].instrument == song [i].instrument) && (song [j].status == MIDI_NOTEOFF)) {
					// corresponding note-off found !
					// determine delta time between midi event and previous midi event
					note2tick (song [i], &tick_on, FALSE);			// number of ticks from BBT (0,0,0)
					note2tick (song [i], &qtick_on, TRUE);			// number of quantized ticks from BBT (0,0,0)
					note2tick (song [j], &tick_off, FALSE);			// number of ticks from BBT (0,0,0)
					tick_difference = tick_off - tick_on;

					if (tick_difference < 0) {
						// negative time difference; this should never happen
						fprintf ( stderr, "Quantization ERROR - negative time difference between note-on & note-off\n");
						tick_difference = 0;
					}

					qtick_difference = quantize (tick_difference, quant_noteon);	// quantized time difference between the 2 notes-on
					qtick_off = qtick_difference + qtick_on;						// add to quantized BBT of previous note, and store to quantized BBT of current note
					qtick_off--;													// adjust qtick_off so it does not start with a new bar
					tick2note (qtick_off, &song [j], TRUE);							// store to quantized BBT of current note
					break;		// leave loop : next note-on
				}
			}
		}
	}
}


// go through the whole song and quantize the notes; notes-on are quantized according to quant_noteon parameter
// duration between notes-on and notes-off are quantized according to quant_noteoff parameter
void useless_quantize_song_simple (int quant_noteon) {

	int i, j, prev;
	note_t note;						// temp structure to store BBT info
	uint32_t tick_on, qtick_on;			// temp structure to store tick info
	uint32_t tick_off, qtick_off;		// temp structure to store tick info
	int tick_difference, qtick_difference;

	if (song_length == 0) return;		// make sure song exists

	// how quantization is done:
	// first note of song is quantized according its absolute timing in the song
	// each next note is quantized according its relative timing compared to previous note
	// (said differently, time difference between the 2 consecutive notes is quantized with quant_noteon value)
	// a processing is done for note-off, to make sure a new bar never starts with note-off (useful for copy-paste). This consist in decrementing quantized value for note-offs
	
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


