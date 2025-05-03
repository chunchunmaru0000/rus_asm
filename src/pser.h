#include "tzer.h"

uc sc(char *, const char *);

struct Pser {
	struct PList *ts; // tokens
	long pos;
	uc debug;
};
struct Pser *new_pser(char *, uc);
struct PList *pse(struct Pser *); // instructions

enum OCode {  // operand type codes
	OINT,	  // integer numbers negative number are handled in tzer
	OFPN,	  // floating point numbers that just different for the parser
	OREL,	  // labels and vars relative offsets
	OREG,	  // general purpose registers
	OSIMD,	  // single instruction multiple data registers
	OMEM_REG, // [reg]
	OMEM_REL, // [label or var]
	// OMEM_INT, // not for now
	OMEM_REG_INT, // [reg + int]
	OMEM_REL_INT, // [label or var + int] // is there such an op codes in cpu?
	// [rbx + rcx*4 + 0x10] something like this also in the future
	// OSEG, // segment registers cs, ds, fs, gs
	// OCR // control registers?
	// ODR // debug registers?
	// IOR // i/o registers? are there even these
	OFPU, // FPU registers and things
};

enum RegCode {
	R_NONE,
	R_EIP,
	R_EFLAGS,

	R_EAX,
	R_ECX,
	R_EDX,
	R_EBX,
	R_ESP,
	R_EBP,
	R_ESI,
	R_EDI,

	R_R8D,
	R_R9D,
	R_R10D,
	R_R11D,
	R_R12D,
	R_R13D,
	R_R14D,
	R_R15D,
};

struct Reg {
	enum RegCode c;
	const char *v;
};

struct Oper { // operand
	enum OCode code;
	struct Token *t;
	uc sz;
	// ---possible values---
	enum RegCode rcode;
	// enum SimdCode scode;
	// all other needed enums also
};

enum ICode {
	IEOI, // end of instructions
	IENTRY,
	ISEGMENT,
	ISECTION,
	ILABEL,
	ILET,
	IMOV,
	// IMOV_EAX_INT,
	// IMOV_EDX_INT,
	// IMOV_EDI_INT,
	// IMOV_ESI_INT,
	// IMOV_ESI_LABEL,
	IJMP,
	IADD,
	ISUB,
	IIMUL,
	ISYSCALL,
	ICALL,
	IRET,
	INOP,
	ISET_VAR,
};

struct Inst {
	enum ICode code;
	struct PList *os; // operands
	long col;
	long line;
	char *file;
};
struct Inst *new_inst(enum ICode, struct PList *, struct Token *);
