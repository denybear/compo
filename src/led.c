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
	int i, j;
	uint8_t buffer [4];

	buffer [0] = MIDI_NOTEON;
	for (i=0; i<8; i++) {
		for (j=0; j<8; j++) {
			buffer [1] = (i << 4) + j;
			buffer [2] = ui_bars [instr][page][(i * 8) + j];
			push_to_list (UI, buffer);		// put in midisend buffer
		}
	}
}

// light a single bar led, according to its value in the table
void led_ui_bar (int instr, int page, int bar) {

	uint8_t buffer [4];

	buffer [0] = MIDI_NOTEON;
	buffer [1] = bar2midi (bar);
	buffer [2] = ui_bars [instr][page][bar];
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





// function called to turn pad led on/off for a given row/col for filename
int led_filename (int row, int col, int on_off) {

	// check if the light is already ON, OFF, PENDING according to what we want
	// this will allow to determine whether we take the request into account or not
	if (led_status_filename [row][col] != on_off) {

		// push to list of led requests to be processed
		//push_to_list (NAMES, row, col, on_off);
		// update led status so it matches with request
		led_status_filename [row][col] = (unsigned char) on_off;

	}
}


// function called to turn function rows pad led on/off
int led_filefunct (int row, int col, int on_off) {

	// check if the light is already ON, OFF, PENDING according to what we want
	// this will allow to determine wether we take the request into account or not
	if (led_status_filefunct [row][col] != on_off) {
		
		// push to list of led requests to be processed
		//push_to_list (FCT, row, col, on_off);
		// update led status so it matches with request
		led_status_filefunct [row][col] = (unsigned char) on_off;

	}
}


// function called to turn pad led off for a filename
int filename_led_off (int row) {

	led_filename (row, PLAY, OFF);
	led_filename (row, LOAD, OFF);
	led_filename (row, B0, OFF);
	led_filename (row, B1, OFF);
	led_filename (row, B2, OFF);
	led_filename (row, B3, OFF);
	led_filename (row, B4, OFF);
	led_filename (row, B5, OFF);
	led_filename (row, B6, OFF);
	led_filename (row, B7, OFF);
}


// function called to turn pad led off for a set of functions
int filefunct_led_off (int row) {

int i;

	led_filefunct (row, VOLDOWN, OFF);
	led_filefunct (row, VOLUP, OFF);
	led_filefunct (row, BPMDOWN, OFF);
	led_filefunct (row, BPMUP, OFF);
}

