#!/bin/bash
sleep 10
export DISPLAY=:0
export HOME=/home/pi
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
sudo pigpiod &
sleep 1

# get connected devices
nanokontrol=$( amidi -l | grep nanoKONTROL2 | awk -F ' ' '{print $2}')
launchpad=$( amidi -l | grep Launchpad | awk -F ' ' '{print $2}')
headphones=$( aplay -L | grep plughw:CARD=Headphones | awk -F ',' '{print $1}')
io2=$( aplay -L | grep plughw:CARD=io2 | awk -F ',' '{print $1}')

echo "nanokontrol : $nanokontrol"
echo "launchpad: $launchpad"
echo "headphones: $headphones"
echo "io2: $io2"

if [ -z "$io2" ];
then
	soundcard=$headphones
	device="hw:Headphones"
else
	soundcard=$io2
	device="hw:io2"
fi

if [ -n "$nanokontrol" ];
then
# launch syntwo
	/home/pi/syntwo.a $soundcard $nanokontrol
else
# launch synthi + boocli
	export JACK_NO_AUDIO_RESERVATION=1
	jackd --realtime --realtime-priority 70 --port-max 30 --silent -d alsa --device $device --nperiods 3 --rate 48000 --period 128 &
	sleep 5
	a2jmidid -ue &
	sleep 5
	/home/pi/boocli/boocli.a /home/pi/boocli/boocli.cfg &
	sleep 5
	/home/pi/synthi/synthi.a /home/pi/synthi/synthi.cfg
fi

