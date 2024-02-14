/** @file song.c
 *
 * @brief Contains functions used to add data (notes) to the song structure.
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
void write_to_song (note_t note) {

	int i;
	int note_bar, note_tick, note_status, song_bar, song_tick, song_status;

	// special case if song is too large : just do nothing and leave
	if (song_length >= SONG_SIZE) return;

	// fill temp variables (for easier reading)
	note_bar = note.qbar;
	note_tick = note.qtick;
	note_status = note.status;

	for (i = 0; i < song_length; i++) {

		// fill temp variables (for easier reading)
		song_bar = song [i].qbar;
		song_tick = song [i].qtick;
		song_status = song [i].status;

		// check the bar first
		if (song_bar < note_bar) continue;		// if bar in song is too small, then next loop
		if (song_bar > note_bar) break;			// if bar in song is too high, then leave

		// note bar == song bar
		// check the tick
		if (song_tick < note_tick) continue;	// if tick in song is too small, then next loop
		if (song_tick > note_tick) break;		// if tick in song is too high, then leave

		// note tick == song tick
		// check the status
		// we want to sort this way: NOTE_ON (0x90), then NOTE_OFF (0x80)
		if (song_status > note_status) continue;	// if note status is higher (ie. note on found when you have note off to insert) then next loop
		break;										// leave
	}

	// i is the index where the note shall be inserted in the song
	// move rest of song 1 note ahead (to the right)
	if (song_length > 0) memmove (&song [i + 1], &song [i], (song_length - i) * sizeof (note_t));	// memmove to prevent memory overlapping
	// copy note in the empty space
	memcpy (&song [i], &note, sizeof (note_t));									// memcpy is fine as no overlapping in memory
	song_length++;
}


// read notes from song structure, which are located between bar, tick_limit1 (inclusive) and bar, tick_limit2 (exclusive)
// returns a pointer to list of notes falling in this category, NULL if nothing
// returns also the number of notes in the list (0 if no notes in the list)
note_t * read_from_song (u_int16_t b_limit1, u_int16_t t_limit1, u_int16_t b_limit2, u_int16_t t_limit2, int *length) {

	return (read_from (song, song_length, b_limit1, t_limit1, b_limit2, t_limit2, length));
}


// read notes from metronome structure, which are located between bar, tick_limit1 (inclusive) and bar, tick_limit2 (exclusive)
// returns a pointer to list of notes falling in this category, NULL if nothing
// returns also the number of notes in the list (0 if no notes in the list) 
note_t * read_from_metronome (u_int16_t b_limit1, u_int16_t t_limit1, u_int16_t b_limit2, u_int16_t t_limit2, int *length) {

	// test that we have not reached song boundaries, otherwise loop
	if ((b_limit1 == 511) && (b_limit2 == 0)) {
		b_limit1 = 0;
		b_limit2 = 1;
	}
	
	// test that we don't want more than 1 bar of metronome
	if (b_limit2 - b_limit1 > 1) {
		*length = 0;
		return NULL;
	}

	return (read_from (metronome, 16, 0, t_limit1, (b_limit2 - b_limit1), t_limit2, length));
}


// read notes from any song structure (main song or metronome), which are located between bar, tick_limit1 (inclusive) and bar, tick_limit2 (exclusive)
// returns a pointer to list of notes falling in this category, NULL if nothing
// returns also the number of notes in the list (0 if no notes in the list) 
note_t * read_from (note_t* sg, int lg, u_int16_t b_limit1, u_int16_t t_limit1, u_int16_t b_limit2, u_int16_t t_limit2, int *length) {

	int i;
	int bar_limit1, bar_limit2;
	int tick_limit1, tick_limit2;
	int bar, tick;		// temp values holding BBT for the note


	////////////
	// check bar
	////////////
	// go through the song to locate the right bar; stop if we found the right bar or next bar
	for (i=0; i<lg; i++) {
		bar = sg [i].qbar;

		if (bar >= b_limit1) {
			bar_limit1 = i;		// limit1 is either the first "same" bar, or first "higher" bar of beginning of song part to play 
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit1 is at the end of the song: leave
	if (i==lg) {
		*length = 0;
		return NULL;
	}

	for (i=0; i<lg; i++) {
		bar = sg [i].qbar;
		
		if (bar > b_limit2) {
			bar_limit2 = i;		// limit2 is the first "higher" bar of the end of the song part to play
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the end of the song
	if (i==lg) bar_limit2 = i;

	// test if bar_limit1 and bar_limit2 are the same; in which case there is nothing to read, and we can leave
	if (bar_limit1 == bar_limit2) {
		*length = 0;
		return NULL;
	}	


	/////////////
	// check tick
	/////////////
	// there is no need to check beat, as tick goes from 0 to max_tick in a bar, it is not reset to 0 at each beat
	// notes to play are between bar_limit1 and bar_limit2
	// go through the song (between limit1 and limit 2) to locate the right tick to play
	for (i=bar_limit1; i<bar_limit2; i++) {
		bar = sg [i].qbar;
		tick = sg [i].qtick;

		if ((bar == b_limit1) && (tick >= t_limit1)) {
			tick_limit1 = i;		// limit1 is either the first "same" tick, or first "higher" tick in the bar of beginning of song part to play
			break;
		}
		if (bar > b_limit1) {
			tick_limit1 = i;		// limit1 is either the first "same" tick, or first "higher" tick in the bar of beginning of song part to play
			break;
		}
	}
	// test if we reached bar boundary without success: in this case there is nothing to play; we just leave
	if (i == bar_limit2) {
		*length = 0;
		return NULL;
	}

	for (i=bar_limit1; i<bar_limit2; i++) {
		bar = sg [i].qbar;
		tick = sg [i].qtick;

		if ((bar == b_limit2) && (tick >= t_limit2)) {
		// if you change condition with ((bar == b_limit2) && (tick > t_limit2)), then tick_limit2 is inclusive, not exclusive
			tick_limit2 = i;		// limit2 is the first "higher" tick of the bar of ending of song part to play
			break;
		}
	}
	// test if we reached bar boundary without success: in this case, limit2 is set to the boundary value
	if (i == bar_limit2) tick_limit2 = i;

	// test if tick_limit1 and tick_limit2 are the same; in which case there is nothing to read, and we can leave
	if (tick_limit1 == tick_limit2) {
		*length = 0;
		return NULL;
	}

	///////////////
	// return value
	///////////////
	// the notes to be played sit between tick_limit1 (inclusive) and tick_limit2 (exclusive)
	// return pointer and length of list of notes
	*length = tick_limit2 - tick_limit1;
	return (&sg [tick_limit1]);
}


// for debug use only
void test_copy_paste () {

	test_write ();


	copy (0, 1, 1);
	display_song (copy_length, copy_buffer, "test: copy area that does not exist");

	copy (3, 4, 1);
	display_song (copy_length, copy_buffer, "test: copy 1st bar that exists but instr does not exist");

	copy (3, 4, 2);
	display_song (copy_length, copy_buffer, "test: copy 1st bar that exists and instr does exist");

	copy (4, 5, 7);
	display_song (copy_length, copy_buffer, "test: copy bar that exists and instr does exist");

	copy (4, 5, 2);
	display_song (copy_length, copy_buffer, "test: copy 2 bars in the middle of song");

	copy (6, 7, 2);
	display_song (copy_length, copy_buffer, "test: copy 2 bars at end of song");



	cut (0, 1, 1);
	display_song (song_length, song, "0, 1, 1 test: cut area that does not exist");
	paste (10, 7, PASTE);
	display_song (song_length, song, "10, 7 test: try to paste bar 10, instr 7");

	cut (3, 4, 1);
	display_song (song_length, song, "3, 4, 1 test: cut 1st bar that exists but instr does not exist");
	paste (10, 7, PASTE);
	display_song (song_length, song, "10, 7 test: try to paste bar 10, instr 7");

	cut (3, 4, 2);
	display_song (song_length, song, "3, 4, 2 test: cut 1st bar that exists and instr does exist");
	paste (10, 1, PASTE);
	display_song (song_length, song, "10, 1 test: paste bar 10, instr 7");

	cut (4, 5, 7);
	display_song (song_length, song, "4, 5, 7 test: cut bar that exists and instr does exist");
	paste (4, 7, PASTE);
	display_song (song_length, song, "4, 7 test: paste same place, same instr");

	cut (4, 5, 2);
	display_song (song_length, song, "4, 5, 2 test: cut 2 bars in the middle of song");
	paste (2, 0, PASTE);
	display_song (song_length, song, "2, 0 test: paste 1st bar of song");

	cut (6, 7, 2);
	display_song (song_length, song, "6, 7, 2 test: cut 2 bars near end of song");
	paste (5, 1, PASTE);
	display_song (song_length, song, "5, 1 test: paste in middle of song");

	cut (5, 20, 1);
	display_song (song_length, song, "5, 20, 1 test: cut 3 bars mid of song");
	display_song (copy_length, copy_buffer, "test: content of copy buffer");
	paste (30, 2, PASTE);
	display_song (song_length, song, "30, 2 test: paste at end of song");

	cut (29, 31, 2);
	display_song (song_length, song, "29, 31, 2 test: cut 3 bars near end of song");
	display_song (copy_length, copy_buffer, "test: content of copy buffer");
	paste (0, 6, PASTE);
	display_song (song_length, song, "0, 6 test: paste at start of song, bar shall be 1");
}


// for debug use only
void test_write () {

	note_t note;

	note.bar =5;
	note.beat =1;
	note.tick =480;
	note.qbar =5;
	note.qbeat =1;
	note.qtick =480;
	note.instrument =1;
	write_to_song (note);
	display_song (song_length, song, "test: insert 1st note in song");

	note.bar =6;
	note.beat =0;
	note.tick =0;
	note.qbar =6;
	note.qbeat =0;
	note.qtick =0;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note at end of song");

	note.bar =3;
	note.beat =2;
	note.tick =960;
	note.qbar =3;
	note.qbeat =2;
	note.qtick =960;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note at beginning of song");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.qbar =4;
	note.qbeat =2;
	note.qtick =960;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note between 2 bars");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.qbar =4;
	note.qbeat =2;
	note.qtick =960;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note same as previous");

	note.bar =4;
	note.beat =2;
	note.tick =961;
	note.qbar =4;
	note.qbeat =2;
	note.qtick =961;
	note.instrument =7;
	write_to_song (note);
	display_song (song_length, song, "insert note same bar, higher tick");

	note.bar =4;
	note.beat =1;
	note.tick =959;
	note.qbar =4;
	note.qbeat =1;
	note.qtick =959;
	note.instrument =7;
	write_to_song (note);
	display_song (song_length, song, "insert note same bar, lower tick");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.qbar =4;
	note.qbeat =2;
	note.qtick =960;
	note.instrument =3;
	write_to_song (note);
	display_song (song_length, song, "insert note same as previous, with higher instr");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.qbar =4;
	note.qbeat =2;
	note.qtick =960;
	note.instrument =0;
	write_to_song (note);
	display_song (song_length, song, "insert note same as previous, with lower instr");

	note.bar =6;
	note.beat =3;
	note.tick =1440;
	note.qbar =6;
	note.qbeat =3;
	note.qtick =1440;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note at end of song");

}


// for debug use only
void test_read () {

	note_t *note;
	int length;

	note = read_from_song (1, 0, 1, 480, &length);
	display_song (length, note, "read note that does not exist (before 1st bar)");
	note = read_from_song (7, 0, 7, 480, &length);
	display_song (length, note, "read note that does not exist (after last bar)");
	note = read_from_song (3, 0, 3, 960, &length);
	display_song (length, note, "read note that does not exist in the middle of song");
	note = read_from_song (3, 0, 3, 961, &length);
	display_song (length, note, "read 1 note");
	note = read_from_song (3, 960, 4, 960, &length);
	display_song (length, note, "read 2 notes");
	note = read_from_song (3, 0, 6, 1500, &length);
	display_song (length, note, "read all notes of song");
	note = read_from_song (4, 0, 4, 1260, &length);
	display_song (length, note, "read bar 4 only");
	note = read_from_song (4, 960, 4, 960, &length);
	display_song (length, note, "read bar 4, tick 960 only; no result as end limit is exclusive");
	note = read_from_song (4, 960, 4, 961, &length);
	display_song (length, note, "read bar 4, tick 960 only");
	note = read_from_song (6, 1450, 6, 1900, &length);
	display_song (length, note, "read note that does not exist, at end of song");
	note = read_from_song (6, 1450, 20, 1900, &length);
	display_song (length, note, "read note that does not exist, at end of song (and bar does not even exist)");
}


// for debug use only
void display_song (int lg, note_t *sg, char * st) {
	int i;

	printf ("out: %x, Length:%d - Test:%s\n", sg, lg, st);
	for (i=0; i<lg; i++) {
		printf ("index:%d, bar:%d, beat:%d, tick:%d, qbar:%d, qbeat:%d, qtick:%d, instr:%d\n", i, sg[i].bar, sg[i].beat, sg[i].tick, sg[i].qbar, sg[i].qbeat, sg[i].qtick, sg[i].instrument);
	}
	printf ("\n");
}


// copy & cut bar functionality. It copies from b_limit1 (inclusive) to b_limit2 (exclusive).
// it copies only the notes from a single instrument, and stores this into a copy-paste buffer
// at the end of the process, notes are copied to copy buffer, and copy_length is set to proper copy buffer length
// corresponding notes in the song are erased from the song
// depending on a parameter, the function can either copy only, copy and erase (cut), erase only
void copy_cut (u_int16_t b_limit1, u_int16_t b_limit2, int instr, int mode) {

	note_t *note;	// pointer to notes in the song to be copied
	int lg;			// number of notes copied from song
	int i;

	// get all the bars from the song which are between b_limit1 (inclusive) and b_limit2 (exclusive) 
	note = read_from_song (b_limit1, 0, b_limit2, 0, &lg);

	if ((mode == COPY) || (mode == CUT)) copy_length = 0;		// reset copy buffer only for copy or cut
	for (i=0; i<lg; i++) {
		// go through the notes and determine whether these have the right instrument and should be copied
		if (note [i].instrument == instr) {
			if ((mode == COPY) || (mode == CUT)) {
				// copy the note to the copy buffer
				memcpy (&copy_buffer [copy_length], &note [i], sizeof (note_t));		// no issue in using memcpy as memory should not overlap
				// correct the bar number in copy buffer so bar_limit1 is now 0
				copy_buffer [copy_length].qbar = copy_buffer [copy_length].qbar - b_limit1;
				copy_length++;
			}

			if ((mode == CUT) || (mode == DEL)) {
				// set 0xFFFF in bar number of note, so we can erase it afterwards
				note [i].bar = 0xFFFF;
			}
		}
	}

	if (mode == COPY) return;		// leave in case of simple copy operation

	// here, the mode is CUT or DEL: we delete the notes in the song
	// go through the whole song to remove cut notes from the song
	for (i=0; i<song_length; i++) {
		if (song [i].bar == 0xFFFF) {
			// note shall be removed
			// overwrite current note with rest of song

			// check if rest of song exists before doing this
			if (i < (song_length - 1)) memmove (&song [i], &song [i + 1], (song_length - (i + 1)) * sizeof (note_t));
			song_length--;		// song has one note less
			i--;				// required as we need to parse same note on next loop
		}
	}

	// clear the rest of song space, to remove crap
	memset (&song [song_length], 0, (SONG_SIZE - song_length) * sizeof (note_t));

	// set the ui leds according to removed bars
	// no, we will do this in the main process instead
}


// copy bar functionality. It copies from b_limit1 (inclusive) to b_limit2 (exclusive).
// it copies only the notes from a single instrument, and stores this into a copy-paste buffer
// at the end of the process, notes are copied to copy buffer, and copy_length is set to proper copy buffer length
void copy (u_int16_t b_limit1, u_int16_t b_limit2, int instr) {

	copy_cut (b_limit1, b_limit2, instr, COPY);
}


// cut bar functionality. It copies from b_limit1 (inclusive) to b_limit2 (exclusive).
// it copies only the notes from a single instrument, and stores this into a copy-paste buffer
// at the end of the process, notes are copied to copy buffer, and copy_length is set to proper copy buffer length
// corresponding notes are erased from the song
void cut (u_int16_t b_limit1, u_int16_t b_limit2, int instr) {

	copy_cut (b_limit1, b_limit2, instr, CUT);
}


// paste bar functionality. It copies the full copy buffer to b_limit1 (inclusive).
// at the end of the process, copy_buffer is not emptied
// if mode == PASTE, then destination area is cleared before paste
// if mode == OVERDUB, then we write on top of destination area (without clearing)
void paste (u_int16_t b_limit1, int instr, int mode) {

	note_t note;		// temp note
	int i;
	uint16_t b_limit2;	// exclusive limit where to stop erasing

	// make sure copy_buffer has some content; if not, then leave
	// this is required to avoid issue with the erasing/deletion of bars
	if (copy_length == 0) return;

	if (mode != OVERDUB) {
		// erase the content of bars before pasting new stuff: we don't do overdubbing when pasting
		b_limit2 = (copy_buffer [copy_length - 1].qbar) + 1 + b_limit1;		// b_limit2 is last bar in copy buffer + 1 (as it is exclusive)
		copy_cut (b_limit1, b_limit2, instr, DEL);				// erase corresponding bars in the song
	}

	for (i=0; i<copy_length; i++) {
		// take note from copy_buffer and store it in a temporary space
		memcpy (&note, &copy_buffer [i], sizeof (note_t));
		// correct bar so it matches to destination bar, correct instrument to match destination instrument
		note.bar += b_limit1;
		note.qbar += b_limit1;
		note.instrument = instr;
		// add to song
		write_to_song (note);
	}
}


// create metromone table
void create_metronome () {
	
	note_t note;
	double half_time_ticks_per_beat;
	int i;
	
	// time for triggering note-off
	half_time_ticks_per_beat = time_ticks_per_beat / 4;
	
	// create 2 bars only of metronome (this is enough)
	for (i=0; i<2; i++) {
	
		// note 1 on
		note.instrument = 0;			// instrument 0 is the drum
		note.bar = i;
		note.beat = 0;
		note.tick = (uint16_t) (0);
		note.qbar = note.bar;
		note.qbeat = note.beat;
		note.qtick = note.tick;
		note.status = MIDI_NOTEON;
		note.key = 76;							// high wood block
		note.vel = DEFAULT_METRONOME_VELOCITY;	// max velocity
		memcpy (&metronome [(i*8) + 0], &note, sizeof (note_t));

		// note 1 off
		note.tick = (uint16_t) (half_time_ticks_per_beat);
		note.qtick = note.tick;
		note.status = MIDI_NOTEOFF;
		memcpy (&metronome [(i*8) + 1], &note, sizeof (note_t));

		// note 2 on
		note.beat = 1;
		note.qbeat = note.beat;
		note.tick = (uint16_t) (time_ticks_per_beat);
		note.qtick = note.tick;
		note.status = MIDI_NOTEON;
		note.key = 77;					// low wood block
		memcpy (&metronome [(i*8) + 2], &note, sizeof (note_t));

		// note 2 off
		note.tick = (uint16_t) (time_ticks_per_beat + half_time_ticks_per_beat);
		note.qtick = note.tick;
		note.status = MIDI_NOTEOFF;
		memcpy (&metronome [(i*8) + 3], &note, sizeof (note_t));

		// note 3 on
		note.beat = 2;
		note.qbeat = note.beat;
		note.tick = (uint16_t) (2 * time_ticks_per_beat);
		note.qtick = note.tick;
		note.status = MIDI_NOTEON;
		memcpy (&metronome [(i*8) + 4], &note, sizeof (note_t));

		// note 3 off
		note.tick = (uint16_t) ((2 * time_ticks_per_beat) + half_time_ticks_per_beat);
		note.qtick = note.tick;
		note.status = MIDI_NOTEOFF;
		memcpy (&metronome [(i*8) + 5], &note, sizeof (note_t));

		// note 4 on
		note.beat = 3;
		note.qbeat = note.beat;
		note.tick = (uint16_t) (3 * time_ticks_per_beat);
		note.qtick = note.tick;
		note.status = MIDI_NOTEON;
		memcpy (&metronome [(i*8) + 6], &note, sizeof (note_t));

		// note 4 off
		note.tick = (uint16_t) ((3 * time_ticks_per_beat) + half_time_ticks_per_beat);
		note.qtick = note.tick;
		note.status = MIDI_NOTEOFF;
		memcpy (&metronome [(i*8) + 7], &note, sizeof (note_t));
	}
}
