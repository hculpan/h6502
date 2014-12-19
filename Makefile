CC=gcc
RM=rm
ASM_HOME?=C:/Users/usucuha/64tass-1.51.774/64tass.exe
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
	$(RM) -f *.bin
	$(RM) -f h6502.exe
	$(RM) -f h6502

rom:
	$(ASM_HOME) -b -o rom.bin test/rom.asm

tinybasic:
	$(ASM_HOME) -b -o rom.bin test/tinybasic.asm
