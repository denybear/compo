/** @file main.h
 *
 * @brief This file contains global variables definition
 *
 */


/********************/
/* global variables */
/********************/

// Time and tempo variables, global to the entire transport timeline.
// There is no attempt to keep a true tempo map.  The default time
// signature is "march time": 4/4, 120bpm
float time_beats_per_bar = 4.0;
float time_beat_type = 4.0;
double time_ticks_per_beat = 1920.0;	// 480 should be enough...
double time_beats_per_minute = 120.0;
jack_position_t time_position;			// structure that contains BBT for the playing / recording 

// define midi ports
jack_port_t *midi_UI_in, *midi_UI_out;
jack_port_t *midi_KBD_in, *midi_KBD_out;
jack_port_t *midi_out;

// define JACKD client : this is this program
jack_client_t *client;

// number of frames per Jack packet and sample-rate
uint32_t nb_frames_per_packet, sample_rate;

// tables required for UI
uint8_t ui_instruments [8];		// state of ui for instrument pads
uint8_t ui_pages [8];			// state of ui for pages pads
uint8_t ui_bars [8][8][64];		// state of ui for bar: 64 bar per page, 8 pages, 8 instruments
int ui_current_instrument;		// instrument currently selected
int ui_current_page;			// page currently selected
int ui_current_bar;				// bar currently selected

// define the structures for managing midi out (ie. lists)
uint8_t ui_list [LIST_ELT] [3];		// midi out buffer for UI
int ui_list_index = 0;				// index in the list
uint8_t kbd_list [LIST_ELT] [3];	// midi out buffer for KBD
int kbd_list_index = 0;				// index in the list
uint8_t out_list [LIST_ELT] [3];	// midi out buffer for OUT (to fluidsynth)
int out_list_index = 0;				// index in the list

// select functionality
int ui_limit1;
int ui_limit2;
int ui_limit1_pressed;
int ui_limit2_pressed;
uint8_t ui_select [64];					// buffer to store pads during selection process
uint8_t ui_select_previous [64];		// buffer to store pads during selection process (previous selection)

// song structure
note_t song [SONG_SIZE];			// assume song will have less than 10000 notes in it

// status variables
int is_play;						// play is in progress
int is_record;						// record is in progress 

