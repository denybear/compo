/** @file disk.c
 *
 * @brief Contains load and save functions.
 *
 */

#include "types.h"
#include "globals.h"
#include "process.h"
#include "utils.h"
#include "led.h"
#include "song.h"
#include "disk.h"



// function called in case user pressed the load key
// name of the file is a pad number, in hex
// directory is the directory where the files are saved
int load (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	int number_of_tracks;
	track_t tr;
	jack_default_audio_sample_t *l, *r;

	// create file in write mode
	fp = fopen ("./boocli.sav", "r");
	if (fp==NULL) {
		fprintf ( stderr, "Cannot read save file.\n" );
		return 0;
	}

	// read number of tracks
	fread ((int *) &number_of_tracks, sizeof (int), 1, fp);
	// if greater than current number of tracks, set to current number of tracks
	if (number_of_tracks > NB_TRACKS) number_of_tracks = NB_TRACKS;

	// read time signature
	fread ((int*) &timesign, sizeof (int), 1, fp);
	// if timesign is out of boundaries, set to first timesign (_4_4)
	if (timesign > LAST_TIMESIGN) timesign = FIRST_TIMESIGN;

	// read track one by one
	for (i=0; i<NB_TRACKS;i++) {

		// read the whole track struct from file
		fread (&tr, sizeof (track_t), 1, fp);
		// check whether there is some audio recorded; if not, read next track
		// this way: in case the track in the file is empty (non-recorded), the track already in memory is kept and is not overwritten by an empty track
		if ((tr.end_index_left == 0) && (tr.end_index_right == 0)) continue;

		// save address of audio buffers (absolutely required otherwise we will point anywhere in memory!)
		l = track[i].left;
		r = track[i].right;

		// copy tr variable to track variable
		memcpy (&track[i], &tr, sizeof(track_t));

		// restore address of audio buffers
		track[i].left = l;
		track[i].right = r;

		// read the audio buffers and write to memory
		if (track[i].end_index_left !=0) fread (track[i].left, sizeof (jack_default_audio_sample_t), track[i].end_index_left, fp);
		if (track[i].end_index_right !=0) fread (track[i].right, sizeof (jack_default_audio_sample_t), track[i].end_index_right, fp);
	}

	// close file
	fclose (fp);
}


// function called in case user pressed the save pad
int save (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	char filename[100];		// temp structure for file name
	char line [100];		// temp line to be written in the file

	// create file path
	sprintf (filename, "%s/%02x", directory, number);


	// create file in write mode
	fp = fopen (filename, "w");
	if (fp==NULL) {
		printf ("Cannot write save file: %s\n", filename);
		return 0;
	}

	// opening bracket
	sprintf (line, "{\n");

	// tempo values
	sprintf (line, "\t\"beats_per_bar\": %f,\n", time_beats_per_bar);
	fwrite ((char *) line, sizeof (line), 1, fp);
	sprintf (line, "\t\"beat_type\": %f,\n", time_beat_type);
	fwrite ((char *) line, sizeof (line), 1, fp);
	sprintf (line, "\t\"ticks_per_beat\": %f,\n", time_ticks_per_beat);
	fwrite ((char *) line, sizeof (line), 1, fp);
	sprintf (line, "\t\"ticks_beats_per_minute\": %f,\n", time_beats_per_minute);
	fwrite ((char *) line, sizeof (line), 1, fp);
	
	// quantizer
	sprintf (line, "\t\"quantizer\": %d,\n", quantizer);
	fwrite ((char *) line, sizeof (line), 1, fp);

	// song length
	sprintf (line, "\t\"song_length\": %d,\n", song_length);
	fwrite ((char *) line, sizeof (line), 1, fp);

	// notes of song
	sprintf (line, "\t\"notes\": [\n");
	fwrite ((char *) line, sizeof (line), 1, fp);
	
	for (i = 0; i < song_length; i++) { 
		// opening bracket
		sprintf (line, "\t\t{\n");
		fwrite ((char *) line, sizeof (line), 1, fp);

		// note
		sprintf (line, "\t\t\t\"key\": \"%02X\",\n", song [i].key);
		fwrite ((char *) line, sizeof (line), 1, fp);

//**HERE****
//	uint16_t bar;			// realtime BBT
//	uint16_t tick;
//	uint8_t beat;
//	uint16_t qbar;			// quantized BBT
//	uint16_t qtick;
//	uint8_t qbeat;
//	uint8_t instrument;
//	uint8_t status;			// MIDI cmd only, not the channel
//	uint8_t key;
//	uint8_t vel;



		// closing bracket
		sprintf (line, "\t\t},\n");
		fwrite ((char *) line, sizeof (line), 1, fp);
	}

	// closing square bracket (notes of song)
	sprintf (line, "\t],\n");
	fwrite ((char *) line, sizeof (line), 1, fp);


	// ui bars per instrument
	
	// close file
	fclose (fp);
}


