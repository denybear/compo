/** @file song.h
 *
 * @brief This file defines prototypes of functions inside song.c
 *
 */


void write_to_song (note_t, int);
note_t* read_from_song (u_int16_t, u_int16_t, u_int16_t, u_int16_t, int*, int);
note_t* read_from_metronome (u_int16_t, u_int16_t, u_int16_t, u_int16_t, int*, int);
note_t* read_from (note_t*, int, u_int16_t, u_int16_t, u_int16_t, u_int16_t, int*, int);
void test_copy_paste ();
void test_write ();
void test_read ();
void display_song (int, note_t *, char *);
void copy_cut (u_int16_t, u_int16_t, int, int);
void copy (u_int16_t, u_int16_t, int);
void cut (u_int16_t, u_int16_t, int);
void paste (u_int16_t, int);
void create_metronome ();






