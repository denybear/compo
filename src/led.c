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


// function called to turn pad led on/off for a given row/col for filename
int led_filename (int row, int col, int on_off) {

	// check if the light is already ON, OFF, PENDING according to what we want
	// this will allow to determine whether we take the request into account or not
	if (led_status_filename [row][col] != on_off) {

		// push to list of led requests to be processed
		push_to_list (NAMES, row, col, on_off);
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
		push_to_list (FCT, row, col, on_off);
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

