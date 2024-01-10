#!/bin/bash
sleep 5
qjackctl -s &
sleep 10
qsynth &
sleep 5
./compo.a
