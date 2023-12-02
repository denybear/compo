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
extern jack_position_t time_position;			// structure that contains BBT for the playing / recording 

// define midi ports
extern jack_port_t *midi_UI_in, *midi_UI_out;
extern jack_port_t *midi_KBD_in, *midi_KBD_out;
extern jack_port_t *midi_out;

// define JACKD client : this is this program
extern jack_client_t *client;

// number of frames per Jack packet and sample-rate
extern uint32_t nb_frames_per_packet, sample_rate;

// tables required for UI
extern uint8_t ui_instruments [8];		// state of ui for instrument pads
extern uint8_t ui_pages [8];			// state of ui for pages pads
extern uint8_t ui_bars [8][8][64];		// state of ui for bar: 64 bar per page, 8 pages, 8 instruments
extern int ui_current_instrument;		// instrument currently selected
extern int ui_current_page;			// page currently selected
extern int ui_current_bar;				// bar currently selected

// define the structures for managing midi out (ie. lists)
extern uint8_t ui_list [][3];		// midi out buffer for UI
extern int ui_list_index;			// index in the list
extern uint8_t kbd_list [][3];		// midi out buffer for KBD
extern int kbd_list_index;			// index in the list

// select functionality
extern int ui_limit1;
extern int ui_limit2;
extern int ui_limit1_pressed;
extern int ui_limit2_pressed;
extern uint8_t ui_select [64];				// buffer to store pads during selection process
extern uint8_t ui_select_previous [64];		// buffer to store pads during selection process (previous selection)






// determine if midi clock shall be sent or not
extern int send_clock;

// define filename structure for each file name: midi file and SF2 file
extern filename_t filename [];
// define function structure
extern filefunct_t filefunct [];

// status of leds for filenames
extern unsigned char led_status_filename [NB_NAMES][LAST_ELT]; 	// this table will contain whether each light is on/off at a time; this is to avoid sending led requests which are not required
extern unsigned char led_status_filefunct [NB_FCT][LAST_ELT_FCT]; 	// this table will contain whether each light is on/off at a time; this is to avoid sending led requests which are not required

/* volume and BPM */
extern int bpm;
extern int initial_bpm;
extern int volume;
extern int is_volume;

/* PPQ */
extern int ppq;

/* beat */
extern uint64_t now;       // time now
extern uint64_t previous;  // time when "beat" key was last pressed


