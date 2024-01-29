#!/bin/bash

LOGFILE=/home/pi/log
export TERM="xterm-256color"

(
	sleep 5
	qjackctl -s &
	sleep 10
	qsynth &
	sleep 5
	/home/pi/compo.a
) >& $LOGFILE
