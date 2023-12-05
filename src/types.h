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


/* types */
// each note consists in this structure : 66 bytes - TBD whether it needs to be optimized
typedef struct {
	uint8_t	already_played;
	uint8_t instrument;
	uint16_t bar;
	uint8_t beat;
	uint16_t tick;
	uint8_t status;		// MIDI cmd + channel
	uint8_t key;
	uint8_t vel;
} note_t;




#define MIDI_CLOCK_RATE 96 // 24*4 ticks for full note, 24 ticks per quarter note

#define NB_NAMES 2		// 2 file names: 1 midi file name, 1 SF2 file name
#define FIRST_ELT 0		// used for declarations and loops for filename struct
#define	B0	0
#define	B1	1
#define	B2	2
#define	B3	3
#define	B4	4
#define	B5	5
#define	B6	6
#define	B7	7
#define PLAY	8		// play file
#define LOAD	9		// load files (midi and SF2) to player
#define LAST_ELT 10		// used for declarations and loops

#define NB_FCT 1		// 1 line of functions for midi file: vol -/+, BPM -/+
#define FIRST_ELT_FCT 0		// used for declarations and loops for function struct
#define	VOLDOWN	0
#define	VOLUP	1
#define	BPMDOWN	2
#define	BPMUP	3
#define BEAT	4
#define LAST_ELT_FCT 5		// used for declarations and loops

/* define status, etc */

#define NAMES 0
#define FCT 1

#define CLOCK_PLAY_READY 3
#define	CLOCK_PLAY 2
#define CLOCK 1
#define NO_CLOCK 0

#define FIRST_STATE 0		// used for declarations and loops
#define OFF 0
#define ON 1
#define PENDING	2
#define LAST_STATE 3		// used for declarations and loops

/* list management (used for led mgmt) */
#define LIST_ELT 100

