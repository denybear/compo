/** @file types.h
 *
 * @brief This file defines constants and main global types
 *
 */

/* includes */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>
#include <ncurses.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>
#include <jack/midiport.h>


/* constants */
// colors used by Novation Launchpad mini
#define BLACK		0x0C
#define LO_RED		0x0D
#define HI_RED		0x0F
#define LO_AMBER	0x1D
#define HI_AMBER	0x3F
#define LO_GREEN	0x1C
#define HI_GREEN	0x3C
#define LO_ORANGE	0x1E
#define HI_ORANGE	0x2F
#define LO_YELLOW	0x2D
#define HI_YELLOW	0x3E

// midi messages definition
#define MIDI_NOTEON		0x90
#define MIDI_NOTEOFF	0x80
#define MIDI_CC			0xB0
#define MIDI_PC			0xC0
#define MIDI_1BYTE		0xF0
#define MIDI_CLOCK		0xF8
#define MIDI_RESERVED	0xF9
#define MIDI_PLAY		0xFA
#define MIDI_STOP		0xFC

/* define status, etc */
#define TRUE	1
#define FALSE	0
#define UI		0
#define KBD		1
#define OUT		2
#define SONG_SIZE	10000		// max number of notes for a song
#define COPY_SIZE	1000		// max number of notes for copy buffer

/* list management (used for led mgmt) */
#define LIST_ELT 100

/* quantizer values */
#define FREE_TIMING		0
#define	QUARTER			1
#define EIGHTH			2
#define SIXTEENTH		3
#define THIRTY_SECOND	4

/* copy_cut values */
#define COPY		0
#define	CUT			1
#define DEL			2

/* types */
// each note consists in this structure : 66 bytes - TBD whether it needs to be optimized
typedef struct {
	uint8_t	already_played;
	uint8_t instrument;
	uint16_t bar;
	uint8_t beat;
	uint16_t tick;
	uint8_t status;		// MIDI cmd only, not the channel
	uint8_t key;
	uint8_t vel;
} note_t;



