#ifndef _CPU_H_
#define _CPU_H_

#include <stdlib.h>
#include <stdint.h>

#define FALSE 0
#define TRUE 1

#define byte uint8_t
#define maddress uint16_t

#define RESET_VECTOR 0xfffc

#define LOBYTE(x) (byte)(x)
#define HIBYTE(x) (byte)(x >> 8)

#define Z_SET(x) (x & 0b00000010)
#define N_SET(x) (x & 0b10000000)
#define C_SET(x) (x & 0b00000001)
#define V_SET(x) (x & 0b01000000)
#define D_SET(x) (x & 0b00001000)
#define I_SET(x) (x & 0b00000100)

#define SEGMENT_TYPE_RAM 0
#define SEGMENT_TYPE_ROM 1
#define SEGMENT_TYPE_DMA 2

#define WORD_LOHI(lo, hi) (maddress)((hi << 8) + lo)
#define WORD_HILO(hi, lo) (maddress)((hi << 8) + lo)

typedef struct mem_segment mem_segment;

struct mem_segment {
	byte segment_type;
	maddress start;
	maddress end;
	struct mem_segment *next_segment;
};

struct cpu_status {
	byte a;
	byte x;
	byte y;
	byte s;
	maddress pc;	
	byte flags;
	byte lastop;
	byte lastcycles;
	uint64_t totalcycles;
	char message[100];
};

extern mem_segment* create_mem_segment(byte segment_type, maddress start, maddress end, mem_segment *prev);

extern void init_cpu(mem_segment *first_segment);

extern void start_cpu(void (*after)(struct cpu_status *), void (*fault)(struct cpu_status *), struct cpu_status*);

extern void step(void (*after)(struct cpu_status *), void (*fault)(struct cpu_status *), struct cpu_status*);

extern void get_cpu_status(struct cpu_status *cpustatus);

extern void set_pc_register(maddress address);

extern void lock_rom();

extern uint16_t rom_size();

extern uint16_t ram_size();

extern byte get_a();

extern byte get_x();

extern byte get_y();

extern maddress rom_start();

extern void set_memory(maddress location, byte value);

extern byte get_memory(maddress location);

extern mem_segment *get_first_memory_segment();

#endif