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

	printf("  PC   AC  XR  YR  SP  NV-BDIZC  PC    OP       PC+1 PC+2\n");
	printf(" %04x  %02x  %02x  %02x  %02x  %s  %04x  %02x (%s)  %02x   %02x\n\n",
		cpustatus->pc,
		cpustatus->a,
		cpustatus->x,
		cpustatus->y,
		cpustatus->s,
		b,
		cpustatus->pc,
		get_memory(cpustatus->pc),
		inst_mneumonics[get_memory(cpustatus->pc)],
		get_memory(cpustatus->pc + 1),
		get_memory(cpustatus->pc + 2)
		);
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

byte parse_command(char *cmd) {
	byte result = FALSE;
	char *c, *d, *lasts;
	c = strtok_r(cmd, " ", &lasts);
	d = strtok_r(NULL, " ", &lasts);
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
	} else if (strcmp("s", c) == 0 || strcmp("step", c) == 0) {
		if (d) {
			char *p;
			set_pc_register(strtol(d, &p, 16));
		}
		struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
		step(0, &cpu_status_callback, cpustatus);
		free(cpustatus);
	} else if (strcmp("start", c) == 0) {
		if (d) {
			char *p;
			set_pc_register(strtol(d, &p, 16));
		}
		struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
		start_cpu(0, &cpu_status_callback, cpustatus);
		free(cpustatus);
	} else if (strcmp("r", c) == 0 || strcmp("reg", c) == 0) {
		struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
		get_cpu_status(cpustatus);
		display_cpu_status(cpustatus);
		free(cpustatus);
	} else if (strcmp("h", c) == 0 || strcmp("help", c) == 0) {
		printf("  m, mem [address]\tMemory dump\n");
		printf("  out\t\t\tToggle display of registers at each execution step\n");
		printf("  r, reg\t\tDisplay cpu registers\n");
		printf("  s, step [address]\tExecute address as single step\n");
		printf("  start [address]\tStart execution at current PC register\n");
		printf("  q, quit\t\tExit simulator\n");
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
//	start_cpu(0, &after_current_op, cpustatus);
}

int main(int argc, char *argv[]) {
	printf("H6502 Emulator\n");
	printf("Initializing...\n");
	init_cpu(ROM_SIZE, RAM_SIZE);
	printf("System ready\n");
	printf("%d bytes ROM, %d bytes RAM available\n", rom_size(), ram_size());
	monitor_command();
}