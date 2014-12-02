#include <stdio.h>
#include <string.h>

#include "h6502.h"
#include "cpu.h"

void before_current_op(struct cpu_status *cpustatus) {
	printf("-after--------------------------\n");
	printf("| a=%02x        x=%02x        y=%02x |\n", cpustatus->a, cpustatus->x, cpustatus->y);
	printf("| pc=%04x     s=%02x             |\n", cpustatus->pc, cpustatus->s);

	char b[9];
	b[8] = '\0';
	byte test = 1;
	for (int i = 7; i >= 0; i--) {
		b[i] = (cpustatus->flags & test ? '1': '0');
		test = test << 1;
//		printf("flags=%d test=%d", cpustatus->flags, test);
	}

	printf("| flags=%s                 |\n", b);
	printf("--------------------------------\n");
}

void after_current_op(struct cpu_status *cpustatus) {
	printf("-after--------------------------\n");
	printf("| a=%02x        x=%02x        y=%02x |\n", cpustatus->a, cpustatus->x, cpustatus->y);
	printf("| pc=%04x     s=%02x             |\n", cpustatus->pc, cpustatus->s);

	char b[9];
	b[8] = '\0';
	byte test = 1;
	for (int i = 7; i >= 0; i--) {
		b[i] = (cpustatus->flags & test ? '1': '0');
		test = test << 1;
//		printf("flags=%d test=%d", cpustatus->flags, test);
	}

	printf("| flags=%s                 |\n", b);
	printf("| lastop=%02x  (%s)             |\n", cpustatus->lastop, inst_mneumonics[cpustatus->lastop]);
	printf("--------------------------------\n");
}

int main(int argc, char *argv[]) {
	printf("H6502 Emulator\n");
	printf("Initializing...\n");
	init_cpu(ROM_SIZE, RAM_SIZE);
	printf("System ready\n");
	printf("%d bytes ROM, %d bytes RAM available\n", rom_size(), ram_size());
	struct cpu_status *cpustatus = malloc(sizeof(struct cpu_status));
	get_cpu_status(cpustatus);
	before_current_op(cpustatus);
	start_cpu(0, &after_current_op, cpustatus);

}