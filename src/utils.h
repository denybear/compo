/** @file utils.h
 *
 * @brief This file defines prototypes of functions inside utils.c
 *
 */


int same_event (unsigned char *, unsigned char *);
int push_to_list (int, int , int , int);
int pull_from_list (int *, int *, int *, int *);
void compute_bbt (jack_nframes_t, jack_position_t *, int);

