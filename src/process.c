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


// number of midi clock signal sent per quarter note; from 0 to 23
int midi_clock_num;
// previous index pulse: used to see whether we changed quarter note (beat)
int previous_index_pulse;


// main process callback called at capture of (nframes) frames/samples
int process ( jack_nframes_t nframes, void *arg )
{
	jack_nframes_t h;
	int i,j,k;
	int mute = OFF;
	void *midiin;
	void *midiout;
	jack_midi_event_t in_event;
	jack_midi_data_t buffer[5];				// midi out buffer for lighting the pad leds and for midi clock
	int dest, row, col, on_off;				// variables used to manage lighting of the pad leds


	/**************************************/
	/* First, process MIDI in (UI) events */
	/**************************************/

	// Get midi buffers
	midiin = jack_port_get_buffer(midi_UI_in, nframes);

	// process MIDI IN events
	for (i=0; i< jack_midi_get_event_count(midiin); i++) {
		if (jack_midi_event_get (&in_event, midiin, i) != 0) {
			fprintf ( stderr, "Missed in event\n" );
			continue;
		}
		// call processing function
		midi_in_process (&in_event,nframes);
	}


	/****************************************/
	/* Second, process MIDI in (KBD) events */
	/****************************************/

	// Get midi buffers
	midiin = jack_port_get_buffer(midi_KBD_in, nframes);

	// process MIDI IN events
	for (i=0; i< jack_midi_get_event_count(midiin); i++) {
		if (jack_midi_event_get (&in_event, midiin, i) != 0) {
			fprintf ( stderr, "Missed in event\n" );
			continue;
		}
		// call processing function
		midi_in_process (&in_event,nframes);
	}


	/***************************************/
	/* Third, process MIDI out (UI) events */
	/***************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_UI_out, nframes);

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests
	while (pull_from_list(&dest, &row, &col, &on_off)) {

		// if this is a filename led
		// copy midi event required to light led into midi buffer
		if (dest == NAMES) memcpy (buffer, &filename[row].led [col][on_off][0], 3);
		// copy midi event required to light led into midi buffer
		if (dest == FCT) memcpy (buffer, &filefunct[row].led [col][on_off][0], 3);

		// if buffer is not empty, then send as midi out event
		// we take care of writing led events at different time marks to make sure all of these are taken into account
		if (buffer [0] | buffer [1] | buffer [2]) {
			jack_midi_event_write (midiout, 0, buffer, 3);
		}
	}


	/*****************************************/
	/* Fourth, process MIDI out (KBD) events */
	/*****************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_KBD_out, nframes);

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests
	while (pull_from_list(&dest, &row, &col, &on_off)) {

		// if this is a filename led
		// copy midi event required to light led into midi buffer
		if (dest == NAMES) memcpy (buffer, &filename[row].led [col][on_off][0], 3);
		// copy midi event required to light led into midi buffer
		if (dest == FCT) memcpy (buffer, &filefunct[row].led [col][on_off][0], 3);

		// if buffer is not empty, then send as midi out event
		// we take care of writing led events at different time marks to make sure all of these are taken into account
		if (buffer [0] | buffer [1] | buffer [2]) {
			jack_midi_event_write (midiout, 0, buffer, 3);
		}
	}


	/******************************************/
	/* Fifth, process MIDI out events (music) */
	/******************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_out, nframes);

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests
	while (pull_from_list(&dest, &row, &col, &on_off)) {

		// if this is a filename led
		// copy midi event required to light led into midi buffer
		if (dest == NAMES) memcpy (buffer, &filename[row].led [col][on_off][0], 3);
		// copy midi event required to light led into midi buffer
		if (dest == FCT) memcpy (buffer, &filefunct[row].led [col][on_off][0], 3);

		// if buffer is not empty, then send as midi out event
		// we take care of writing led events at different time marks to make sure all of these are taken into account
		if (buffer [0] | buffer [1] | buffer [2]) {
			jack_midi_event_write (midiout, 0, buffer, 3);
		}
	}


	return 0;
}


// process callback called to process midi_in events in realtime
int midi_in_process (jack_midi_event_t *event, jack_nframes_t nframes) {


}
