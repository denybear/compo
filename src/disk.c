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



// function called in case user pressed the load pad
// name is a pad number (0-63) which corresponds to the number of the song
// directory is the patch where songs are stored
int load (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	char filename[100];		// temp structure for file name
	char line [100];		// temp line to be written in the file


	// create file path
	sprintf (filename, "%s/%02x", directory, number);

	// create file in write mode
	fp = fopen (filename, "rt");
	if (fp==NULL) {
		printf ("Cannot read save file: %s\n", filename);
		return 0;
	}

	// read number of tracks
//	fread ((int *) &number_of_tracks, sizeof (int), 1, fp);

	// close file
	fclose (fp);
}


// function called in case user pressed the save pad
// name is a pad number (0-63) which corresponds to the number of the song
// directory is the patch where songs are stored
int save (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	char filename[100];		// temp structure for file name
	char line [100];		// temp line to be written in the file


	// create file path
	sprintf (filename, "%s/%02x", directory, number);

	// create file in write mode
	fp = fopen (filename, "wt");
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
		sprintf (line, "\t\t\t\"instrument\": %d,\n", song [i].instrument);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"status\": \"%02X\",\n", song [i].status);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"key\": \"%02X\",\n", song [i].key);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"velocity\": \"%02X\",\n", song [i].vel);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"color\": \"%02X\",\n", song [i].color);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"bar\": %d,\n", song [i].bar);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"beat\": %d,\n", song [i].beat);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"tick\": %d,\n", song [i].tick);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"qbar\": %d,\n", song [i].qbar);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"qbeat\": %d,\n", song [i].qbeat);
		fwrite ((char *) line, sizeof (line), 1, fp);
		sprintf (line, "\t\t\t\"qtick\": %d,\n", song [i].qtick);
		fwrite ((char *) line, sizeof (line), 1, fp);

		// closing bracket; add comma except for last element of the list
		if (i == (song_length - 1)) sprintf (line, "\t\t}\n");
		else sprintf (line, "\t\t},\n");
		fwrite ((char *) line, sizeof (line), 1, fp);
	}

	// closing square bracket (notes of song)
	sprintf (line, "\t]\n");
	fwrite ((char *) line, sizeof (line), 1, fp);

	// closing final bracket
	sprintf (line, "}\n");

	// close file
	fclose (fp);
}


// go through the color of bar in the ui, and set the same color to each note of the song
void get_colors_from_ui () {

	int i;

	for (i = 0; i < song_length; i++) {
		song [i].color = ui_bars [song[i].instrument][(song[i].bar / 64)][(song[i].bar % 64)];
	}
}


// go through the notes of the song, read color of note and set the same color for the bar in the ui
void set_colors_to_ui () {

	int i;

	// clear all ui bars memory with black
	memset (ui_bars, BLACK, 8 * 8 * 64);

	for (i = 0; i < song_length; i++) {
		ui_bars [song[i].instrument][song[i].bar / 64][song[i].bar % 64] = song [i].color;
	}
}
