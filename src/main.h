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

// determine if midi clock shall be sent or not
int send_clock = NO_CLOCK;

// define filename structure for each file name: midi file and SF2 file
filename_t filename [NB_NAMES];
// define function structure
filefunct_t filefunct [NB_FCT];

// define the structures for managing leds of midi control surface
unsigned char list_buffer[LIST_ELT][4];	// list buffer of LIST_ELT led request
unsigned char list_index;			// index where to write led request to

// status of leds for filenames
unsigned char led_status_filename [NB_NAMES][LAST_ELT]; 	// this table will contain whether each light is on/off at a time; this is to avoid sending led requests which are not required
unsigned char led_status_filefunct [NB_FCT][LAST_ELT_FCT]; 	// this table will contain whether each light is on/off at a time; this is to avoid sending led requests which are not required

/* volume and BPM */
int bpm;
int initial_bpm;
int volume;
int is_volume;

/* PPQ */
int ppq;

/* beat */
uint64_t now;       // time now
uint64_t previous;  // time when "beat" key was last pressed

/* for debug purpose only
char trace[50000][80];
int trace_index = 0;
int trace_beat;
*/

