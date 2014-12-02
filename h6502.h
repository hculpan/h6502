#ifndef _H6502_H_
#define _H6502_H_

#define ROM_SIZE 8192

#define RAM_SIZE 57344

#define ADC "ADC"
#define AND "AND"
#define ASL "ASL"
#define BCC "BCC"
#define BCS "BCS"
#define BEQ "BEQ"
#define BIT "BIT"
#define BMI "BMI"
#define BNE "BNE"
#define BPL "BPL"
#define BRK "BRK"
#define BVC "BVC"
#define BVS "BVS"
#define CLC "CLC"
#define CLD "CLD"
#define CLI "CLI"
#define CLV "CLV"
#define CMP "CMP"
#define CPX "CPX"
#define CPY "CPY"
#define DEC "DEC"
#define DEX "DEX"
#define DEY "DEY"
#define EOR "EOR"
#define INC "INC"
#define INX "INX"
#define INY "INY"
#define JMP "JMP"
#define JSR "JSR"
#define LDA "LDA"
#define LDX "LDX"
#define LDY "LDY"
#define LSR "LSR"
#define NOP "NOP"
#define ORA "ORA"
#define PHA "PHA"
#define PHP "PHP"
#define PLA "PLA"
#define PLP "PLP"
#define ROL "ROL"
#define ROR "ROR"
#define RTI "RTI"
#define RTS "RTS"
#define SBC "SBC"
#define SEC "SEC"
#define SED "SED"
#define SEI "SEI"
#define STA "STA"
#define STX "STX"
#define STY "STY"
#define TAX "TAX"
#define TAY "TAY"
#define TSX "TSX"
#define TXA "TXA"
#define TXS "TXS"
#define TYA "TYA"

const char *inst_mneumonics[256] = {
	BRK, ORA, NOP, NOP, NOP, ORA, ASL, NOP, PHP, ORA, ASL, NOP, NOP, ORA, ASL, NOP,
	BPL, ORA, NOP, NOP, NOP, ORA, ASL, NOP, CLC, ORA, NOP, NOP, NOP, ORA, ASL, NOP,
	JSR, AND, NOP, NOP, BIT, AND, ROL, NOP, PLP, AND, ROL, NOP, BIT, AND, ROL, NOP,
	BMI, AND, NOP, NOP, NOP, AND, ROL, NOP, SEC, AND, NOP, NOP, NOP, AND, ROL, NOP,
	RTI, EOR, NOP, NOP, NOP, EOR, LSR, NOP, PHA, EOR, LSR, NOP, JMP, EOR, LSR, NOP,
	BVC, EOR, NOP, NOP, NOP, EOR, LSR, NOP, CLI, EOR, NOP, NOP, NOP, EOR, LSR, NOP,
	RTS, ADC, NOP, NOP, NOP, ADC, ROR, NOP, PLA, ADC, ROR, NOP, JMP, ADC, ROR, NOP,
	BVS, ADC, NOP, NOP, NOP, ADC, ROR, NOP, SEI, ADC, NOP, NOP, NOP, ADC, ROR, NOP,
	NOP, STA, NOP, NOP, STY, STA, STX, NOP, DEY, NOP, TXA, NOP, STY, STA, STX, NOP,
	BCC, STA, NOP, NOP, STY, STA, STX, NOP, TYA, STA, TXS, NOP, NOP, STA, NOP, NOP,
	LDY, LDA, LDX, NOP, LDY, LDA, LDX, NOP, TAY, LDA, TAX, NOP, LDY, LDA, LDX, NOP,
	BCS, LDA, NOP, NOP, LDY, LDA, LDX, NOP, CLV, LDA, TSX, NOP, LDY, LDA, LDX, NOP,
	CPY, CMP, NOP, NOP, CPY, CMP, DEC, NOP, INY, CMP, DEX, NOP, CPY, CMP, DEC, NOP,
	BNE, CMP, NOP, NOP, NOP, CMP, DEC, NOP, CLD, CMP, NOP, NOP, NOP, CMP, DEC, NOP,
	CPX, SBC, NOP, NOP, CPX, SBC, INC, NOP, INX, SBC, NOP, NOP, CPX, SBC, INC, NOP,
	BEQ, SBC, NOP, NOP, NOP, SBC, INC, NOP, SED, SBC, NOP, NOP, NOP, SBC, INC, NOP
};

#endif