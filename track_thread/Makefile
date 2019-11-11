
CC      := gcc
LIBS    := -lpthread -ltinyalsa -lm
LDFLAGS := src
DEFINES :=
INCLUDE := -I. -Iinclude
CFLAGS  := 
CXXFLAGS:= 

foo : foo.o convolve.o process_audio.o audio_effect.o
	$(CC) -o foo foo.o process_audio.o audio_effect.o $(LIBS)

foo.o : foo.c
	$(CC) -c foo.c

convolve.o : convolve.c
	$(CC) -c convolve.c

process_audio.o : process_audio.c
	$(CC) -c process_audio.c

audio_effect.o : audio_effect.c
	$(CC) -c audio_effect.c

clean:
	rm foo foo.o
