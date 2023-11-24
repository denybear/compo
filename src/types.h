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
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>
#include <jack/midiport.h>


/* constants */
#define MIDI_CLOCK 0xF8
#define MIDI_RESERVED 0xF9
#define MIDI_PLAY 0xFA
#define MIDI_STOP 0xFC
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
#define TRUE 1
#define FALSE 0

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


/* types */
typedef struct {						// structure for each of the 2 names
	unsigned char ctrl [LAST_ELT] [2];	//controls on the midi control surface
	unsigned char led [LAST_ELT] [LAST_STATE] [3];		// led lightings on the midi control surface (off, pending, on...)
	unsigned char status [LAST_ELT];	// Status byte for each function
} filename_t;

typedef struct {						// structure for each of the 2 names
	unsigned char ctrl [LAST_ELT_FCT] [2];	//controls on the midi control surface
	unsigned char led [LAST_ELT_FCT] [LAST_STATE] [3];		// led lightings on the midi control surface (off, pending, on...)
	unsigned char status [LAST_ELT_FCT];	// Status byte for each function
} filefunct_t;

