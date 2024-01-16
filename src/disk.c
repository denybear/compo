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
#include "midiwriter.h"



// function called in case user pressed the load pad
// name is a pad number (0-63) which corresponds to the number of the song
// directory is the patch where songs are stored
int load (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	char filename [100];		// temp structure for file name
	uint8_t instr [8];			// (drum), piano, elec piano, hammond organ, fingered bass, clean guitar, string ensemble, brass ensemble
	char buffer [50000];		// buffer containing json data
	int len;
	cJSON *json;				// used for CJSON writing
	cJSON *instruments = NULL;
	cJSON *notes = NULL;
	cJSON *note = NULL;
	cJSON *data = NULL;			// temp data
	char *json_str;				// string where json is stored
	int status = 0;
	char *error_ptr;

	// create file path
	sprintf (filename, "%s/%02x", directory, number);

	// create file in write mode
	fp = fopen (filename, "rt");
	if (fp==NULL) {
		printf ("Cannot read save file: %s\n", filename);
		return 2;
	}

	// read and close file
	len = fread(buffer, 1, sizeof(buffer), fp); 
	fclose(fp); 
  
	// parse the JSON data 
	status = 1;
	json = cJSON_Parse(buffer); 
	if (json == NULL) { 
		error_ptr = cJSON_GetErrorPtr(); 
		if (error_ptr != NULL) { 
			printf("Error in JSON before: %s\n", error_ptr); 
		}
		goto end;
    } 

	// instruments; first instrument does not count (drum channel)
    instruments = cJSON_GetObjectItemCaseSensitive(json, "instruments");
	for (i = 0; i < 8; i++) {
		instrument = cJSON_GetArrayItem(instruments, i);
		if (instrument == NULL) goto end;
		instr [i] = instrument->valueint;
	}

	// tempo values
	data = cJSON_GetObjectItemCaseSensitive (json, "beats_per_bar");
	if (data == NULL) goto end;
	time_beats_per_bar = data->valuedouble;

	data = cJSON_GetObjectItemCaseSensitive (json, "beat_type");
	if (data == NULL) goto end;
	time_beat_type = data->valuedouble;

	data = cJSON_GetObjectItemCaseSensitive (json, "ticks_per_beat");
	if (data == NULL) goto end;
	time_ticks_per_beat = data->valuedouble;

	data = cJSON_GetObjectItemCaseSensitive (json, "ticks_beats_per_minute");
	if (data == NULL) goto end;
	time_beats_per_minute = data->valuedouble;

	// quantizer
	data = cJSON_GetObjectItemCaseSensitive (json, "quantizer");
	if (data == NULL) goto end;
	quantizer = data->valueint;

	// song length
	data = cJSON_GetObjectItemCaseSensitive (json, "song_length");
	if (data == NULL) goto end;
	song_length = data->valueint;

	// notes of song
    notes = cJSON_GetObjectItemCaseSensitive(json, "notes");
	if (notes == NULL) goto end;

	i = 0;
    cJSON_ArrayForEach(note, notes) {

		data = cJSON_GetObjectItemCaseSensitive (note, "instrument");
		if (data == NULL) goto end;
		song [i].instrument = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "status");
		if (data == NULL) goto end;
		song [i].instrument = data->status;

		data = cJSON_GetObjectItemCaseSensitive (note, "key");
		if (data == NULL) goto end;
		song [i].instrument = data->key;

		data = cJSON_GetObjectItemCaseSensitive (note, "velocity");
		if (data == NULL) goto end;
		song [i].instrument = data->vel;

		data = cJSON_GetObjectItemCaseSensitive (note, "color");
		if (data == NULL) goto end;
		song [i].instrument = data->color;

		data = cJSON_GetObjectItemCaseSensitive (note, "bar");
		if (data == NULL) goto end;
		song [i].instrument = data->bar;

		data = cJSON_GetObjectItemCaseSensitive (note, "qbar");
		if (data == NULL) goto end;
		song [i].instrument = data->qbar;

		data = cJSON_GetObjectItemCaseSensitive (note, "beat");
		if (data == NULL) goto end;
		song [i].instrument = data->beat;

		data = cJSON_GetObjectItemCaseSensitive (note, "qbeat");
		if (data == NULL) goto end;
		song [i].instrument = data->qbeat;

		data = cJSON_GetObjectItemCaseSensitive (note, "tick");
		if (data == NULL) goto end;
		song [i].instrument = data->tick;

		data = cJSON_GetObjectItemCaseSensitive (note, "qtick");
		if (data == NULL) goto end;
		song [i].instrument = data->qtick;

		i++;
	}

	// make sure we have as many notes as defined in song_length
	if (i != song_length) goto end;	

	// everything went well
	status = 0;

end:
    cJSON_Delete(json);
    return status;
}
	

// function called in case user pressed the save pad
// name is a pad number (0-63) which corresponds to the number of the song
// directory is the patch where songs are stored
int save (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	char filename [100];		// temp structure for file name
	const uint8_t instr [8] = {0, 0, 2, 16, 33, 27, 48, 61};		// (drum), piano, elec piano, hammond organ, fingered bass, clean guitar, string ensemble, brass ensemble
	cJSON *json;				// used for CJSON writing
	cJSON *instruments = NULL;
	cJSON *notes = NULL;
	cJSON *note = NULL;
	char *json_str;				// string where json is stored
	int status;

	// create a cJSON object 
	status = 1;
	json = cJSON_CreateObject(); 

	// instruments; first instrument does not count (drum channel)
	instruments = cJSON_CreateIntArray (instr, 8);
	if (instruments == NULL) goto end;
	if (cJSON_AddItemToObject(json, "instruments", instruments) == NULL) goto end;

	// tempo values
	if (cJSON_AddNumberToObject(json, "beats_per_bar", time_beats_per_bar) == NULL) goto end;
	if (cJSON_AddNumberToObject(json, "beat_type", time_beat_type) == NULL) goto end;
	if (cJSON_AddNumberToObject(json, "ticks_per_beat", time_ticks_per_beat) == NULL) goto end;
	if (cJSON_AddNumberToObject(json, "ticks_beats_per_minute", time_beats_per_minute) == NULL) goto end;

	// quantizer
	if (cJSON_AddNumberToObject(json, "quantizer", quantizer) == NULL) goto end;

	// song length
	if (cJSON_AddNumberToObject(json, "song_length", song_length) == NULL) goto end;

	// notes of song
	notes = cJSON_AddArrayToObject(json, "notes");
	if (notes == NULL) goto end;

	for (i = 0; i < song_length; i++) { 

		note = cJSON_CreateObject();
		if (cJSON_AddNumberToObject(note, "instrument", song [i].instrument) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "status", song [i].status) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "key", song [i].key) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "velocity", song [i].vel) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "color", song [i].color) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "bar", song [i].bar) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "qbar", song [i].qbar) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "beat", song [i].beat) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "qbeat", song [i].qbeat) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "tick", song [i].tick) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "qtick", song [i].qtick) == NULL) goto end;
		cJSON_AddItemToArray(notes, note);
    }

	// convert the cJSON object to a JSON string 
	json_str = cJSON_Print(json); 

	// create file path
	sprintf (filename, "%s/%02x", directory, number);

	// create file in write mode
	fp = fopen (filename, "wt");
	if (fp==NULL) {
		printf ("Cannot write save file: %s\n", filename);
		status = 2;
		goto end;
	}
 
	// write and close file
	fputs(json_str, fp); 
	fclose (fp);

	// everything went well
	status = 0;

end:
	// free the JSON string and cJSON object 
	cJSON_free (json_str);
    cJSON_Delete (json);
	return status;
}


int save_to_midi (uint8_t name, char * directory) {
	
	FILE *out;
	int32_t trackSize = 0;
	uint8_t trackByte[4] = {0};
	int	PPQN;
	int i;

	// create file path
	sprintf (filename, "%s/%02x.mid", directory, number);

	// create file in write mode
	out = fopen (filename, "wb");
	if (out==NULL) {
		printf ("Cannot write midi file: %s\n", filename);
		return 2;
	}

	// set ppqn
	PPQN = (int) ticks_per_beat;

	// write header
	WriteMidiHeader(PPQN, out);
	WriteTrackHeader(out);
	for(int i=0; i<4; i++) {
		fwrite(trackByte+i, sizeof(uint8_t), 1, out);		// dummy track size (all 0) for now
	}

//****HERE
	
	trackSize += WriteTrackName(out);
	trackSize += WriteSysEx_GS_Reset(out);
	trackSize += WriteControlChange(0, 9, 7, 100, out);
	trackSize += WriteControlChange(0, 9, 0, 0, out);
	trackSize += WriteControlChange(0, 9, 32, 0, out);
	trackSize += WriteProgramChange(0, 9, 80, out);
	trackSize += WriteTempo(0, 120, out);
	
	int offset = 0;
	bool add = true;
	srand(time(NULL));
	for(int i=0; i<256; i++) {
		offset = rand()%25;
		trackSize += WriteNote(0, 9, 35+offset, 100, out);
		
		trackSize += WriteNote(PPQN/4, 9, 35+offset, 0, out);
		
		/*
		if(offset == 16) add = false;
		else if(offset == 0) add = true;
		offset += (add) ? 1 : -1;
		*/
	}
	trackSize += WriteTrackEnd(out);

	// write final track size
	GetBytes(trackByte, trackSize, 4);
	fseek(out, 18, SEEK_SET);
	for(int i=0; i<4; i++) {
		fwrite(trackByte+i, sizeof(uint8_t), 1, out);
	}
	fclose(out);
	
	return 0;
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
