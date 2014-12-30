#include "cpu.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

byte ram[65536];

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

uint64_t totalcycles = 0;

char cpu_fault_message[100];

mem_segment *first_mem_segment;

mem_segment *last_segment = NULL; // store last mem_segment accessed

byte load_raw(char *name, maddress start) {
	byte result = FALSE;
	FILE *file = fopen(name, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);  
		int32_t sz = ftell(file);  
		if (sz > 65536 - start) {
			sprintf(cpu_fault_message, 
				"File size %d exceeds available space of %d", 
				sz, 
				65536 - start);
		} else {
			byte *filebuffer = malloc(sz);
			rewind(file);           
			fread(filebuffer, sz, 1, file);
			fclose(file);

			for (int i = 0; i < sz; i++) {
				set_memory(start + i, filebuffer[i]);
			}
			free(filebuffer);

			result = TRUE;
		}
	}

	return result;
}

void load_rom() {
	set_memory(0xfffc, 0x00);
	set_memory(0xfffd, 0xe0);

	if (!load_raw("rom.bin", 0xe000)) {
		printf("No rom.bin found; loading default\n");
		maddress romStart = 0xe000;
		set_memory(romStart    , 0xa9);  // LDA #ef
		set_memory(romStart + 1, 0xef);
		set_memory(romStart + 2, 0x85);  // STA $10
		set_memory(romStart + 3, 0x10);
		set_memory(romStart + 4, 0xa6);  // LDX $10
		set_memory(romStart + 5, 0x10);
	} else {
		printf("rom.bin loaded\n");
	}

	lock_rom();
}

void init_cpu(mem_segment *first_segment) {
	sp_reg = 0xff;
	first_mem_segment = first_segment;
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

void set_decimal_flag() {
	flags_reg |= 0b00001000;
}

byte get_decimal_flag() {
	return D_SET(flags_reg);
}

void clear_decimal_flag() {
	flags_reg &= 0b11110111;
}

void set_interrupt_flag() {
	flags_reg |= 0b00000100;
}

byte get_interrupt_flag() {
	return I_SET(flags_reg);
}

void clear_interrupt_flag() {
	flags_reg &= 0b11111011;
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

void set_overflow(byte value) {
	if (value) {
		set_overflow_flag();
	} else {
		clear_overflow_flag();
	}
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

void set_carry(byte value) {
	if (value) {
		set_carry_flag();
	} else {
		clear_carry_flag();
	}
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

void ora_values(byte value) {
	accum_reg = accum_reg | value;
	update_zero_and_negative_flags(accum_reg);
}

void eor_values(byte value) {
	accum_reg = accum_reg ^ value;
	update_zero_and_negative_flags(accum_reg);
}

void sbc_values(byte value) {
	unsigned short result = (unsigned short)accum_reg - (unsigned short)value - (get_carry_flag() ? 0 : 1);
	set_carry(result < 0x100);
	set_overflow(((accum_reg ^ result) & 0x80) && ((accum_reg ^ value) & 0x80));
	accum_reg = (result & 0xff);
	update_zero_and_negative_flags(accum_reg);
}

void adc_values(byte value) {
	unsigned short result = accum_reg + value + (get_carry_flag() != 0 ? 1 : 0);
	if ((accum_reg & 0b10000000) != (result & 0b10000000)) {
		set_overflow_flag();
	} else {
		clear_overflow_flag();
	}
	update_negative_flag(accum_reg);
	update_zero_flag(result);
	if (get_decimal_flag()) {
		if (result > 99) {
			set_carry_flag();
		} else {
			clear_carry_flag();
		}
	} else {
		if (result > 255) {
			set_carry_flag();
		} else {
			clear_carry_flag();
		}
	}
	accum_reg = (byte)result & 0xff;
}

void asl_memory(maddress address) {
	byte oldvalue = get_memory(address);
	if (oldvalue & 0b10000000) {
		set_carry_flag();
	} else {
		clear_carry_flag();
	}
	oldvalue <<= 1;
	update_zero_and_negative_flags(oldvalue);
	set_memory(address, oldvalue);
}

void lsr_memory(maddress address) {
	byte oldvalue = get_memory(address);
	if (oldvalue & 0b00000001) {
		set_carry_flag();
	} else {
		clear_carry_flag();
	}
	oldvalue >>= 1;
	update_zero_and_negative_flags(oldvalue);
	set_memory(address, oldvalue);
}

void rol_memory(maddress address) {
	byte value = get_memory(address);
	byte t = value & 0b10000000;
	value = (value << 1) & 0xfe;
	value |= (get_carry_flag() ? 1 : 0);
	if (t) {
		set_carry_flag();
	} else {
		clear_carry_flag();
	}
	update_zero_and_negative_flags(value);
	set_memory(address, value);
}

int excute_instruction() {
	cpu_fault_message[0] = '\0';
	byte cycles = 0;
	lastop = get_memory(pc_reg++);
	maddress memarg;
	signed char relative_diff;

	switch (lastop) {
		case 0x69: // ADC immediate
			adc_values(get_immediate_value());
			cycles = 2;
			break;
		case 0x65: // ADC zero page
			adc_values(get_zero_page_value());
			cycles = 3;
			break;
		case 0x75: // ADC zero page,x
			adc_values(get_zero_page_x_value());
			cycles = 4;
			break;
		case 0x6d: // ADC absolute
			adc_values(get_absolute_value());
			cycles = 4;
			break;
		case 0x7d: // ADC absolute,x
			adc_values(get_absolute_x_value());
			cycles = 4;
			break;
		case 0x79: // ADC absolute,y
			adc_values(get_absolute_y_value());
			cycles = 4;
			break;
		case 0x61: // ADC (ind,x)
			adc_values(get_indexed_indirect_value());
			cycles = 6;
			break;
		case 0x71: // ADC (ind),y
			adc_values(get_indirect_indexed_value());
			cycles = 5;
			break;
		case 0xe9: // SBC immediate
			sbc_values(get_immediate_value());
			cycles = 2;
			break;		
		case 0xe5: // SBC zero page
			sbc_values(get_zero_page_value());
			cycles = 3;
			break;		
		case 0xf5: // SBC zero page,x
			sbc_values(get_zero_page_x_value());
			cycles = 4;
			break;		
		case 0xed: // SBC absolute
			sbc_values(get_absolute_value());
			cycles = 4;
			break;		
		case 0xfd: // SBC absolute,x
			sbc_values(get_absolute_x_value());
			cycles = 4;
			break;		
		case 0xf9: // SBC absolute,y
			sbc_values(get_absolute_y_value());
			cycles = 4;
			break;		
		case 0xe1: // SBC (ind,x)
			sbc_values(get_indexed_indirect_value());
			cycles = 6;
			break;		
		case 0xf1: // SBC (ind),y
			sbc_values(get_indirect_indexed_value());
			cycles = 5;
			break;		
		case 0x0a: // ASL implied
			if (accum_reg & 0b10000000) {
				set_carry_flag();
			} else {
				clear_carry_flag();
			}
			accum_reg <<= 1;
			update_zero_and_negative_flags(accum_reg);
			cycles = 2;
			break;
		case 0x06: // ASL zero page
			asl_memory(get_zero_page_arg());
			cycles = 5;
			break;
		case 0x16: // ASL zero page,x
			asl_memory(get_zero_page_x_arg());
			cycles = 6;
			break;
		case 0x0e: // ASL absolute
			asl_memory(get_absolute_arg());
			cycles = 6;
			break;
		case 0x1e: // ASL absolute,x
			asl_memory(get_absolute_x_arg());
			cycles = 7;
			break;
		case 0xe0: // CPX immediate
			compare_values(x_reg, get_immediate_value());
			cycles = 2;
			break;
		case 0xe4: // CPX zero page
			compare_values(x_reg, get_zero_page_value());
			cycles = 2;
			break;
		case 0xec: // CPX absolute
			compare_values(x_reg, get_absolute_value());
			cycles = 2;
			break;
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
		case 0xb5: // LDA zero page,x
			accum_reg = get_zero_page_x_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 4;
			break;
		case 0xad: // LDA absolute
			accum_reg = get_absolute_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 4;
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
		case 0xa1: // LDA (ind,x)
			accum_reg = get_indexed_indirect_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 6;
			break;
		case 0xb1: // LDA (ind),y
			accum_reg = get_indirect_indexed_value();
			update_zero_and_negative_flags(accum_reg);
			cycles = 5;
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
		case 0xa0: // LDY immediate
			y_reg = get_immediate_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 2;
			break;
		case 0xa4: // LDY zero page
			y_reg = get_zero_page_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 3;
			break;
		case 0xb4: // LDY zero page,X
			y_reg = get_zero_page_x_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 4;
			break;
		case 0xac: // LDY absolute
			y_reg = get_absolute_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 4;
			break;
		case 0xbc: // LDY absolute.x
			y_reg = get_absolute_x_value();
			update_zero_and_negative_flags(y_reg);
			cycles = 4;
			break;
		case 0x4a: // LSR implied
			if (accum_reg & 0b00000001) {
				set_carry_flag();
			} else {
				clear_carry_flag();
			}
			accum_reg >>= 1;
			update_zero_and_negative_flags(accum_reg);
			cycles = 2;
			break;
		case 0x46: // LSR zero page
			lsr_memory(get_zero_page_arg());
			cycles = 5;
			break;
		case 0x56: // LSR zero page,x
			lsr_memory(get_zero_page_x_arg());
			cycles = 6;
			break;
		case 0x4e: // LSR absolute
			lsr_memory(get_absolute_arg());
			cycles = 6;
			break;
		case 0x5e: // LSR absolute,x
			lsr_memory(get_absolute_x_arg());
			cycles = 7;
			break;
		case 0x2a: { // ROL implied
			byte t = accum_reg & 0b10000000;
			accum_reg = (accum_reg << 1) & 0xfe;
			accum_reg |= (get_carry_flag() ? 1 : 0);
			if (t) {
				set_carry_flag();
			} else {
				clear_carry_flag();
			}
			update_zero_and_negative_flags(accum_reg);
			cycles = 2;
			break; }
		case 0x26: // ROL zero page
			rol_memory(get_zero_page_arg());
			cycles = 5;
			break;
		case 0x36: // ROL zero page,x
			rol_memory(get_zero_page_x_arg());
			cycles = 6;
			break;
		case 0x2e: // ROL absolute
			rol_memory(get_absolute_arg());
			cycles = 6;
			break;
		case 0x3e: // ROL absolute,x
			rol_memory(get_absolute_x_value());
			cycles = 7;
			break;
		case 0x09: // ORA immediate
			ora_values(get_immediate_value());
			cycles = 2;
			break;
		case 0x05: // ORA zero page
			ora_values(get_zero_page_value());
			cycles = 2;
			break;
		case 0x15: // ORA zero page,x
			ora_values(get_zero_page_x_value());
			cycles = 3;
			break;
		case 0x0d: // ORA absolute
			ora_values(get_absolute_value());
			cycles = 4;
			break;
		case 0x1d: // ORA absolute,x
			ora_values(get_absolute_x_value());
			cycles = 4;
			break;
		case 0x19: // ORA absolute,y
			ora_values(get_absolute_y_value());
			cycles = 4;
			break;
		case 0x01: // ORA indexed indirect
			ora_values(get_indexed_indirect_value());
			cycles = 6;
			break;
		case 0x11: // ORA indirect indexed
			ora_values(get_indirect_indexed_value());
			cycles = 5;
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
		case 0x86: // STX zero page
			set_memory(get_zero_page_arg(), x_reg);
			cycles = 3;
			break;
		case 0x96: // STX zero page,y
			set_memory(get_zero_page_y_arg(), x_reg);
			cycles = 4;
			break;
		case 0x8e: // STX absolute
			set_memory(get_absolute_arg(), x_reg);
			cycles = 4;
			break;
		case 0x84: // STY zero page
			set_memory(get_zero_page_arg(), y_reg);
			cycles = 3;
			break;
		case 0x94: // STY zero page,x
			set_memory(get_zero_page_x_arg(), y_reg);
			cycles = 4;
			break;
		case 0x8c: // STY absolute
			set_memory(get_absolute_arg(), y_reg);
			cycles = 4;
			break;
		case 0x4c: // JMP absolute
			pc_reg = get_absolute_arg();
			cycles = 3;
			break;
		case 0x60: // RTS implied
			pc_reg = pop_address() + 1;
			cycles = 6;
			break;
		case 0x40: // RTI implied
			flags_reg = pop_byte();
			pc_reg = pop_address();
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
		case 0xc6: // DEC zero page
			memarg = get_zero_page_arg();
			set_memory(memarg, get_memory(memarg) - 1);
			cycles = 5;
			break;
		case 0xd6: // DEC zero page,x
			memarg = get_zero_page_x_arg();
			set_memory(memarg, get_memory(memarg) - 1);
			cycles = 5;
			break;
		case 0xce: // DEC absolute
			memarg = get_absolute_arg();
			set_memory(memarg, get_memory(memarg) - 1);
			cycles = 5;
			break;
		case 0xde: // DEC absolute,x
			memarg = get_absolute_x_arg();
			set_memory(memarg, get_memory(memarg) - 1);
			cycles = 5;
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
		case 0xb0: // BCS relative
			relative_diff = get_relative();
			if (get_carry_flag()) {
				byte currpage = pc_reg >> 8;
				pc_reg += relative_diff;
				cycles = 3 + ( currpage != pc_reg >> 8 ? 1 : 0);
			} else {
				cycles = 2;
			}
			break;
		case 0x90: // BCC relative
			relative_diff = get_relative();
			if (!get_carry_flag()) {
				byte currpage = pc_reg >> 8;
				pc_reg += relative_diff;
				cycles = 3 + ( currpage != pc_reg >> 8 ? 1 : 0);
			} else {
				cycles = 2;
			}
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
		case 0xd5: //CMP zero page,x
			compare_values(accum_reg, get_zero_page_x_value());
			cycles = 4;
			break;
		case 0xcd: //CMP absolute
			compare_values(accum_reg, get_absolute_value());
			cycles = 4;
			break;
		case 0xdd: //CMP absolute,x
			compare_values(accum_reg, get_absolute_x_value());
			cycles = 4;
			break;
		case 0xd9: //CMP absolute,y
			compare_values(accum_reg, get_absolute_y_value());
			cycles = 4;
			break;
		case 0xc1: //CMP (ind,x)
			compare_values(accum_reg, get_indexed_indirect_value());
			cycles = 6;
			break;
		case 0xd1: //CMP (ind),y
			compare_values(accum_reg, get_indirect_indexed_value());
			cycles = 5;
			break;
		case 0x24: // BIT zero page
			bit_values(accum_reg, get_zero_page_value());
			cycles = 3;
			break;
		case 0x2c: // BIT absolute
			bit_values(accum_reg, get_absolute_value());
			cycles = 4;
			break;
		case 0x29: // AND immediate
			and_values(get_immediate_value());
			cycles = 2;
			break;
		case 0x00: // BRK implied
			cycles = 7;
			cpu_running = FALSE;
			break;
		case 0xaa: // TAX implied
			x_reg = accum_reg;
			cycles = 2;
			break;
		case 0xa8: // TAY implied
			y_reg = accum_reg;
			cycles = 2;
			break;
		case 0x98: // TYA implied
			accum_reg = y_reg;
			cycles = 2;
			break;
		case 0x8a: // TXA implied
			accum_reg = x_reg;
			cycles = 2;
			break;
		case 0x9a: // TXS implied
			sp_reg = x_reg;
			cycles = 2;
			break;
		case 0x49: // EOR immediate
			eor_values(get_immediate_value());
			cycles = 2;
			break;
		case 0x45: // EOR zero page
			eor_values(get_zero_page_value());
			cycles = 2;
			break;
		case 0x55: // EOR zero page,x
			eor_values(get_zero_page_x_value());
			cycles = 2;
			break;
		case 0x4d: // EOR absolute
			eor_values(get_absolute_value());
			cycles = 2;
			break;
		case 0x5d: // EOR absolute,x
			eor_values(get_absolute_x_value());
			cycles = 2;
			break;
		case 0x59: // EOR absolute,y
			eor_values(get_absolute_y_value());
			cycles = 2;
			break;
		case 0x41: // EOR indexed indirect
			eor_values(get_indexed_indirect_value());
			cycles = 2;
			break;
		case 0x51: // EOR indirect indexed
			eor_values(get_indirect_indexed_value());
			cycles = 2;
			break;
		case 0x08: // PHP implied
			push_byte(flags_reg);
			cycles = 3;
			break;
		case 0x48: // PHA implied
			push_byte(accum_reg);
			cycles = 3;
			break;
		case 0x68: // PLA implied
			accum_reg = pop_byte();
			cycles = 4;
			break;
		case 0x28: // PLP implied
			flags_reg = pop_byte();
			cycles = 4;
			break;
		case 0xf8: // SED implied
			set_decimal_flag();
			cycles = 2;
			break;
		case 0xd8: // CLD implied
			clear_decimal_flag();
			cycles = 2;
			break;
		case 0x78: // SEI implied
			set_interrupt_flag();
			cycles = 2;
			break;
		case 0x58: // CLI implied
			clear_interrupt_flag();
			cycles = 2;
			break;
		case 0x38: // SEC implied
			set_carry_flag();
			cycles = 2;
			break;
		case 0x18: // CLC implied
			clear_carry_flag();
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

/**
 * Walks the mem_segment chain to find the segment that
 * contains that address, irrespective of segment type
 */
mem_segment *find_mem_segment(maddress forlocation) {
	if (last_segment && last_segment->start >= forlocation && last_segment->end <= forlocation) {
		return last_segment;
	} else {
		mem_segment *curr_segment = first_mem_segment;
		while (curr_segment) {
			if (curr_segment->start <= forlocation && curr_segment->end >= forlocation) {
				break;
			}
			curr_segment = curr_segment->next_segment;
		}
		last_segment = curr_segment;
		return curr_segment;
	}
}

/**
 * Walk the mem_segment chain and add size of any
 * segments matching the type
 */
uint16_t get_memory_size_by_type(byte segment_type) {
	uint16_t total = 0;
	mem_segment *curr_segment = first_mem_segment;
	while (curr_segment) {
		if (curr_segment->segment_type == segment_type) {
			total += curr_segment->end - curr_segment->start;
		}
		curr_segment = curr_segment->next_segment;
	}
	return total;
}

/**
 * Walk the mem_segment chain and find the first segment
 * matching the specified type, returning the address in the
 * least significant 16 bits.  If no such segment type exists,
 * will return > 0xFFFF.
 */
int32_t find_first_segment_address_by_type(byte segment_type) {
	mem_segment *curr_segment = first_mem_segment;
	while (curr_segment) {
		if (curr_segment->segment_type == segment_type) {
			break;
		}
		curr_segment = curr_segment->next_segment;
	}
	return (curr_segment ? curr_segment->start : 0xffffffff);
}

uint16_t rom_size() {
	return get_memory_size_by_type(SEGMENT_TYPE_ROM);
}

uint16_t ram_size() {
	return get_memory_size_by_type(SEGMENT_TYPE_RAM);
}

maddress rom_start() {
	return (maddress)(find_first_segment_address_by_type(SEGMENT_TYPE_ROM) & 0x0000ffff);
}

void set_memory(maddress location, byte value) {
	if (location == 0xd012) { // send char to output
//		printf("send char: %02x\n", value);
		if (value == 0x0a) {
//			printf("\r");
		} else if (value == 0x0d) {
			printf("\n\r");
		} else {
			printf("%c", value);
		}
		ram[location] = value & 0b10111111;
	} else {
		mem_segment *segment = find_mem_segment(location);
		if (segment && (segment->segment_type != SEGMENT_TYPE_ROM || !rom_locked)) {
			ram[location] = value;
		}
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
	} else {
		result = ram[location];
	};

	return result;
}

void set_pc_register(maddress address) {
	pc_reg = address;
}

mem_segment* create_mem_segment(byte segment_type, maddress start, maddress end, mem_segment *prev) {
	mem_segment *result = malloc(sizeof(mem_segment));
	result->segment_type = segment_type;
	result->start = start;
	result->end = end;
	result->next_segment = NULL;
	if (prev) prev->next_segment = result;
	return result;
}

mem_segment* get_first_memory_segment() {
	return first_mem_segment;
}