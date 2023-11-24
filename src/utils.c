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

