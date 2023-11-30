/** @file utils.h
 *
 * @brief This file defines prototypes of functions inside utils.c
 *
 */


int same_event (unsigned char *, unsigned char *);
uint8_t midi2bar (uint8_t);
uint8_t bar2midi (uint8_t);
int push_to_list (int, uint8_t *);
int pull_from_list (int, uint8_t *);
void compute_bbt (jack_nframes_t, jack_position_t *, int);
void create_quantization_table (uint32_t *, int);


