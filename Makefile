CC=gcc
RM=rm
CFLAGS=-std=c99 -Wall -I .

all: h6502

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

h6502: h6502_main.o cpu.o
	$(CC) -o h6502 $^ $(CFLAGS)

clean:
	$(RM) -f *.o
	$(RM) -f h6502.exe
	$(RM) -f h6502

