#This sample makefile has been setup for a project which contains the following files: main.h, ap-main.c, ap-main.h, ap-gen.c, ap-gen.h   Edit as necessary for your project

#Change output_file_name.a below to your desired executible filename

#Set all your object files (the object files of all the .c files in your project, e.g. main.o my_sub_functions.o )
OBJ = main.o process.o utils.o led.o song.o disk.o useless.o

#Set any dependant header files so that if they are edited they cause a complete re-compile (e.g. main.h some_subfunctions.h some_definitions_file.h ), or leave blank
DEPS = jack/jack.h jack/midiport.h types.h main.h process.h utils.h led.h song.h disk.h midiwriter.h useless.h

#Any special libraries you are using in your project (e.g. -lbcm2835 -lrt `pkg-config --libs gtk+-3.0` ), or leave blank
#LIBS = -L/usr/lib/i386-linux-gnu -ljack
LIBS = -ljack -lm -lncurses -lcjson -L/usr/local/lib64


#Set any compiler flags you want to use (e.g. -I/usr/include/somefolder `pkg-config --cflags gtk+-3.0` ), or leave blank
#REMOVE -g TO REMOVE DEBUGGER
CFLAGS =

#Set the compiler you are using ( gcc for C or g++ for C++ )
CC = gcc

#Set the filename extension of your C files (e.g. .c or .cpp )
EXTENSION = .c

#define a rule that applies to all files ending in the .o suffix, which says that the .o file depends upon the .c version of the file and all the .h files included in the DEPS macro.  Compile each object file
%.o: %$(EXTENSION) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#Combine them into the output file
#Set your desired exe output file name here
#REMOVE -g TO REMOVE DEBUGGER
compo.a: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
#	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -Xlinker --verbose
#Clean and move executable one level up
	rm -f *.o *~ core *~
	mv $@ ../$@

#Cleanup
.PHONY: clean

clean:
	rm -f *.o *~ core *~
