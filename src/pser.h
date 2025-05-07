#include "tzer.h"

#define BYTE 1
#define WORD 2
#define DWORD 4
#define QWORD 8

uc sc(char *, const char *);

struct Pser {
	struct PList *ts; // tokens
	long pos;
	uc debug;
	struct PList *ds; // #define's
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
	OSREG, // segment registers cs, ds, fs, gs
	OMOFFS,
	// OCR // control registers?
	// ODR // debug registers?
	// IOR // i/o registers? are there even these
	OFPU, // FPU registers and things
};

enum RegCode {
	R_NONE,

	R_CS,
	R_DS,
	R_ES,
	R_SS,
	R_FS,
	R_GS,
	R_LDTR,
	R_TR,

	R_AH,
	R_AL,
	R_CH,
	R_CL,
	R_DH,
	R_DL,
	R_BH,
	R_BL,
	R_R8B,
	R_R9B,
	R_R10B,
	R_R11B,
	R_R12B,
	R_R13B,
	R_R14B,
	R_R15B,

	R_IP,
	R_FLAGS,
	R_AX,
	R_CX,
	R_DX,
	R_BX,
	R_SP,
	R_BP,
	R_SI,
	R_DI,
	R_R8W,
	R_R9W,
	R_R10W,
	R_R11W,
	R_R12W,
	R_R13W,
	R_R14W,
	R_R15W,

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

	R_RIP,
	R_RFLAGS,
	R_RAX,
	R_RCX,
	R_RDX,
	R_RBX,
	R_RSP,
	R_RBP,
	R_RSI,
	R_RDI,
	R_R8,
	R_R9,
	R_R10,
	R_R11,
	R_R12,
	R_R13,
	R_R14,
	R_R15,
};

enum ICode {
	INONE,
	IEOI, // end of instructions
	IENTRY,
	ISEGMENT,
	ISECTION,
	ILABEL,
	ILET,
	IDATA,

	IADD,
	IOR,
	IADC,
	ISBB,
	IAND,
	ISUB,
	IXOR,
	ICMP,
	ITEST,

	IPUSH,
	IPOP,
	IIMUL,
	IJMP,
	IMOV,

	IRET,
	INOP,

	ICALL,
	ISYSCALL,
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

struct Defn {
	char *view;
	struct Oper *value;
};

struct Inst {
	enum ICode code;
	struct PList *os; // operands
	long col;
	long line;
	char *file;
};
struct Inst *new_inst(enum ICode, struct PList *, struct Token *);
