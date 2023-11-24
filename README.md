# synthi: the midi file player companion of boocli
In the case you don't know boocli, please refer to: [boocli wiki](https://github.com/denybear/boocli/wiki)


synthi is a midi file player. It is designed to work side-by-side with boocli, and provides the following functionalities:
* Based on famous Fluidsynth engine, allowing: Support of midi (.mid) files and of SF2 soundfont files
* Midi files and SF2 files are stored on Raspi SD card
* Supports upto 255 different midi files, upto 15 different SF2 files
* Headless, no screen required: Midi-driven UI (for example through a midi control surface such as Novation launchpad)
* Works seamlessly with boocli, as a replacement of an external groovebox: synthi plays the file and sends clock indication to boocli; boocli uses the clock indication to synchronize the loops
* Generates midi clock signal while playing midi file
* Supports volume -/+ while playing midi file
* Support BPM -/+ while playing midi file
* Optional support of "beat" button via MIDI or GPIO; this button allows to adjust rhythm when playing with a live band, as rhythm can fluctuate a bit
* All this using a simple Raspberry 3B and above!

The big benefit of synthi is simplification while using boocli.
While boocli requires a Raspi, a capture/sound card, a midi control surface (for UI) and an external groovebox, synthi removes the need for external groovebox and related cabling.
Thanks to synthi, the Raspi IS the groovebox! 

