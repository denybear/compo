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


// read notes from song structure, which are located between bar, tick_limit1 (inclusive) and bar, tick_limit2 (exclusive)
// returns a pointer to list of notes falling in this category, NULL if nothing
// returns also the number of notes in the list (0 if no notes in the list) 
note_t * read_from_song (u_int16_t b_limit1, u_int16_t t_limit1, u_int16_t b_limit2, u_int16_t t_limit2, int *length) {

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
	if (i==song_length) {
		*length = 0;
		return NULL;
	}

	for (i=0; i<song_length; i++) {
		if (song [i].bar > b_limit2) {
			bar_limit2 = i;		// limit2 is the first "higher" bar of the end of the song part to play
			break;
		}
	}
	// test if we reached song boundary without success: in this case, limit2 is at the end of the song
	if (i==song_length) bar_limit2 = i;

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
	if (i == bar_limit2) {
		*length = 0;
		return NULL;
	}

	for (i=bar_limit1; i<bar_limit2; i++) {
		if ((song [i].bar == b_limit2) && (song [i].tick >= t_limit2)) {
		// if you change condition with ((song [i].bar == b_limit2) && (song [i].tick > t_limit2)), then tick_limit2 is inclusive, not exclusive
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
	return (&song [tick_limit1]);
}


// for debug use only
void test_write () {

	note_t note;

	note.bar =5;
	note.beat =1;
	note.tick =480;
	note.instrument =1;
	write_to_song (note);
	display_song (song_length, song, "test: insert 1st note in song");

	note.bar =6;
	note.beat =0;
	note.tick =0;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note at end of song");

	note.bar =3;
	note.beat =2;
	note.tick =960;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note at beginning of song");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note between 2 bars");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.instrument =2;
	write_to_song (note);
	display_song (song_length, song, "insert note same as previous");

	note.bar =4;
	note.beat =2;
	note.tick =961;
	note.instrument =7;
	write_to_song (note);
	display_song (song_length, song, "insert note same bar, higher tick");

	note.bar =4;
	note.beat =1;
	note.tick =959;
	note.instrument =7;
	write_to_song (note);
	display_song (song_length, song, "insert note same bar, lower tick");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.instrument =3;
	write_to_song (note);
	display_song (song_length, song, "insert note same as previous, with higher instr");

	note.bar =4;
	note.beat =2;
	note.tick =960;
	note.instrument =0;
	write_to_song (note);
	display_song (song_length, song, "insert note same as previous, with lower instr");

	note.bar =6;
	note.beat =3;
	note.tick =1440;
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
		printf ("index:%d, bar:%d, beat:%d, tick:%d, instr:%d\n", i, sg[i].bar, sg[i].beat, sg[i].tick, sg[i].instrument);
	}
	printf ("\n");
}
