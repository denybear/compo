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


// returns the color of the "bar" cursor
int color_ui_cursor () {
	
	if (is_record) return HI_RED;
	return HI_GREEN;
}


// light the "instruments" row of leds
void led_ui_instruments () {
	int i;
	uint8_t buffer [4];

	buffer [0] = MIDI_CC;
	for (i=0; i<8; i++) {
		buffer [1] = i + 0x68;
		buffer [2] = ui_instruments [i];
		push_to_list (UI, buffer);			// put in midisend buffer
	}
}

// light the "pages" row of leds
void led_ui_pages () {
	int i;
	uint8_t buffer [4];

	buffer [0] = MIDI_NOTEON;
	for (i=0; i<8; i++) {
		buffer [1] = (i << 4) + 0x8;
		buffer [2] = ui_pages [i];
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
//	if ((ui_select [bar] == BLACK) || (ui_select [bar] == BLACK_ERASE)) color = ui_bars [instr][page][bar];
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
	buffer [2] = ui_instruments [instr];
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
		}
		if ((ui_select_previous [i] == BLACK) && (ui_select [i] != BLACK)) {
			// light pad in case it was not lit previously
			led_ui_bar (ui_current_instrument, ui_current_page, i);
		}
		// in case pad was hi green or red previously, we do nothing (no midi msg sent) as an optimisation
	}

	// copy current select buffer into previous buffer: current becomes previous
	memcpy (ui_select_previous, ui_select, 64);		// memcpy is fine as there is no overlapping memory area

	return (start);
}

// go through the whole song, and set the "bars" tables of leds accordingly (depending a bar exists or not)
void refresh_ui_bars () {

	int i;
	int instr, page, bar;

	// fill UI structure with start values to set up leds
	memset (ui_bars, BLACK, 8 * 8 * 64);

	for (i = 0; i <song_length; i++) {
		// values
		instr = song [i].instrument;
		page = song [i].bar / 64;		// 64 bars per page
		bar = song [i].bar % 64;

		ui_bars [instr][page][bar] = LO_AMBER;		// to change to bar color when we will implement the functionality
	}
}
