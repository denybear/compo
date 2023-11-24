/** @file main.c
 *
 * @brief This is the main file for the looper program. It uses the basic features of JACK.
 *
 */

#include "types.h"
#include "main.h"
#include "process.h"
#include "utils.h"
#include "led.h"


/*************/
/* functions */
/*************/

static void init_globals ( )
{
	int i;

	/******************************/
	/* INIT SOME GLOBAL VARIABLES */
	/******************************/


}


static void signal_handler ( int sig )
{
	// JACK client close
	jack_client_close ( client );

	fprintf ( stderr, "signal received, exiting ...\n" );
	exit ( 0 );
}


/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown ( void *arg )
{
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

	/* assign number of frames per packet and sample rate */
	sample_rate = jack_get_sample_rate(client);
	nb_frames_per_packet =  jack_get_buffer_size(client);

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
