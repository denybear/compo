#!/bin/bash

LOGFILE=/home/pi/log
export TERM="xterm-256color"

(
	sleep 5
	qjackctl -s &
	sleep 10
	qsynth &
#	fluidsynth -C0 -R0 -a jack -m jack /home/pi/soundfonts/00_FluidR3_GM.sf2 &
#	jack_connect fluidsynth-midi:left system:playback_1
#	jack_connect fluidsynth-midi:right system:playback_2
	sleep 5
	x-terminal-emulator -e /home/pi/compo.a

) >& $LOGFILE
