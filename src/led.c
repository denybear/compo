/** @file led.c
 *
 * @brief led module allows switching LEDs on/off on the midi control surface.
 *
 */

#include "types.h"
#include "globals.h"
#include "process.h"
#include "utils.h"
#include "led.h"
#include "song.h"
#include "disk.h"


// returns the color of the "bar" cursor
int color_ui_cursor () {
	
	if (is_record) return HI_RED;
	return HI_GREEN;
}


// for a colored bar, returns the next color (if color were pressed)
int color_ui_bar (int col) {

	if (col == BLACK) return BLACK;
	if (col == LO_RED) return LO_AMBER;
	if (col == LO_AMBER) return LO_GREEN;
	if (col == LO_GREEN) return LO_ORANGE;
	if (col == LO_ORANGE) return LO_YELLOW;
	if (col == LO_YELLOW) return LO_RED;
}


// light the "instruments" row of leds
// mode OFF turns all the leds to black
void led_ui_instruments (int mode) {
	int i;
	uint8_t buffer [4];

	buffer [0] = MIDI_CC;
	for (i=0; i<8; i++) {
		buffer [1] = i + 0x68;
		if (mode) {
			// manage case of LO_BLACK
			if (ui_instruments [i] == LO_BLACK) buffer [2] = BLACK;
			else buffer [2] = ui_instruments [i];
		}
		else {
			buffer [2] = BLACK;
		}
		push_to_list (UI, buffer);			// put in midisend buffer
	}
}

// light the "pages" row of leds
// mode OFF turns all the leds to black
void led_ui_pages (int mode) {
	int i;
	uint8_t buffer [4];

	buffer [0] = MIDI_NOTEON;
	for (i=0; i<8; i++) {
		buffer [1] = (i << 4) + 0x8;
		if (mode) buffer [2] = ui_pages [i];
		else buffer [2] = BLACK;
		push_to_list (UI, buffer);			// put in midisend buffer
	}
}


// light the "bars" table of leds
void led_ui_bars (int instr, int page) {

	int i;

	for (i=0; i<64; i++) {
		led_ui_bar (instr, page, i);
	}
}

// light a single bar led, according to its value in the table
void led_ui_bar (int instr, int page, int bar) {

	uint8_t buffer [4], color;

	// determine color based on bar color + selection color (in case selection is on the bar)
	if (ui_select [bar] == BLACK) color = ui_bars [instr][page][bar];
	else color = ui_select [bar];

	buffer [0] = MIDI_NOTEON;
	buffer [1] = bar2midi (bar);
	buffer [2] = color;
	push_to_list (UI, buffer);		// put in midisend buffer
}

// light a single instrument
void led_ui_instrument (int instr) {

	uint8_t buffer [4];

	buffer [0] = MIDI_CC;
	buffer [1] = instr + 0x68;
	// manage case of LO_BLACK
	if (ui_instruments [instr] == LO_BLACK) buffer [2] = BLACK;
	else buffer [2] = ui_instruments [instr];
	push_to_list (UI, buffer);			// put in midisend buffer
}

// light a single page
void led_ui_page (int page) {

	uint8_t buffer [4];

	buffer [0] = MIDI_NOTEON;
	buffer [1] = (page << 4) + 0x08;
	buffer [2] = ui_pages [page];
	push_to_list (UI, buffer);			// put in midisend buffer
}

// light selection between limit1 and limit2 in high green/red; erase previous selection; processing is done so that number of midi messages is optimized
// return first selected bar, which is always the min (lim1, lim2)
// lim1 and lim2 are both inclusive; ie. leds are lighted from lim1 to lim2 included
uint8_t led_ui_select (int lim1, int lim2) {

	uint8_t buffer [4], color;
	int start, end, i;

	// assign start and end so start <= end
	if (lim1 < lim2) {
		start = lim1;
		end = lim2;
	}
	else {
		start = lim2;
		end = lim1;
	}

	// clean current display buffer (to avoid overwriting issues)
	memset (ui_select, BLACK, 64);
	// display selection in high green in the display buffer
	i = start;
	while (i <= end ) {
		ui_select [i] = color_ui_cursor ();
		i++;
	}

	// display on the launchpad, by analyzing current selection to be displayed vs. previous selection buffer to be displayed
	// go from pad to pad
	for (i=0; i<64; i++) {
		if ((ui_select_previous [i] != BLACK) && (ui_select [i] == BLACK)) {
			// this pad was lit, but should now be unlit
			led_ui_bar (ui_current_instrument, ui_current_page, i);
			continue;
		}
		if ((ui_select_previous [i] == BLACK) && (ui_select [i] != BLACK)) {
			// light pad in case it was not lit previously
			led_ui_bar (ui_current_instrument, ui_current_page, i);
			continue;
		}
		if (ui_select_previous [i] != ui_select [i]) {
			// pads are both lit, but not the same color
			led_ui_bar (ui_current_instrument, ui_current_page, i);
			continue;
		}
	}

	// copy current select buffer into previous buffer: current becomes previous
	memcpy (ui_select_previous, ui_select, 64);		// memcpy is fine as there is no overlapping memory area

	return (start);
}

// go through the whole song notes, and set the "bars" tables of leds according to notes color
void note2bar_color () {

	int i;
	int instr, page, bar;

	// fill UI structure with start values to set up leds
	memset (ui_bars, BLACK, 8 * 8 * 64);

	for (i = 0; i <song_length; i++) {
		// get bar number
		instr = song [i].instrument;
		page = song [i].qbar / 64;		// 64 bars per page
		bar = song [i].qbar % 64;

		ui_bars [instr][page][bar] = song [i].color;	// set to the right color
	}
}


// go through the "bars" of the song, and set the notes color according to bars color
void bar2note_color () {

	int i;
	int instr, page, bar;

	for (i = 0; i <song_length; i++) {
		// get bar number
		instr = song [i].instrument;
		page = song [i].qbar / 64;		// 64 bars per page
		bar = song [i].qbar % 64;

		song [i].color = ui_bars [instr][page][bar];	// set to the right color
	}
}


// light the "files" table of leds
void led_ui_files () {

	int i;
	uint8_t buffer [4];

	for (i=0; i<64; i++) {

		buffer [0] = MIDI_NOTEON;
		buffer [1] = bar2midi (i);
		// set color according whether we load or save file
		if (is_load) buffer [2] = (save_files) [i] ? LO_GREEN : BLACK;
		if (is_save) buffer [2] = (save_files) [i] ? LO_RED : BLACK;
		push_to_list (UI, buffer);		// put in midisend buffer
	}
}


// light the "midi instruments" table of leds
void led_ui_instrument_bank (int bank) {

	int i;
	uint8_t buffer [4];

	for (i=0; i<64; i++) {

		buffer [0] = MIDI_NOTEON;
		buffer [1] = bar2midi (i);
		// set color according bank number
		if (bank == 0) buffer [2] = BLACK;
		if (bank == 1) buffer [2] = LO_AMBER;
		if (bank >= 2) buffer [2] = LO_ORANGE;
		push_to_list (UI, buffer);		// put in midisend buffer
	}
}


// light a single instrument (top row of leds of launchpad)
void led_ui_single_instrument (int instr, int bank) {

	uint8_t buffer [4];

	buffer [0] = MIDI_CC;
	buffer [1] = instr + 0x68;
	// set color according bank number
	if (bank == 0) buffer [2] = BLACK;
	if (bank == 1) buffer [2] = LO_AMBER;
	if (bank >= 2) buffer [2] = LO_ORANGE;
	push_to_list (UI, buffer);			// put in midisend buffer
}


// light led on the pad of selected "midi instrument"
// depending on "midi instrument" number and current bank number, it will display or not
// onoff parameter allows to light/unlight the led 
void led_ui_cursor_instrument (int instr_number, int bank, int onoff) {

	uint8_t buffer [4];
	int instr_pad;

	instr_pad = instr_number % 64;		// pad number between 0 and 63

	buffer [0] = MIDI_NOTEON;
	buffer [1] = bar2midi (instr_pad);
	// set color according bank number
	if (onoff) {		// in case of ON
		if (bank == 0) buffer [2] = BLACK;
		if (bank == 1) buffer [2] = HI_AMBER;
		if (bank >= 2) buffer [2] = HI_ORANGE;
	}
	else {				// in case of OFF
		if (bank == 0) buffer [2] = BLACK;
		if (bank == 1) buffer [2] = LO_AMBER;
		if (bank >= 2) buffer [2] = LO_ORANGE;
	}

	// based on bank number, determine whether we should light pad or not (i. send midi data or not)
	if (((instr_number / 64) + 1) == bank)	push_to_list (UI, buffer);			// put in midisend buffer
}
