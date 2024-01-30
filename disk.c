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


// in the given directory, look for all the files named between 0x00 and 0x40
// fill a table containing TRUE or FALSE, depending whether the file is there or not
int get_files_in_directory (char * directory, uint8_t * table) {

	DIR *dir;
	struct dirent *ent;
	char st1[64][3];
	int i;

	// empty recipient table
	memset (table, FALSE, 64);

	// build temp table for storing file names first
	for (i = 0; i < 64; i++) {
		// convert i into string (in lowercases and uppercases)
		sprintf (st1 [i], "%02X", i);
	}

	if ((dir = opendir (directory)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			// check if file starts with number; allow mixing lower and upper cases
			for (i = 0; i < 64; i++) {
				if ((st1[i][0] == ent->d_name [0])  && (st1[i][1] == ent->d_name [1])) {
					table [i] = TRUE;
				}
			}
		}
		// processing done, close dir
		closedir (dir);
		return TRUE;
	}

	else {
		// could not open directory
		fprintf ( stderr, "Directory not found.\n" );
		return FALSE;
	}
}


// function called in case user pressed the load pad
// name is a pad number (0-63) which corresponds to the number of the song
// directory is the patch where songs are stored
int load (uint8_t name, char * directory) {

	FILE *fp;
	int i;
	char filename [100];		// temp structure for file name
	char buffer [50000];		// buffer containing json data
	int len;
	cJSON *json;				// used for CJSON writing
	cJSON *instruments = NULL;
	cJSON *volumes = NULL;
	cJSON *notes = NULL;
	cJSON *note = NULL;
	cJSON *data = NULL;			// temp data
	char *json_str;				// string where json is stored
	int status = 0;
	const char *error_ptr;

	// create file path
	sprintf (filename, "%s/%02X.json", directory, name);

	// create file in write mode
	fp = fopen (filename, "rt");
	if (fp==NULL) {
		fprintf ( stderr, "Cannot read save file: %s\n", filename);
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
			fprintf ( stderr, "Error in JSON before: %s\n", error_ptr); 
		}
		goto end;
    } 

	// instruments; first instrument does not count (drum channel)
    instruments = cJSON_GetObjectItemCaseSensitive(json, "instruments");
	for (i = 0; i < 8; i++) {
		data = cJSON_GetArrayItem(instruments, i);
		if (data == NULL) goto end;
		instrument_list [i] = data->valueint;
	}

	// volumes; first instrument does not count (drum channel)
    volumes = cJSON_GetObjectItemCaseSensitive(json, "volumes");
	for (i = 0; i < 8; i++) {
		data = cJSON_GetArrayItem(volumes, i);
		if (data == NULL) goto end;
		volume_list [i] = data->valueint;
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

	data = cJSON_GetObjectItemCaseSensitive (json, "time_bpm_multiplier");
	if (data == NULL) goto end;
	time_bpm_multiplier = data->valuedouble;

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
		song [i].status = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "key");
		if (data == NULL) goto end;
		song [i].key = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "velocity");
		if (data == NULL) goto end;
		song [i].vel = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "color");
		if (data == NULL) goto end;
		song [i].color = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "bar");
		if (data == NULL) goto end;
		song [i].bar = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "beat");
		if (data == NULL) goto end;
		song [i].beat = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "tick");
		if (data == NULL) goto end;
		song [i].tick = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "qbar");
		if (data == NULL) goto end;
		song [i].qbar = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "qbeat");
		if (data == NULL) goto end;
		song [i].qbeat = data->valueint;

		data = cJSON_GetObjectItemCaseSensitive (note, "qtick");
		if (data == NULL) goto end;
		song [i].qtick = data->valueint;

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
	cJSON *json;				// used for CJSON writing
	cJSON *instruments = NULL;
	cJSON *volumes = NULL;
	cJSON *notes = NULL;
	cJSON *note = NULL;
	char *json_str;				// string where json is stored
	int status;

	// create a cJSON object 
	status = 1;
	json = cJSON_CreateObject(); 

	// instruments; first instrument does not count (drum channel)
	instruments = cJSON_CreateIntArray ((const int *)instrument_list, 8);
	if (instruments == NULL) goto end;
	if (cJSON_AddItemToObject(json, "instruments", instruments) == FALSE) goto end;

	// volumes
	volumes = cJSON_CreateIntArray ((const int *)volume_list, 8);
	if (volumes == NULL) goto end;
	if (cJSON_AddItemToObject(json, "volumes", volumes) == FALSE) goto end;

	// tempo values
	if (cJSON_AddNumberToObject(json, "beats_per_bar", time_beats_per_bar) == NULL) goto end;
	if (cJSON_AddNumberToObject(json, "beat_type", time_beat_type) == NULL) goto end;
	if (cJSON_AddNumberToObject(json, "ticks_per_beat", time_ticks_per_beat) == NULL) goto end;
	if (cJSON_AddNumberToObject(json, "ticks_beats_per_minute", time_beats_per_minute) == NULL) goto end;
	if (cJSON_AddNumberToObject(json, "time_bpm_multiplier", time_bpm_multiplier) == NULL) goto end;

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
		if (cJSON_AddNumberToObject(note, "beat", song [i].beat) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "tick", song [i].tick) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "qbar", song [i].qbar) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "qbeat", song [i].qbeat) == NULL) goto end;
		if (cJSON_AddNumberToObject(note, "qtick", song [i].qtick) == NULL) goto end;
		cJSON_AddItemToArray(notes, note);
    }

	// convert the cJSON object to a JSON string 
	json_str = cJSON_Print(json); 

	// create file path
	sprintf (filename, "%s/%02X.json", directory, name);

	// create file in write mode
	fp = fopen (filename, "wt");
	if (fp==NULL) {
		fprintf ( stderr, "Cannot write save file: %s\n", filename);
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

// export to midi
// if quant is FALSE, then timing values are non-quantized; else, they are quantized
int save_to_midi (uint8_t name, char * directory, int quant) {
	
	FILE *out;
	char filename [100];		// temp structure for file name
	int32_t trackSize = 0;
	uint8_t trackByte[4] = {0};
	int	chan, vel;
	int i;
	uint32_t previous_tick, tick, delta;

	// create file path
	if (quant==FALSE) sprintf (filename, "%s/%02X.mid", directory, name);
	else sprintf (filename, "%s/%02Xq.mid", directory, name);

	// create file in write mode
	out = fopen (filename, "wb");
	if (out==NULL) {
		fprintf ( stderr, "Cannot write midi file: %s\n", filename);
		return 2;
	}

	// write header
	WriteMidiHeader((int) time_ticks_per_beat, out);				// PPQN
	WriteTrackHeader(out);
	for(int i=0; i<4; i++) {
		fwrite(trackByte+i, sizeof(uint8_t), 1, out);		// write dummy track size (all 0) for now
	}

	trackSize += WriteTrackName(out);
	// trackSize += WriteSysEx_GS_Reset(out);						// use instruments above 127 (above GM)
	// trackSize += WriteControlChange(0, 9, 0, 0, out);			// MSB - LSB of instrument
	// trackSize += WriteControlChange(0, 9, 32, 0, out);

	// set tempo
	trackSize += WriteTempo(0, (int) time_beats_per_minute, out);

	//set instruments for each channel
	for (i = 0; i < 8; i++) {
		chan = instr2chan (i, MIDI_EXPORT);
		if (is_drum (i, MIDI_EXPORT) == FALSE) {
			// non-drum instruments will get program change
			trackSize += WriteProgramChange(0, chan, instrument_list [i], out);			// instrument change
		}
	}

	//set volumes for each channel
	for (i = 0; i < 8; i++) {
		chan = instr2chan (i, MIDI_EXPORT);
		trackSize += WriteControlChange(0, chan, 0x07, volume_list [i], out);			// instrument change
	}

	// write notes of the song
	previous_tick = 0;	// first event happens at timing 0
	for (i = 0; i < song_length; i++) {
	
		// determine velocity depending on note-on or note-off
		if (song [i].status == MIDI_NOTEON) vel = song [i].vel;
		else vel = 0;			// note-off can be defined as note-on with 0 velocity

		// determine channel
		chan = instr2chan (song [i].instrument, MIDI_EXPORT);

		// determine delta time between midi event and previous midi event
		// we use quant parameter to determine whether we should use quantized values or not
		note2tick (song [i], &tick, quant);			// number of ticks from BBT (0,0,0)
		if (previous_tick > tick) {
			fprintf ( stderr, "Error in generating MIDI file : negative delta time\n");		// error message in case delta time is negative
			tick = previous_tick;													// set delta time to 0 in this case
		}
		delta = tick - previous_tick;				// compute delta time between 2 events; in theory, delta should always be > 0 as note events are sorted by time
		previous_tick = tick;

		// write note
		trackSize += WriteNote(delta, chan, song [i].key, vel, out);
	}

	// write track end
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
