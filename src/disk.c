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
	char filename [100];		// temp structure for file name
	char str1 [50], str2 [50], str3 [50];
	uint8_t instr [8];			// (drum), piano, elec piano, hammond organ, fingered bass, clean guitar, string ensemble, brass ensemble

	// create file path
	sprintf (filename, "%s/%02x", directory, number);

	// create file in write mode
	fp = fopen (filename, "rt");
	if (fp==NULL) {
		printf ("Cannot read save file: %s\n", filename);
		return 0;
	}

	// opening bracket
	fscanf (fp, "%s %s", str1);

	// instruments; first instrument does not count (drum channel)
	fscanf (fp, "%s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %s", str1, str2, &instr [0], &instr [1], &instr [2], &instr [3], &instr [4], &instr [5], &instr [6], &instr [7], str3);

	//**********HERE


	// tempo values
	fprintf (fp, "\t\"beats_per_bar\": %f,\n", time_beats_per_bar);
	fprintf (fp, "\t\"beat_type\": %f,\n", time_beat_type);
	fprintf (fp, "\t\"ticks_per_beat\": %f,\n", time_ticks_per_beat);
	fprintf (fp, "\t\"ticks_beats_per_minute\": %f,\n", time_beats_per_minute);
	
	// quantizer
	fprintf (fp, "\t\"quantizer\": %d,\n", quantizer);

	// song length
	fprintf (fp, "\t\"song_length\": %d,\n", song_length);

	// notes of song
	fprintf (fp, "\t\"notes\": [\n");
	
	for (i = 0; i < song_length; i++) { 
		// opening bracket
		fprintf (fp, "\t\t{\n");

		// note
		fprintf (fp, "\t\t\t\"instrument\": %d,\n", song [i].instrument);
		fprintf (fp, "\t\t\t\"status\": \"%02X\",\n", song [i].status);
		fprintf (fp, "\t\t\t\"key\": \"%02X\",\n", song [i].key);
		fprintf (fp, "\t\t\t\"velocity\": \"%02X\",\n", song [i].vel);
		fprintf (fp, "\t\t\t\"color\": \"%02X\",\n", song [i].color);
		fprintf (fp, "\t\t\t\"bar\": %d,\n", song [i].bar);
		fprintf (fp, "\t\t\t\"beat\": %d,\n", song [i].beat);
		fprintf (fp, "\t\t\t\"tick\": %d,\n", song [i].tick);
		fprintf (fp, "\t\t\t\"qbar\": %d,\n", song [i].qbar);
		fprintf (fp, "\t\t\t\"qbeat\": %d,\n", song [i].qbeat);
		fprintf (fp, "\t\t\t\"qtick\": %d,\n", song [i].qtick);

		// closing bracket; add comma except for last element of the list
		if (i == (song_length - 1)) fprintf (fp, "\t\t}\n");
		else fprintf (fp, "\t\t},\n");
	}

	// closing square bracket (notes of song)
	fprintf (fp, "\t]\n");

	// closing final bracket
	fprintf (fp, "}\n");






	// close file
	fclose (fp);
}


// function called in case user pressed the save pad
// name is a pad number (0-63) which corresponds to the number of the song
// directory is the patch where songs are stored
int save (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	char filename [100];		// temp structure for file name
	const uint8_t instr [8] = {0, 0, 2, 16, 33, 27, 48, 61};		// (drum), piano, elec piano, hammond organ, fingered bass, clean guitar, string ensemble, brass ensemble


	// create file path
	sprintf (filename, "%s/%02x", directory, number);

	// create file in write mode
	fp = fopen (filename, "wt");
	if (fp==NULL) {
		printf ("Cannot write save file: %s\n", filename);
		return 0;
	}

	// opening bracket
	fprintf (fp, "{\n");

	// instruments; first instrument does not count (drum channel)
	fprintf (fp, "\t\"instruments\": [%d, %d, %d, %d, %d, %d, %d, %d]\n", instr [0], instr [1], instr [2], instr [3], instr [4], instr [5], instr [6], instr [7]);

	// tempo values
	fprintf (fp, "\t\"beats_per_bar\": %f,\n", time_beats_per_bar);
	fprintf (fp, "\t\"beat_type\": %f,\n", time_beat_type);
	fprintf (fp, "\t\"ticks_per_beat\": %f,\n", time_ticks_per_beat);
	fprintf (fp, "\t\"ticks_beats_per_minute\": %f,\n", time_beats_per_minute);
	
	// quantizer
	fprintf (fp, "\t\"quantizer\": %d,\n", quantizer);

	// song length
	fprintf (fp, "\t\"song_length\": %d,\n", song_length);

	// notes of song
	fprintf (fp, "\t\"notes\": [\n");
	
	for (i = 0; i < song_length; i++) { 
		// opening bracket
		fprintf (fp, "\t\t{\n");

		// note
		fprintf (fp, "\t\t\t\"instrument\": %d,\n", song [i].instrument);
		fprintf (fp, "\t\t\t\"status\": \"%02X\",\n", song [i].status);
		fprintf (fp, "\t\t\t\"key\": \"%02X\",\n", song [i].key);
		fprintf (fp, "\t\t\t\"velocity\": \"%02X\",\n", song [i].vel);
		fprintf (fp, "\t\t\t\"color\": \"%02X\",\n", song [i].color);
		fprintf (fp, "\t\t\t\"bar\": %d,\n", song [i].bar);
		fprintf (fp, "\t\t\t\"beat\": %d,\n", song [i].beat);
		fprintf (fp, "\t\t\t\"tick\": %d,\n", song [i].tick);
		fprintf (fp, "\t\t\t\"qbar\": %d,\n", song [i].qbar);
		fprintf (fp, "\t\t\t\"qbeat\": %d,\n", song [i].qbeat);
		fprintf (fp, "\t\t\t\"qtick\": %d,\n", song [i].qtick);

		// closing bracket; add comma except for last element of the list
		if (i == (song_length - 1)) fprintf (fp, "\t\t}\n");
		else fprintf (fp, "\t\t},\n");
	}

	// closing square bracket (notes of song)
	fprintf (fp, "\t]\n");

	// closing final bracket
	fprintf (fp, "}\n");

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
