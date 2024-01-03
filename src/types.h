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
#define LO_BLACK	0xFF	// dummy value processed as black for the UI
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
#define MIDI_CC_MUTE	0x78

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
#define SIXTEENTH		4
#define THIRTY_SECOND	8

/* copy_cut values */
#define COPY		0
#define	CUT			1
#define DEL			2
#define PASTE		3
#define INSERT		4
#define REMOVE		5
#define COLOR		6

/* transpo values */
#define PLUS	1
#define MINUS 	0

/* num pad */
#define NUM_0		0x4B
#define NUM_000		0xC3
#define NUM_DOT		0x4A
#define NUM_1		0x68
#define NUM_2		0x02
#define NUM_3		0x52
#define	NUM_ENTER	0x57
#define NUM_4		0x04
#define NUM_5		0x3E
#define NUM_6		0x05
#define	NUM_BACK	0x07
#define NUM_7		0x06
#define NUM_8		0x03
#define NUM_9		0x53
#define	NUM_PLUS	0x3F
#define NUM_SLASH	0x41
#define NUM_STAR	0x43
#define NUM_MINUS	0x44
/* num pad + numlock (shift) */
#define SNUM_0		0x30
#define SNUM_000	0xC3
#define SNUM_DOT	0x2E
#define SNUM_1		0x31
#define SNUM_2		0x32
#define SNUM_3		0x33
#define	SNUM_ENTER	0x0A
#define SNUM_4		0x34
#define SNUM_5		0x35
#define SNUM_6		0x36
#define	SNUM_BACK	0x07
#define SNUM_7		0x37
#define SNUM_8		0x38
#define SNUM_9		0x39
#define	SNUM_PLUS	0x2B
#define SNUM_SLASH	0x2F
#define SNUM_STAR	0x2A
#define SNUM_MINUS	0x2D

/* types */
// each note consists in this structure : 16 bytes - TBD whether it needs to be optimized
typedef struct {
	uint16_t bar;			// realtime BBT
	uint16_t tick;
	uint8_t beat;
	uint16_t qbar;			// quantized BBT
	uint16_t qtick;
	uint8_t qbeat;
	uint8_t instrument;
	uint8_t status;			// MIDI cmd only, not the channel
	uint8_t key;
	uint8_t vel;
	uint8_t padding [2];	// goal is to make 16 bytes, ie. 2 x 64 bits
} note_t;



