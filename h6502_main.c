#include <stdio.h>
#include <string.h>

#include "h6502.h"
#include "cpu.h"

maddress monitor_current_address = 0x0200;

byte error = FALSE;

char last_error[256];

byte output_exec_steps = FALSE;

void display_cpu_status(struct cpu_status *cpustatus) {
 // ADDR AC XR YR SP 00 01 NV-BDIZC LIN CYC  STOPWATCH
 //.;e5cf 00 00 0a f3 2f 37 00100010 012 001    3351401
	char b[9];
	b[8] = '\0';
	byte test = 1;
	for (int i = 7; i >= 0; i--) {
		b[i] = (cpustatus->flags & test ? '1': '0');
		test = test << 1;
	}

	printf("  PC   AC  XR  YR  SP  NV-BDIZC  PC    OP              PC+1 PC+2\n");
	printf(" %04x  %02x  %02x  %02x  %02x  %s  %04x  %02x (%s %s)  \t%02x   %02x\n\n",
		cpustatus->pc,
		cpustatus->a,
		cpustatus->x,
		cpustatus->y,
		cpustatus->s,
		b,
		cpustatus->pc,
		get_memory(cpustatus->pc),
		inst_mneumonics[get_memory(cpustatus->pc)],
		inst_addressing[get_memory(cpustatus->pc)],
		get_memory(cpustatus->pc + 1),
		get_memory(cpustatus->pc + 2)
		);
}

void cpu_fault(struct cpu_status *cpustatus) {
	printf("FAULT: %s\n", cpustatus->message);
	display_cpu_status(cpustatus);
}

void cpu_status_callback(struct cpu_status *cpustatus) {
	if (output_exec_steps) {
		display_cpu_status(cpustatus);
	}	
};

void mem_display() {
	for (int i = 0; i < 8; i++) {
		printf("%04x: ", monitor_current_address + (i * 16));
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				byte mem = get_memory(monitor_current_address + (i * 16) + (j * 4) + k);
				printf("%02x ", mem);
			}
			printf(" ");
		}
		printf("\n");
	}
	monitor_current_address += 16 * 8;
}

byte get_size_of_param(byte instr) {
	const char *addr_mode = inst_addressing[instr];
	if (strcmp(IMP, addr_mode) == 0 || strcmp(ACC, addr_mode) == 0 || addr_mode[0] == 'N') {
		return 0;
	} else if (addr_mode[0] == 'I' || addr_mode[0] == 'Z' || addr_mode[0] == 'R') {
		return 1;
	} else {
		return 2;
	}
}

maddress disassemble(maddress address) {
	byte instr = get_memory(address);
	byte param_size = get_size_of_param(instr);
	if (param_size == 0) {
		printf("   %04x\t%02x\t\t: %s\n", address, instr, inst_mneumonics[instr]);
	} else if (param_size == 1 && strcmp(IMM, inst_addressing[instr]) == 0) {
		printf("   %04x\t%02x %02x\t\t: %s\t#$%02x\n", address, instr, get_memory(address + 1), inst_mneumonics[instr], get_memory(address + 1));
	} else if (param_size == 1 && strcmp(INDX, inst_addressing[instr]) == 0) {
		printf("   %04x\t%02x %02x\t\t: %s\t($%02x,x)\n", address, instr, get_memory(address + 1), inst_mneumonics[instr], get_memory(address + 1));
	} else if (param_size == 1 && strcmp(INDY, inst_addressing[instr]) == 0) {
		printf("   %04x\t%02x %02x\t\t: %s\t($%02x),y\n", address, instr, get_memory(address + 1), inst_mneumonics[instr], get_memory(address + 1));
	} else if (param_size == 1 && strcmp(ZPX, inst_addressing[instr]) == 0) {
		printf("   %04x\t%02x %02x\t\t: %s\t$%02x,x\n", address, instr, get_memory(address + 1), inst_mneumonics[instr], get_memory(address + 1));
	} else if (param_size == 1 && strcmp(ZPX, inst_addressing[instr]) == 0) {
		printf("   %04x\t%02x %02x\t\t: %s\t$%02x,y\n", address, instr, get_memory(address + 1), inst_mneumonics[instr], get_memory(address + 1));
	} else if (param_size == 1) {
		printf("   %04x\t%02x %02x\t\t: %s\t$%02x\n", address, instr, get_memory(address + 1), inst_mneumonics[instr], get_memory(address + 1));
	} else if (strcmp(ABX, inst_addressing[instr]) == 0) {
		printf("   %04x\t%02x %02x %02x\t: %s\t$%04x,x\n", address, instr, 
			get_memory(address + 1), get_memory(address + 2), 
			inst_mneumonics[instr], 
			WORD_LOHI(get_memory(address + 1), get_memory(address + 2))
			);
	} else if (strcmp(ABY, inst_addressing[instr]) == 0) {
		printf("   %04x\t%02x %02x %02x\t: %s\t$%04x,y\n", address, instr, 
			get_memory(address + 1), get_memory(address + 2), 
			inst_mneumonics[instr], 
			WORD_LOHI(get_memory(address + 1), get_memory(address + 2))
			);
	} else {
		printf("   %04x\t%02x %02x %02x\t: %s\t$%04x\n", address, instr, 
			get_memory(address + 1), get_memory(address + 2), 
			inst_mneumonics[instr], 
			WORD_LOHI(get_memory(address + 1), get_memory(address + 2))
			);
	}

	return param_size + 1;
}

void display_memory_map() {
	mem_segment* curr_segment = get_first_memory_segment();
	char seg_name[10];
	while (curr_segment) {
		if (curr_segment->segment_type == 0) {
			strcpy(seg_name, "RAM");
		} else if (curr_segment->segment_type == 1) {
			strcpy(seg_name, "ROM");
		} else if (curr_segment->segment_type == 2) {
			strcpy(seg_name, "DMA");
		} else {
			strcpy(seg_name, "UKN");
		}
		printf("%10s: %04x-%04x\t\tsize=%02dk\n",
			seg_name,
			curr_segment->start,
			curr_segment->end,
			(int)((curr_segment->end - curr_segment->start + 1)/1024));
		curr_segment = curr_segment->next_segment;
	}
}

byte parse_command(char *cmd) {
	byte result = FALSE;
	char *c, *d, *e, *lasts;
	c = strtok_r(cmd, "[ ,]", &lasts);
	d = strtok_r(NULL, "[ ,]", &lasts);
	e = strtok_r(NULL, "[ ,]", &lasts);
	if ((strcmp("m", c) == 0 || strcmp("mem", c) == 0) && !d) {
		mem_display();
	} else if (strcmp("m", c) == 0 || strcmp("mem", c) == 0) {
		char *p;
		monitor_current_address = strtol(d, &p, 16);
		mem_display();
	} else if (strcmp("q", c) == 0 || strcmp("quit", c) == 0 || strcmp("exit", c) == 0) {
		result = TRUE;
	} else if (strcmp("out", c) == 0) {
		output_exec_steps = !output_exec_steps;
		printf("OUTPUT=%d\n", output_exec_steps);
	} else if (strcmp("p", c) == 0 || strcmp("put", c) == 0) {
		if (!d || !e) {
			printf("Required parameters not found\n");
		} else {
			char *p;
			long address = strtol(d, &p, 16);
			unsigned short value = (unsigned short)strtoul(e, &p, 16);
			set_memory(address, value);
		}
	} else if (strcmp("d", c) == 0 || strcmp("dis", c) == 0) {
		maddress address = monitor_current_address;
		if (d) {
			char *p;
			address = strtol(d, &p, 16);
		}
		for (int i = 0; i < 20; i++) {
			address += disassemble(address);
		}
	} else if (strcmp("s", c) == 0 || strcmp("step", c) == 0) {
		if (d) {
			char *p;
			set_pc_register(strtol(d, &p, 16));
		}
		struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
		step(&cpu_status_callback, &cpu_fault, cpustatus);
		free(cpustatus);
	} else if (strcmp("b", c) == 0 || strcmp("bin", c) == 0) {
		if (d) {
			char *p;
			set_pc_register(strtol(d, &p, 16));
		}
		struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
		step(&cpu_status_callback, &cpu_fault, cpustatus);
		free(cpustatus);
	} else if (strcmp("map", c) == 0) {
		display_memory_map();
	} else if (strcmp("start", c) == 0) {
		if (d) {
			char *p;
			set_pc_register(strtol(d, &p, 16));
		}
		struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
		start_cpu(&cpu_status_callback, &cpu_fault, cpustatus);
		free(cpustatus);
	} else if (strcmp("r", c) == 0 || strcmp("reg", c) == 0) {
		struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
		get_cpu_status(cpustatus);
		display_cpu_status(cpustatus);
		free(cpustatus);
	} else if (strcmp("h", c) == 0 || strcmp("help", c) == 0) {
		printf("  b, bin [name],[address]\tLoad binary file\n");
		printf("  d, dis [address]\t\tDisassemble\n");
		printf("  l, load [name],[address]\tLoad file\n");
		printf("  m, mem [address]\t\tMemory dump\n");
		printf("  map\t\t\t\tDisplay memory map\n");
		printf("  out\t\t\t\tToggle display at each execution step\n");
		printf("  p, put [address],[value]\tPut the value at address\n");
		printf("  r, reg\t\t\tDisplay cpu registers\n");
		printf("  s, step [address]\t\tExecute address as single step\n");
		printf("  start [address]\t\tStart execution at current PC register\n");
		printf("  q, quit\t\t\tExit simulator\n");
	} else {
		printf("Unrecognized command: '%s'\n", cmd);
	}
	return result;
}

void monitor_command() {
	byte quitting = FALSE;

	char cmd[256];
	while (!quitting) {
		cmd[0] = '\0';
		printf("(%04x) ", monitor_current_address);
		fgets(cmd, 256, stdin);
		if ((strlen(cmd) > 0) && (cmd[strlen(cmd) - 1] == '\n')) {
	        cmd[strlen (cmd) - 1] = '\0';
		}
   		quitting = parse_command(cmd);
	}
}

int main(int argc, char *argv[]) {
	printf("H6502 Emulator\n");

	mem_segment *first = create_mem_segment(SEGMENT_TYPE_RAM, 0, 0xcfff, NULL);
	mem_segment *next = create_mem_segment(SEGMENT_TYPE_DMA, 0xd000, 0xdfff, first);
	next = create_mem_segment(SEGMENT_TYPE_ROM, 0xe000, 0xffff, next);

	printf("Initializing...\n");
	init_cpu(first);
	printf("System ready\n");
	printf("%d bytes ROM, %d bytes RAM available\n", rom_size(), ram_size());
	monitor_command();
}