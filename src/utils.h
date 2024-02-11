/** @file utils.h
 *
 * @brief This file defines prototypes of functions inside utils.c
 *
 */


uint8_t midi2bar (uint8_t);
uint8_t bar2midi (uint8_t);
uint8_t instr2chan (uint8_t, int);
int is_drum (uint8_t, int);
int push_to_list (int, uint8_t *);
int pull_from_list (int, uint8_t *);
int midi_write (void *, jack_nframes_t, jack_midi_data_t *);
int compute_bbt (jack_nframes_t, jack_position_t *, int);
uint32_t quantize (uint32_t, int);
uint32_t min_time (int);
int quantize_note (int, int, note_t *);
void note2tick (note_t, uint32_t *, int);
void tick2note (uint32_t, note_t *, int);
void set_instrument (int, int);
void set_instruments ();
void set_volume (int, int);
void set_volumes ();
int should_play (int);
void set_defaults ();

