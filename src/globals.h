/** @file globals.h
 *
 * @brief This file the global variables for the program
 *
 */

// global variables

// Time and tempo variables, global to the entire transport timeline.
// There is no attempt to keep a true tempo map.  The default time
// signature is "march time": 4/4, 120bpm
extern float time_beats_per_bar;
extern float time_beat_type;
extern double time_ticks_per_beat;
extern double time_beats_per_minute;
extern float time_bpm_multiplier;
extern jack_position_t time_position;			// structure that contains BBT for the playing / recording 
extern jack_position_t previous_time_position;	// structure that contains BBT for the playing / recording 

// define midi ports
extern jack_port_t *midi_UI_in, *midi_UI_out;
extern jack_port_t *midi_KBD_in, *midi_KBD_out;
extern jack_port_t *midi_out;

// define JACKD client : this is this program
extern jack_client_t *client;

// tables required for UI
extern uint8_t ui_instruments [8];		// state of ui for instrument pads
extern uint8_t ui_pages [8];			// state of ui for pages pads
extern uint8_t ui_bars [8][8][64];		// state of ui for bar: 64 bar per page, 8 pages, 8 instruments
extern int ui_current_instrument;		// instrument currently selected
extern int ui_current_page;				// page currently selected
extern int ui_current_bar;				// bar currently selected

// define the structures for managing midi out (ie. lists)
extern uint8_t ui_list [][3];		// midi out buffer for UI
extern int ui_list_index;			// index in the list
extern uint8_t kbd_list [][3];		// midi out buffer for KBD
extern int kbd_list_index;			// index in the list
extern uint8_t out_list [] [3];		// midi out buffer for OUT (to fluidsynth)
extern int out_list_index;			// index in the list

// select functionality
extern int ui_limit1;
extern int ui_limit2;
extern int ui_limit1_pressed;
extern int ui_limit2_pressed;
extern uint8_t ui_select [64];				// buffer to store pads during selection process
extern uint8_t ui_select_previous [64];		// buffer to store pads during selection process (previous selection)

// song structure
extern note_t song [SONG_SIZE];			// assume song will have less than 10000 notes in it
extern int song_length;					// highest index in song []
extern note_t copy_buffer [COPY_SIZE];	// copy-paste buffer
extern int copy_length;					// highest index in copy_buffer []
extern uint8_t led_copy_buffer [512];	// 64 bytes * 8 pages to store led status of bars of copy buffer
extern int led_copy_length;				// highest index in led_copy_buffer []
extern note_t metronome [16];			// metronome: 4 note-on, 4 note-off on 2 bars

// status variables
extern int is_play;						// play is in progress
extern int is_record;					// record is in progress
extern int is_metronome;				// metronome is in progress

// quantization variables
extern int quantizer;						// contains value used for quantization

// tap tempo functionality
extern jack_nframes_t tap1, tap2;			// used to calculate tap tempo


