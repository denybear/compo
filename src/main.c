/** @file main.c
 *
 * @brief This is the main file for the program. It uses the basic features of JACK.
 *
 */

#include "types.h"
#include "main.h"
#include "process.h"
#include "utils.h"
#include "led.h"
#include "song.h"


/*************/
/* functions */
/*************/

static void init_globals ( )
{
	int i;

	/******************************/
	/* INIT SOME GLOBAL VARIABLES */
	/******************************/

	// empty midi send lists
	ui_list_index = 0;
	kbd_list_index = 0;
	out_list_index = 0;
	memset (ui_list, 0, LIST_ELT * 3);
	memset (kbd_list, 0, LIST_ELT * 3);
	memset (out_list, 0, LIST_ELT * 3);
	// empty song structure
	memset (song, 0, SONG_SIZE * sizeof (note_t));
	// empty copy_buffer structure
	memset (copy_buffer, 0, COPY_SIZE * sizeof (note_t));
	// fill UI structures with start values to set up leds
	memset (ui_instruments, LO_GREEN, 8);
	memset (ui_pages, LO_GREEN, 8);
	memset (ui_bars, BLACK, 8 * 8 * 64);
	// set cursor to 1st instrument, 1st page, 1st bar
	ui_current_instrument = 0;
	ui_current_page = 0;
	ui_current_bar = 0;
	ui_instruments [ui_current_instrument] = HI_GREEN;
	ui_pages [ui_current_page] = HI_GREEN;

	// reset selection variables
	// clear selection buffers
	memset (ui_select_previous, BLACK, 64);
	memset (ui_select, BLACK, 64);
	// set current selection to first bar
	ui_select [ui_current_bar] = HI_GREEN;			// cursor color is high green
	ui_limit1 = ui_current_bar;
	ui_limit2 = ui_current_bar;
	ui_limit1_pressed = FALSE;
	ui_limit2_pressed = FALSE;

	// init ncurses for non-blocking key capture
	initscr();				// init curses, 
	nodelay(stdscr, TRUE);	// no delaying, no blocking
	noecho();				// no echoing
	cbreak ();				// no buffering
	keypad (stdscr, TRUE);	// special keys can be captured

	// init quantization engine
	quantizer = THIRTY_SECOND;
	create_quantization_tables ();		// create all the tables used for quantization

	// create metronome table
	create_metronome ();

	// set other variables
	is_play = FALSE;
	is_record = FALSE;
	is_metronome = FALSE;
	song_length = 0;		// indicates length of the song (highest index in song [])
	copy_length = 0;		// length of copy buffer: empty
	tap1 = 0;				// tap tempo
	tap2 = 0;
}


static void signal_handler ( int sig )
{
	// JACK client close
	jack_client_close ( client );
	echo ();
	endwin ();				// end curses

	fprintf ( stderr, "signal received, exiting ...\n" );
	exit ( 0 );
}


/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown ( void *arg )
{
	echo ();
	endwin ();				// end curses

	free (midi_UI_in);
	free (midi_UI_out);
	free (midi_KBD_in);
	free (midi_KBD_out);
	free (midi_out);

	exit ( 1 );
}

/* usage: compo (jack client name) (jack server name)*/

int main ( int argc, char *argv[] )
{
	int i,j;
	
	// JACK variables
	const char *client_name;
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;


	/* use basename of argv[0] as jack client name */
	client_name = strrchr ( argv[0], '/' );
	if ( client_name == 0 ) client_name = argv[0];
	else client_name++;

	if ( argc >= 2 )        /* client name specified? if yes, then assign */
	{
		client_name = argv[1];
		if ( argc >= 3 )    /* server name specified? if yes, then assign */
		{
			server_name = argv[2];
			options |= JackServerName;
		}
	}

	/* open a client connection to the JACK server */

	client = jack_client_open ( client_name, options, &status, server_name );
	if ( client == NULL )
	{
		fprintf ( stderr, "jack_client_open() failed, status = 0x%2.0x.\n", status );
		if ( status & JackServerFailed )
		{
			fprintf ( stderr, "Unable to connect to JACK server.\n" );
		}
		exit ( 1 );
	}
	if ( status & JackServerStarted )
	{
		fprintf ( stderr, "JACK server started.\n" );
	}
	if ( status & JackNameNotUnique )
	{
		client_name = jack_get_client_name ( client );
		fprintf ( stderr, "unique name `%s' assigned.\n", client_name );
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	/* set callback function to process jack events */
	jack_set_process_callback ( client, process, 0 );

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/
	jack_on_shutdown ( client, jack_shutdown, 0 );

	/* register midi-in port: this port will get the midi keys notification (from UI) */
	midi_UI_in = jack_port_register (client, "midi_UI_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (midi_UI_in == NULL ) {
		fprintf ( stderr, "no more JACK MIDI ports available.\n" );
		// JACK client close
		jack_client_close ( client );
		exit ( 1 );
	}

	/* register midi-out port: this port will send the midi notifications to light on/off pad leds from UI */
	midi_UI_out = jack_port_register (client, "midi_UI_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
	if (midi_UI_out == NULL ) {
		fprintf ( stderr, "no more JACK MIDI ports available.\n" );
		// JACK client close
		jack_client_close ( client );
		exit ( 1 );
	}

	/* register midi-in port: this port will get the midi keys notification (from midi keyboard) */
	midi_KBD_in = jack_port_register (client, "midi_KBD_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (midi_KBD_in == NULL ) {
		fprintf ( stderr, "no more JACK MIDI ports available.\n" );
		// JACK client close
		jack_client_close ( client );
		exit ( 1 );
	}

	/* register midi-out port: this port will send the midi notifications to light on/off pad leds from KBD */
	midi_KBD_out = jack_port_register (client, "midi_KBD_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
	if (midi_KBD_out == NULL ) {
		fprintf ( stderr, "no more JACK MIDI ports available.\n" );
		// JACK client close
		jack_client_close ( client );
		exit ( 1 );
	}

	/* register midi-out port: this port will send the midi events to external system (eg. fluidsynth) */
	midi_out = jack_port_register (client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
	if (midi_out == NULL ) {
		fprintf ( stderr, "no more JACK CLOCK ports available.\n" );
		// JACK client close
		jack_client_close ( client );
		exit ( 1 );
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if ( jack_activate ( client ) )
	{
		fprintf ( stderr, "cannot activate client.\n" );
		// JACK client close
		jack_client_close ( client );
		exit ( 1 );
	}

	// init global variables
	init_globals();


	/**************/
	/* MAIN START */
	/**************/

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	/* go through the list of ports to be connected and connect them by pair (server, client) */
	/* fixed devices and fluidsynth output */
	if ( jack_connect ( client, "a2j:Launchpad Mini (capture): Launchpad Mini MIDI 1", "compo.a:midi_UI_in") ) {
			fprintf ( stderr, "cannot connect ports (between client and server).\n" );
	}
	if ( jack_connect ( client, "compo.a:midi_UI_out", "a2j:Launchpad Mini (playback): Launchpad Mini MIDI 1") ) {
			fprintf ( stderr, "cannot connect ports (between client and server).\n" );
	}
	if ( jack_connect ( client, "a2j:MPK Mini Mk II (capture): MPK Mini Mk II MIDI 1", "compo.a:midi_KBD_in") ) {
			fprintf ( stderr, "cannot connect ports (between client and server).\n" );
	}
	if ( jack_connect ( client, "compo.a:midi_KBD_out", "a2j:MPK Mini Mk II (playback): MPK Mini Mk II MIDI 1") ) {
			fprintf ( stderr, "cannot connect ports (between client and server).\n" );
	}
	if ( jack_connect ( client, "compo.a:midi_out", "qsynth:midi_00") ) {
			fprintf ( stderr, "cannot connect ports (between client and server).\n" );
	}


	// assign midi instrument to each channel
	init_instruments ();
	// set volume for each channel to 64 (mid-volume)
	init_volumes (64);

	// light leds on the UI
	led_ui_instruments ();
	led_ui_pages ();
	led_ui_bars (ui_current_instrument, ui_current_page);
	// display between limit 1 and 2
	ui_current_bar = led_ui_select (ui_limit1, ui_limit2);



	/* install a signal handler to properly quits jack client */
#ifdef WIN32
	signal ( SIGINT, signal_handler );
	signal ( SIGABRT, signal_handler );
	signal ( SIGTERM, signal_handler );
#else
	signal ( SIGQUIT, signal_handler );
	signal ( SIGTERM, signal_handler );
	signal ( SIGHUP, signal_handler );
	signal ( SIGINT, signal_handler );
#endif


	/* keep running until the transport stops */
	while (1)
	{
		// check if user has loaded the LOAD button to load midi and SF2 file
		//if (is_load) {
		//}

#ifdef WIN32
		Sleep ( 1000 );
#else
		sleep ( 1 );
#endif
	}

	// JACK client close
	jack_client_close ( client );
	exit ( 0 );
}
