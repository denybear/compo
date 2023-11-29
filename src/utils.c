/** @file process.c
 *
 * @brief The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 */

#include "types.h"
#include "globals.h"
#include "process.h"
#include "utils.h"
#include "led.h"


// determines if 2 midi messages (ie. events) are the same; returns TRUE if yes
int same_event (unsigned char * evt1, unsigned char * evt2) {

	if ((evt1[0]==evt2[0]) && (evt1[1]==evt2[1])) return TRUE;
	return FALSE;
}


// add led request to the list of requests to be processed
int push_to_list (int dest, int row, int col, int on_off) {

	// add to list
	list_buffer [list_index][0] = (unsigned char) dest;
	list_buffer [list_index][1] = (unsigned char) row;
	list_buffer [list_index][2] = (unsigned char) col;
	list_buffer [list_index][3] = (unsigned char) on_off;

	// increment index and check boundaries
	//list_index = (list_index >= LIST_ELT) ? 0; list_index++;
	if (list_index >= LIST_ELT) {
		fprintf ( stderr, "too many led requests in the list.\n" );
		list_index = 0;
	}
	else list_index++;
}


// pull out led request from the list of requests to be processed (FIFO style)
// returns 0 if pull request has failed (nomore request to be pulled out)
int pull_from_list (int *dest, int *row, int *col, int *on_off) {

	// check if we have requests to be pulled; if not, leave
	if (list_index == 0) return 0;

	// remove first element from list
	*dest = list_buffer [0][0];
	*row = list_buffer [0][1];
	*col = list_buffer [0][2];
	*on_off = list_buffer [0][3];

	// decrement index
	list_index--;

	// move the rest of the list 1 item backwards
	memmove (&list_buffer[0][0], &list_buffer[1][0], list_index * 4);

	return 1;
}


// compute BBT based on nframes, tempo (BPM), frame rate, etc.
// based on new_pos value:
// 0 : compute BBT based on previous values of BBT & fram rate
// 1 : compute BBT based on start of Jack
// 2 : compute BBT, and sets position as 1, 1, 1
// requires jack_position_t * which will contain the BBT information
void compute_bbt (jack_nframes_t nframes, jack_position_t *pos, int new_pos)
{
	double min;			/* minutes since frame 0 */
	long abs_tick;		/* ticks since frame 0 */
	long abs_beat;		/* beats since frame 0 */

	if (new_pos) {
		pos->valid = JackPositionBBT;
		pos->beats_per_bar = time_beats_per_bar;
		pos->beat_type = time_beat_type;
		pos->ticks_per_beat = time_ticks_per_beat;
		pos->beats_per_minute = time_beats_per_minute;

		if (new_pos == 2) {
			// set BBT to 1,1,1
			pos->bar = 1;
			pos->beat = 1;
			pos->tick = 1;
			pos->bar_start_tick = 1;
		}
		else {

			/* Compute BBT info from frame number.  This is
			 * relatively simple here, but would become complex if
			 * we supported tempo or time signature changes at
			 * specific locations in the transport timeline.  I
			 * make no claims for the numerical accuracy or
			 * efficiency of these calculations. */

			min = pos->frame / ((double) pos->frame_rate * 60.0);
			abs_tick = min * pos->beats_per_minute * pos->ticks_per_beat;
			abs_beat = abs_tick / pos->ticks_per_beat;

			pos->bar = abs_beat / pos->beats_per_bar;
			pos->beat = abs_beat - (pos->bar * pos->beats_per_bar) + 1;
			pos->tick = abs_tick - (abs_beat * pos->ticks_per_beat);
			pos->bar_start_tick = pos->bar * pos->beats_per_bar * pos->ticks_per_beat;
			pos->bar++;		/* adjust start to bar 1 */
		}
	}
	else {

		/* Compute BBT info based on previous period. */
		pos->tick += (nframes * pos->ticks_per_beat * pos->beats_per_minute / (pos->frame_rate * 60));

		while (pos->tick >= pos->ticks_per_beat) {
			pos->tick -= pos->ticks_per_beat;
			if (++pos->beat > pos->beats_per_bar) {
				pos->beat = 1;
				++pos->bar;
				pos->bar_start_tick += (pos->beats_per_bar * pos->ticks_per_beat);
			}
		}
	}
printf ("%d, %d, %d, %d\n", pos->bar, pos->beat, pos->tick, pos->frame_rate);
}


