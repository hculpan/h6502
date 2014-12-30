CC=gcc
RM=rm
ASM_HOME?=/Users/harryculpan/Dropbox/RetroComputing/dev/64tass-os-x/64tass
CFLAGS=-std=c99 -Wall -Ofast -I .

all: h6502

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

h6502: h6502_main.o cpu.o
	$(CC) -o h6502 $^ $(CFLAGS)

test: test.o
	$(CC) -o test $^ $(CFLAGS)

clean:
	$(RM) -f *.o
	$(RM) -f *.stackdump
	$(RM) -f *.bin
	$(RM) -f *.zip
	$(RM) -f h6502.exe
	$(RM) -f h6502

rom:
	$(ASM_HOME) -b -o rom.bin rom/rom.asm

zip: clean
	zip -r9 h6502.zip *

tinybasic:
	$(ASM_HOME) -b -o rom.bin rom/tinybasic.asm
