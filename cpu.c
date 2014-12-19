#include "cpu.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int romSize;
int ramSize;

maddress romStart;

byte *rom;
byte *ram;

byte rom_locked = FALSE;

byte accum_reg = 0;
byte x_reg = 0;
byte y_reg = 0;
byte flags_reg = 0b00100000;

maddress stack_start = 0x0100;
byte sp_reg = 0x00;

maddress pc_reg = 0x00;

byte cpu_running = FALSE;

byte lastop = 0;

byte lastcycles = 0;

unsigned long totalcycles = 0;

char cpu_fault_message[100];

byte * init_memory(int size) {
	byte *result = malloc(size);
	for (int i = 0; i < size; i++) {
		result[i] = 0;
	}
	return result;
}

byte load_bin() {
	printf("Attempting to find rom.bin\n");
	byte result = FALSE;
	FILE *file = fopen("rom.bin", "rb");
	if (!file) {
		file = fopen("test/rom.bin", "rb");
	}

	if (file) {
		printf("Reading from rom.bin\n");
		fseek(file, 0, SEEK_END);  
		long sz = ftell(file);  
		rewind(file);           
		fread(rom, sz, 1, file);
		fclose(file);
		result = TRUE;
	}

	return result;
}

void load_rom() {
	set_memory(0xfffc, LOBYTE(romStart));
	set_memory(0xfffd, HIBYTE(romStart));

	if (!load_bin()) {
		printf("No rom.bin found; loading default\n");
		set_memory(romStart    , 0xa9);  // LDA #ef
		set_memory(romStart + 1, 0xef);
		set_memory(romStart + 2, 0x85);  // STA $10
		set_memory(romStart + 3, 0x10);
		set_memory(romStart + 4, 0xa6);  // LDX $10
		set_memory(romStart + 5, 0x10);
	}

	lock_rom();
}

void init_cpu(int RomSize, int RamSize) {
	sp_reg = 0xff;
	romSize = RomSize;
	rom = init_memory(romSize);
	ramSize = RamSize;
	ram = init_memory(ramSize);
	romStart = 65536 - romSize;
	totalcycles = 0;
	load_rom();
	pc_reg = WORD_LOHI(get_memory(RESET_VECTOR), get_memory(RESET_VECTOR + 1));
}

void get_cpu_status(struct cpu_status *cpustatus) {
	cpustatus->a = accum_reg;
	cpustatus->x = x_reg;
	cpustatus->y = y_reg;
	cpustatus->s = sp_reg;
	cpustatus->pc = pc_reg;
	cpustatus->flags = flags_reg;
	cpustatus->lastop = lastop;
	cpustatus->lastcycles = lastcycles;
	cpustatus->totalcycles = totalcycles;
	strcpy(cpustatus->message, cpu_fault_message);
}

void set_overflow_flag() {
	flags_reg |= 0b01000000;
}

byte get_overflow_flag() {
	return V_SET(flags_reg);
}

void clear_overflow_flag() {
	flags_reg &= 0b10111111;
}

void update_overflow_flag(byte value) {
	flags_reg |= (0b01000000 & value);
}

void set_carry_flag() {
	flags_reg |= 0b00000001;
}

byte get_carry_flag() {
	return C_SET(flags_reg);
}

void clear_carry_flag() {
	flags_reg &= 0b11111110;
}

void set_negative_flag() {
	flags_reg |= 0b10000000;
}

byte get_negative_flag() {
	return N_SET(flags_reg);
}

void clear_negative_flag() {
	flags_reg &= 0b01111111;
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

void update_zero_flag(byte value) {
	if (value == 0) {
		set_zero_flag();
	} else {
		clear_zero_flag();
	}
}

void update_negative_flag(byte value) {
	if (value > 127) {
		set_negative_flag();
	} else {
		clear_negative_flag();
	}
}

void update_zero_and_negative_flags(byte value) {
	update_zero_flag(value);
	update_negative_flag(value);
}

byte get_immediate_value() {
	return get_memory(pc_reg++);
}

maddress get_absolute_arg() {
	maddress result = WORD_LOHI(get_memory(pc_reg), get_memory(pc_reg + 1));
	pc_reg += 2;
	return result;
}

byte get_absolute_value() {
	return get_memory(get_absolute_arg());
}

maddress get_absolute_x_arg() {
	return get_absolute_arg() + x_reg;
}

byte get_absolute_x_value() {
	return get_memory(get_absolute_x_arg());
}

maddress get_absolute_y_arg() {
	return get_absolute_arg() + y_reg;
}

byte get_absolute_y_value() {
	return get_memory(get_absolute_y_arg());
}

maddress get_zero_page_arg() {
	return WORD_LOHI(get_memory(pc_reg++), 0x00);
}

byte get_zero_page_value() {
	return get_memory(get_zero_page_arg());
}

maddress get_zero_page_x_arg() {
	return get_zero_page_arg() + x_reg;
}

byte get_zero_page_x_value() {
	return get_memory(get_zero_page_x_arg());
}

maddress get_zero_page_y_arg() {
	return get_zero_page_arg() + y_reg;
}

byte get_zero_page_y_value() {
	return get_memory(get_zero_page_y_arg());
}

signed char get_relative() {
	return get_memory(pc_reg++);
}

maddress get_indirect_arg() {
	maddress result = WORD_LOHI(get_memory(pc_reg++), 0x00);
	result = WORD_LOHI(get_memory(result), get_memory(result + 1));
	return result;
}

byte get_indirect_value() {
	return get_memory(get_indirect_arg());
}

// (Ind,x)
maddress get_indexed_indirect_arg() {
	maddress result = WORD_LOHI(get_memory(pc_reg) + x_reg, 0x00);
	pc_reg += 1;
	result = WORD_LOHI(get_memory(result), get_memory(result + 1));
	return result;
}

// (Ind,x)
byte get_indexed_indirect_value() {
	return get_memory(get_indexed_indirect_arg());
}

// (Ind),y
maddress get_indirect_indexed_arg() {
	return get_indirect_arg() + y_reg;
}

// (Ind),y
byte get_indirect_indexed_value() {
	return get_memory(get_indirect_indexed_arg());
}

void push_byte(byte value) {
	set_memory(sp_reg + 256, value);
	sp_reg--;
}

byte pop_byte() {
	sp_reg++;
	byte result = get_memory(sp_reg + 256);
	return result;
}

void push_address(maddress value) {
	push_byte(HIBYTE(value));
	push_byte(LOBYTE(value));
}

maddress pop_address() {
	byte lo = pop_byte();
	byte hi = pop_byte();
	return WORD_LOHI(lo, hi);
}

void compare_values(byte reg, byte value) {
	update_negative_flag(reg - value);
	update_zero_flag(reg - value);
	if (reg >= value) {
		set_carry_flag();
	} else {
		clear_carry_flag();
	}
}

void bit_values(byte reg, byte value) {
	update_negative_flag(reg & value);
	update_zero_flag(reg & value);
	update_overflow_flag(reg & value);
}

void and_values(byte value) {
	accum_reg = accum_reg & value;
	update_zero_and_negative_flags(accum_reg);
}

void eor_values(byte value) {
	accum_reg = accum_reg ^ value;
	update_zero_and_negative_flags(accum_reg);
}

int excute_instruction() {
	cpu_fault_message[0] = '\0';
	byte cycles = 0;
	lastop = get_memory(pc_reg++);
	maddress memarg;
	signed char relative_diff;

	switch (lastop) {
		case 0xa9: // LDA immediate
			accum_reg = get_immediate_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 2;
			break;
		case 0xa5: // LDA zero page
			accum_reg = get_zero_page_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 3;
			break;
		case 0xbd: // LDA absolute,x
			accum_reg = get_absolute_x_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 4 + (pc_reg % 256 > 0 ? 1 : 0);
			break;
		case 0xb9: // LDA absolute,y
			accum_reg = get_absolute_y_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 4 + (pc_reg % 256 > 0 ? 1 : 0);
			break;
		case 0xad: // LDA absolute
			accum_reg = get_absolute_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 4;
			break;
		case 0xb1: // LDA (ind),y
			accum_reg = get_indirect_indexed_value();
			update_zero_and_negative_flags(accum_reg);
			break;
		case 0xa6: // LDX zero page
			x_reg = get_zero_page_value();
			update_zero_and_negative_flags(x_reg);
			cycles = 3;
			break;
		case 0xa2: // LDX immediate
			x_reg = get_immediate_value();
			update_zero_and_negative_flags(x_reg);
			cycles = 2;
			break;
		case 0xae: // LDX absolute
			x_reg = get_absolute_value();
			update_zero_and_negative_flags(x_reg);
			cycles = 4;
			break;
		case 0xa4: // LDY zero page
			y_reg = get_zero_page_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 3;
			break;
		case 0xa0: // LDY immediate
			y_reg = get_immediate_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 2;
			break;
		case 0xac: // LDY absolute
			y_reg = get_absolute_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 4;
			break;
		case 0x8d: // STA absolute
			set_memory(get_absolute_arg(), accum_reg);
			cycles = 4;
			break;
		case 0x9d: // STA absolute, x
			set_memory(get_absolute_x_arg(), accum_reg);
			cycles = 5;
			break;
		case 0x99: // STA absolute, y
			set_memory(get_absolute_y_arg(), accum_reg);
			cycles = 5;
			break;
		case 0x85: // STA zero page
			set_memory(get_zero_page_arg(), accum_reg);
			cycles = 3;
			break;
		case 0x95: // STA zero page,x
			set_memory(get_zero_page_x_arg(), accum_reg);
			cycles = 4;
			break;
		case 0x81: // STA (indirect, x)
			set_memory(get_indexed_indirect_arg(), accum_reg);
			cycles = 6;
			break;
		case 0x91: // STA (indirect), y
			set_memory(get_indirect_indexed_arg(), accum_reg);
			cycles = 6;
			break;
		case 0x4c: // JMP absolute
			pc_reg = get_absolute_arg();
			cycles = 3;
			break;
		case 0x60: // RTS implied
			pc_reg = pop_address() + 1;
			cycles = 6;
			break;
		case 0x20: // JSR absolute
			push_address(pc_reg + 1);
			pc_reg = get_absolute_arg();
			cycles = 6;
			break;
		case 0xe6: // INC zero page
			memarg = get_zero_page_arg();
			set_memory(memarg, get_memory(memarg) + 1);
			cycles = 5;
			break;
		case 0xe8: // INX impl
			x_reg++;
			update_zero_and_negative_flags(x_reg);
			cycles = 2;
			break;
		case 0xc8: // INY impl
			y_reg++;
			update_zero_and_negative_flags(y_reg);
			cycles = 2;
			break;
		case 0xca: // DEX impl
			x_reg--;
			update_zero_and_negative_flags(x_reg);
			cycles = 2;
			break;
		case 0x88: // DEY impl
			y_reg--;
			update_zero_and_negative_flags(y_reg);
			cycles = 2;
			break;
		case 0xf0: //BEQ relative
			relative_diff = get_relative();
			if (get_zero_flag()) {
				byte currpage = pc_reg >> 8;
				pc_reg += relative_diff;
				cycles = 3 + ( currpage != pc_reg >> 8 ? 1 : 0);
			} else {
				cycles = 2;
			}
			break;
		case 0xd0: //BNE relative
			relative_diff = get_relative();
			if (!get_zero_flag()) {
				byte currpage = pc_reg >> 8;
				pc_reg += relative_diff;
				cycles = 3 + ( currpage != pc_reg >> 8 ? 1 : 0);
			} else {
				cycles = 2;
			}
			break;
		case 0x10: //BPL relative
			relative_diff = get_relative();
			if (!get_negative_flag()) {
				byte currpage = pc_reg >> 8;
				pc_reg += relative_diff;
				cycles = 3 + ( currpage != pc_reg >> 8 ? 1 : 0);
			} else {
				cycles = 2;
			}
			break;
		case 0x30: //BMI relative
			relative_diff = get_relative();
			if (get_negative_flag()) {
				byte currpage = pc_reg >> 8;
				pc_reg += relative_diff;
				cycles = 3 + ( currpage != pc_reg >> 8 ? 1 : 0);
			} else {
				cycles = 2;
			}
			break;
		case 0xc9: //CMP immediate
			compare_values(accum_reg, get_immediate_value());
			cycles = 2;
			break;
		case 0xc5: //CMP zero page
			compare_values(accum_reg, get_zero_page_value());
			cycles = 3;
			break;
		case 0xd1: //CMP (indirect),y
			compare_values(accum_reg, get_indirect_indexed_value());
			cycles = 3;
			break;
		case 0x24: //BIT zero page
			bit_values(accum_reg, get_zero_page_value());
			cycles = 3;
			break;
		case 0x2c: //BIT absolute
			bit_values(accum_reg, get_absolute_value());
			cycles = 4;
			break;
		case 0x29: //AND immediate
			and_values(get_immediate_value());
			cycles = 2;
			break;
		case 0x00: //BRK implied
			cycles = 7;
			cpu_running = FALSE;
			break;
		case 0xaa: //TAX implied
			x_reg = accum_reg;
			cycles = 2;
			break;
		case 0x49: //EOR immediate
			eor_values(get_immediate_value());
			cycles = 2;
			break;
		case 0x08: //PHP implied
			push_byte(flags_reg);
			cycles = 3;
			break;
		case 0x28: // PLP implied
			flags_reg = pop_byte();
			cycles = 4;
			break;
		case 0x8a: // TXA implied
			accum_reg = x_reg;
			cycles = 2;
			break;
		default:
			sprintf(cpu_fault_message, "Unimplemented op-code: %02x at %04x", lastop, --pc_reg);
			cpu_running = FALSE;
			break;
	}

	return cycles;
}

void step(void (*after)(struct cpu_status *), void (*fault)(struct cpu_status *), struct cpu_status *cpustatus) {
	lastcycles = excute_instruction();
	totalcycles += lastcycles;

	if (fault && cpu_fault_message[0] != '\0') {
		get_cpu_status(cpustatus);
		fault(cpustatus);
		cpu_running = FALSE;
	} else if (after) {
		get_cpu_status(cpustatus);
		after(cpustatus);
	}
}

void start_cpu(void (*after)(struct cpu_status *), void (*fault)(struct cpu_status *), struct cpu_status *cpustatus) {
	totalcycles = 0;
	cpu_running = TRUE;

	while (cpu_running) {
		step(after, fault, cpustatus);
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
	if (location == 0xd012) { // send char to output
		if (value == 0x0d) {
			printf("\n");
		} else {
			printf("%c", value);
		}
		ram[location] = value & 0b10111111;
	} else if (location < ramSize) {
		ram[location] = value;
	} else if (location >= romStart && !rom_locked) {
		rom[location - romStart] = value;
	}
}

int kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

byte get_memory(maddress location) {
	byte result = 0;

	if (location == 0xd011) { // check keyboard status
		if (kbhit() == 0) {
			return 0;
		} else {
			byte c = LOBYTE(getchar());
			set_memory(0xd010, (c == 0x0a ? 0x0d : c));
			return 0xff;
		}
		return 0;
	} else if (location < ramSize) {
		result = ram[location];
	} else if (location >= romStart) {
		result = rom[location - romStart];
	}

	return result;
}

void set_pc_register(maddress address) {
	pc_reg = address;
}

