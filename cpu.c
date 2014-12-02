#include "cpu.h"

int romSize;
int ramSize;

maddress romStart;

byte *rom;
byte *ram;

byte rom_locked = FALSE;

byte accum_reg = 0;
byte x_reg = 0;
byte y_reg = 0;
byte flags_reg = 0;

maddress stack_start = 0x0100;
byte sp_reg = 0x00;

maddress pc_reg = 0x00;

byte cpu_running = FALSE;

byte lastop = 0;

byte * init_memory(int size) {
	byte *result = malloc(size);
	for (int i = 0; i < size; i++) {
		result[i] = 0;
	}
	return result;
}

void load_rom() {
	set_memory(0xfffc, LOBYTE(romStart));
	set_memory(0xfffd, HIBYTE(romStart));

	set_memory(romStart, 0xa9);
	set_memory(romStart + 1, 0x00);
	set_memory(romStart + 2, 0xa9);
	set_memory(romStart + 3, 0xef);

	lock_rom();
}

void init_cpu(int RomSize, int RamSize) {
	romSize = RomSize;
	rom = init_memory(romSize);
	ramSize = RamSize;
	ram = init_memory(ramSize);
	romStart = 65536 - romSize;
	load_rom();
}

void get_cpu_status(struct cpu_status *cpustatus) {
	cpustatus->a = accum_reg;
	cpustatus->x = x_reg;
	cpustatus->y = y_reg;
	cpustatus->s = sp_reg;
	cpustatus->pc = pc_reg;
	cpustatus->flags = flags_reg;
	cpustatus->lastop = lastop;
}

void set_negative_flag() {
	flags_reg |= 0b01000000;
}

byte get_negative_flag() {
	return N_SET(flags_reg);
}

void clear_negative_flag() {
	flags_reg &= 0b10111111;
}

void set_zero_flag() {
	flags_reg |= 0b00000010;
}

byte get_zero_flag() {
	return Z_SET(flags_reg);
}

void clear_zero_flag() {
	flags_reg &= 0b11111101;
}

int excute_instruction() {
	byte cycles = 0;
	lastop = get_memory(pc_reg);

	switch (lastop) {
		case 0xa9: // LDA immediate
			accum_reg = get_memory(pc_reg + 1);
			cycles = 2;
			if (accum_reg == 0) {
				set_zero_flag();
				clear_negative_flag();
			} else if (accum_reg > 127) {
				set_negative_flag();
				clear_zero_flag();
			} else {
				clear_zero_flag();
				clear_negative_flag();
			}

			pc_reg += 2;
			break;
		default:
			pc_reg += 1;
			cpu_running = FALSE;
			break;
	}

	return cycles;
}

void start_cpu(void (*before)(struct cpu_status *), void (*after)(struct cpu_status *), struct cpu_status *cpustatus) {
	cpu_running = TRUE;

	pc_reg = WORD_LOHI(get_memory(RESET_VECTOR), get_memory(RESET_VECTOR + 1));

	while (cpu_running) {
		if (before) {
			get_cpu_status(cpustatus);
			before(cpustatus);
		}

		excute_instruction();

		if (after) {
			get_cpu_status(cpustatus);
			after(cpustatus);
		}
	}
}

void lock_rom() {
	rom_locked = TRUE;
}

byte get_a() {
	return accum_reg;
}

byte get_x() {
	return x_reg;
}

byte get_y() {
	return y_reg;
}

int rom_size() {
	return romSize;
}

int ram_size() {
	return ramSize;
}

maddress rom_start() {
	return romStart;
}

void set_memory(maddress location, byte value) {
	if (location < ramSize) {
		ram[location] = value;
	} else if (location >= romStart && !rom_locked) {
		rom[location] = value;
	}
}

byte get_memory(maddress location) {
	byte result = 0;

	if (location < ramSize) {
		result = ram[location];
	} else if (location >= romStart) {
		result = rom[location];
	}

	return result;
}