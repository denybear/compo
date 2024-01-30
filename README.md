# compo: Music composer based on Akai MPK Mini and Novation Launchpad Mini   
The goal of the project is to provide a sort of "notepad" to write music quickly and efficiently.   
In the music creation process, it can be used in the first stages to write down ideas and building songs rapidly.   

Main functionalities:
* based on JACK audio server
* 8 midi tracks
* ability to select 1 instrument per track
* song size: 64 bars x 8 = 512 bars per track
* midi output; any midi synthetizer can be used to render audio
* midi-clock out (including midi clock, midi start, midi stop)
* no effects supported: goal is to write 'dry' music; effects can be done on a DAW at a later stage
* export to midi file functionality
* play, record (overdub)
* solo, mute, volume change per track
* metronome, tap-tempo, tempo -/+
* copy, cut, paste, insert, remove bars
* velocity support, quantization (somehow limited), transposition -/+
   
It features a Raspberry PI, a midi keyboard (Akai MPK Mini) and a midi control surface (Novation Launchpad mini).   

Check out the wiki pages for more details.
