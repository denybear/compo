from mido import MidiFile
import sys
import collections as col
import json
import os

gm_instruments = [
	"Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano",
	"Honky-tonk Piano", "Electric Piano 1", "Electric Piano 2", "Harpsichord",
	"Clavinet", "Celesta", "Glockenspiel", "Music Box", "Vibraphone",
	"Marimba", "Xylophone", "Tubular Bells", "Dulcimer", "Drawbar Organ",
	"Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ",
	"Accordion", "Harmonica", "Tango Accordion", "Acoustic Guitar (nylon)",
	"Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)",
	"Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar",
	"Guitar Harmonics", "Acoustic Bass", "Electric Bass (finger)",
	"Electric Bass (pick)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2",
	"Synth Bass 1", "Synth Bass 2", "Violin", "Viola", "Cello", "Contrabass",
	"Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
	"String Ensemble 1", "String Ensemble 2", "SynthStrings 1", "SynthStrings 2",
	"Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit", "Trumpet",
	"Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section",
	"SynthBrass 1", "SynthBrass 2", "Soprano Sax", "Alto Sax", "Tenor Sax",
	"Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet", "Piccolo",
	"Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle",
	"Ocarina", "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)",
	"Lead 4 (chiff)", "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)",
	"Lead 8 (bass + lead)", "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)",
	"Pad 4 (choir)", "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)",
	"Pad 8 (sweep)", "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)",
	"FX 4 (atmosphere)", "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)",
	"FX 8 (sci-fi)", "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba",
	"Bagpipe", "Fiddle", "Shanai", "Tinkle Bell", "Agogo", "Steel Drums",
	"Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
	"Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring",
	"Helicopter", "Applause", "Gunshot"
]


def checkUsedChannels (song):

	nbNotes = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]	# 0 to 16, to correspond to 16 channels
	usedChannels = []

	for nt in song:													# check whether channel is used or not
		nbNotes [nt.channel] = nbNotes [nt.channel] + 1 			# indicate that the channel is used
	
	# check unused channels
	for i in range (1, 17):
		if i == 10:				# remove channel 10 from the list... we will include it back later (we assume it is present all the time)
			continue
		if nbNotes [i] != 0:
			print ('channel:', i, '- USED - instrument:', instruments [i], gm_instruments [instruments [i]], '- volume:', volumes [i], '- nb notes:', nbNotes [i])
			usedChannels.append (i)
		else:
			print ('channel:', i, '- UNUSED')

	print ('')
	return usedChannels, nbNotes


def sortSong (song):

	newSong = []
	
	# sort the song ascending by qbar, qbeat, qtick; note-on first, then note-off
	for note in song:

	# fill temp variables (for easier reading)
		note_bar = note.qbar;
		note_tick = note.qtick;
		note_status = note.status;

		idx = 0
		while (idx < len (newSong)):

			newNote = newSong [idx]				# we are adding 'note' to newSong made of 'newNotes'

			# check the bar first
			if (newNote.bar < note.bar):
				idx = idx + 1
				continue						# if bar in song is too small, then next loop
			if (newNote.bar > note.bar):
				break							# if bar in song is too high, then leave loop

			# note bar == song bar
			# check the tick
			if (newNote.tick < note.tick):
				idx = idx + 1
				continue						# if tick in song is too small, then next loop
			if (newNote.tick > note.tick):
				break							# if tick in song is too high, then leave loop

			# note tick == song tick
			# check the status
			# we want to sort this way: NOTE_ON (0x90), then NOTE_OFF (0x80)
			if (newNote.status > note.status):
				idx = idx + 1
				continue						# if note status is higher (ie. note on found when you have note off to insert) then next loop
			break								# leave loop

		# at this stage, idx corresponds to where we should add the note
		newSong.insert (idx, note)
	
	return newSong



class note:

	def __init__(self, msg, time, tempo):

		# compute new timing
		time = time + (int) (msg ['time'] * 1000000)			# time is in usec
		nbBeatsSinceBeginning = (int) (time / tempo)
		nbBars = (int) (nbBeatsSinceBeginning / 4)		# assume 4/4
		nbBeats = nbBeatsSinceBeginning % 4				# assume 4/4
		
		nbTicks = (int) (time % (tempo * 4))					# time % tempo is the number of ticks in a beat; we want number of ticks in a bar 
		nbTicks = (int) ((nbTicks * 480) / tempo)				# assume PPQN is 480
		#nbTicks = (int) ((nbTicks * 480 * 4) / (tempo * 4))	# this is the true formula... but can be simplified by removing 4

#		print ('time:', time, 'nbBeatsSince:', nbBeatsSinceBeginning, 'bar:', nbBars, 'beat:', nbBeats, 'tick:', nbTicks)

		self.bar = nbBars
		self.beat = nbBeats
		self.tick = nbTicks
		self.qbar = self.bar
		self.qbeat = self.beat
		self.qtick = self.tick

		if msg ['type'] == 'note_on':
			self.status = 0x90			# note on
		else:
			self.status = 0x80			# note off
		self.key = msg ['note']
		self.vel = msg ['velocity']
		if self.vel == 0:				# in case note-on + velocity = 0, then this is a note-off
			self.status = 0x80			# note off

		self.channel = msg ['channel']	# for the moment, we set channel number as instrument

		self.color = 0x2D



#####################
# BEGINNING OF MAIN #
#####################

instruments = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]		# 0 to 16, to correspond to 16 channels
volumes = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]			# 0 to 16, to correspond to 16 channels



# check if filename is present in the request
if len (sys.argv) >= 2:
	fileName = sys.argv[1]

	# init default variables
	tempo = 500000				# time in usec required for 1 beat : 500000 us = 500 ms = 0.5 s = 120 bpm
	time = 0
	song = []
	tempoSet = False

	# open midifile and get message by message
	for mid in MidiFile (fileName):

		# parse & print message
		msg = mid.dict ()
#		print (msg)

		# note-on or note-off
		if (msg ['type'] == 'note_on') or (msg ['type'] == 'note_off'):
			# add note to list of notes
#			print (msg)
			song.append (note (msg, time, tempo))


		# tempo
		if msg ['type'] == 'set_tempo':
			if tempoSet:
				print ('Warning: multiple set tempo')
			else:
				tempo = msg ['tempo']
				tempoSet = True

		# program change
		if msg ['type'] == 'program_change':
			instruments [ msg ['channel'] ] = msg ['program']	# set new instrument # to the channel

		# control change
		if msg ['type'] == 'control_change':
			if msg ['control'] == 7:		# volume change
				volumes [ msg ['channel'] ] = msg ['value']		# set volume to the channel
			elif msg ['control'] == 0:		# bank MSB
				pass
			elif msg ['control'] == 32:		# bank LSB
				pass
			else:
				pass

		# update timing
		time = time + (int) (msg ['time'] * 1000000)	# time is in usec
#		print (time)

	# go through the song to make the best recommendations on channels, how they are used, etc
	# check which channels are used
	usedChannels, nbNotes = checkUsedChannels (song)
	print ('checking whether number of channels should be reduced... \n')

	# if more than 7 channels (+ 1 drum channel), merge channels that have the same instrument number
	if len (usedChannels) > 7:
		print ('More than 8 channels used. Trying to reduce number of channels by merging channel using the same instrument...')

		# more than 7 channels, let's see if we can decrease this number
		lett_list = []
		for i in usedChannels:
			lett_list.append (instruments [i])		# build a list of instruments used by usedChannels

		# I don't understand what is going on here, but it works
		lett_dict = {}
		counter = col.Counter(lett_list)
		elements = [i for i in counter] 
		for elem in elements:
			lett_dict[elem]=[i for i in range(len(lett_list)) if lett_list[i]==elem]
	
		# analyse results and try to merge tracks that use the same instrument
		for key, value in lett_dict.items():		# key is the instrument number, value is a list of indexes in usedChannels
			instr = key
			firstChan = usedChannels [value [0]]	# get 1st channel
			for i in value:							# go through the list of values
				instr = key
				chan = usedChannels [i]
				if (chan != firstChan):				# merge only if several channels for an instrument
					print ('merging channel', chan, 'with channel', firstChan, '- instrument is:', instr, gm_instruments [instr])
					# modify notes of the channel
					for nt in song:
						if nt.channel == chan:
							nt.channel = firstChan
			
		# here we should have merged all the channels that have the same instrument
		usedChannels, nbNotes = checkUsedChannels (song)
		print ('')

	# if more than 7 channels (+ 1 drum channel), remove channels that have the lowest number of notes
	if len (usedChannels) > 7:
		print ('More than 8 channels used. Trying to reduce number of channels by removing the channels that have the lowest number of notes...')

		while len (usedChannels) > 7:
			# find the channel that has the smallest smallest
			minChan = usedChannels [0]
			for i in usedChannels:
#				print ('channel', i, 'nbNotes', nbNotes [i], 'minChan', minChan)
				if nbNotes [i] < nbNotes [minChan]:
					minChan = i
			print ('channel:', minChan, 'will be removed - instrument:', instruments [minChan], gm_instruments [instruments [minChan]], '- volume:', volumes [minChan], '- nb notes:', nbNotes [minChan])

			# remove channel from list of used channels
			# get index of channel that has the minimum of notes
			i = usedChannels.index (minChan)
			del usedChannels [i]
			# remove unused channels from song; starting from the end of the list to make sure not to miss any element
			for i in range ((len (song) - 1), -1, -1):
				if song [i].channel == minChan:
					del song [i]

		# here we should have removed the channels with the fewest notes
		usedChannels, nbNotes = checkUsedChannels (song)
		print ('')

	#
	print ('building json output file... \n')

	# At this stage, usedChannels contains the channels that should be saved
	# There are 7 elements in usedChannels, at a maximum!!!
	usedChannels.insert (0, 10)		# add channel 10, the drum channel, at first place of list

	# build json
	# usedChannels are the ones that are the most used, and that should be exported
	data = {}
	
	# create instruments list
	instrList = []
	for i in usedChannels:
		instrList.append (instruments [i])
	while len (instrList) < 8:
		instrList.append (0)
	data ["instruments"] = instrList
	
	# create volumes list
	volList = []
	for i in usedChannels:
		volList.append (volumes [i])
	while len (volList) < 8:
		volList.append (0)
	data ["volumes"] = volList

	# fixed data
	data ["beats_per_bar"] = 4
	data ["beat_type"] = 4
	data ["ticks_per_beat"] = 480
	data ["ticks_beats_per_minute"] = (int) (60000000 / tempo)
	data ["time_bpm_multiplier"] = 1
	data ["quantizer"] = 4			# sixteenth

	# just to be sure: sort song by qbar, qbeat, qtick; note-on first, then note-off
	song = sortSong (song)

	# go in song note by note
	songLength = 0
	sg = []
	for nt in song:
		if nt.channel in usedChannels:			# we take only notes that are used (ie. belonging to used channels)
			nte = {}
			nte ["instrument"] =  usedChannels.index (nt.channel)
			nte ["status"] = nt.status
			nte ["key"] = nt.key
			nte ["velocity"] = nt.vel
			nte ["color"] = nt.color
			nte ["bar"] = nt.bar
			nte ["beat"] = nt.beat
			nte ["tick"] = nt.tick
			nte ["qbar"] = nt.qbar
			nte ["qbeat"] = nt.qbeat
			nte ["qtick"] = nt.qtick

			sg.append (nte)						# add to list of directories
			songLength = songLength + 1			# increase number of notes


	data ["song_length"] = songLength
	data ["notes"] = sg

#	print (json.dumps(data, indent=4, sort_keys=False))

	# write to file
	with open(fileName + '.json', 'w') as f:
		json.dump(data, f, indent = 4)
	f.close ()

	# print warning message about song length and file length
	print ('WARNING: song size is', songLength, 'notes (usual limit is 10000 notes)')
	print ('WARNING: json file length is', os.stat (fileName + '.json').st_size, 'characters (pls check usual limit: could be 4000000 chars)')


else:
	print ('\nusage : XXX.py name of midi file\n')

