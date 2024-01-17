/** @file utils.h
 *
 * @brief This file defines prototypes of functions inside utils.c
 *
 */


uint8_t midi2bar (uint8_t);
uint8_t bar2midi (uint8_t);
uint8_t instr2chan (uint8_t);
int push_to_list (int, uint8_t *);
int pull_from_list (int, uint8_t *);
int midi_write (void *, jack_nframes_t, jack_midi_data_t *);
void compute_bbt (jack_nframes_t, jack_position_t *, int);
uint32_t quantize (uint32_t, int);
void quantize_song (int, int);
void note2tick (note_t, uint32_t *, int);
void tick2note (uint32_t, note_t *, int);
void set_instrument (int, uint8_t);
void set_instruments (uint8_t *);
void set_volume (int, uint8_t);
void set_volumes (uint8_t *);
int should_play (int);



