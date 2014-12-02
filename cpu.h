#ifndef _CPU_H_
#define _CPU_H_

#include <stdlib.h>

#define FALSE 0
#define TRUE 1

#define byte unsigned char
#define maddress unsigned short

#define RESET_VECTOR 0xfffc

#define LOBYTE(x) (byte)(x)
#define HIBYTE(x) (byte)(x >> 8)

#define Z_SET(x) (x & 0b00000010)
#define N_SET(x) (x & 0b01000000)

struct cpu_status
{
	byte a;
	byte x;
	byte y;
	byte s;
	maddress pc;	
	byte flags;
	byte lastop;
};

#define WORD_LOHI(lo, hi) (maddress)((hi << 8) + lo)

extern void init_cpu(int RomSize, int RamSize);

extern void start_cpu(void (*before)(struct cpu_status *), void (*after)(struct cpu_status *), struct cpu_status*);

extern void get_cpu_status(struct cpu_status *cpustatus);

extern void lock_rom();

extern int rom_size();

extern int ram_size();

extern byte get_a();

extern byte get_x();

extern byte get_y();

extern maddress rom_start();

extern void set_memory(maddress location, byte value);

extern byte get_memory(maddress location);

#endif