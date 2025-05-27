#include "lbel.h"
#include <stdint.h>

extern const char *const WRONG_FST_OPER;
extern const char *const WRONG_FST_OPER_INT;
extern const char *const WRONG_SND_OPER;
extern const char *const WRONG_FST_OPER_REG_DWORD;
extern const char *const WRONG_FST_OPER_REG_QWORD;
extern const char *const WRONG_SND_OPER_SIZE;
extern const char *const WRONG_FPN_OP_SIZE;
extern const char *const REGS_SIZES_NOT_MATCH;
extern const char *const MEM_REG_SIZES_NOT_MATCH;
extern const char *const REG_MEM_IMM_SIZES_NOT_MATCH;
extern const char *const UNKNOWN_LABEL;

// for label and maybe variables that have a string view and ptr to value
struct Plov {		   // Pointer Label Of Value
	char *label;	   // label
	uint64_t addr;	   // adress
	uint64_t rel_addr; // relative adress in executable file from file start
	int si;			   //  segment place
	struct PList *us;  // usages
					   // uint64_t size; // like for db dd dw dq
};

enum OpsCode {
	OPC_INVALID,
	__REL_8, // rel is just a number
	__REL_32,
	__IMM_8,
	__IMM_32,
	__R_16_64, // 50+r, 58+r
	__RM_16_64,
	__GS,
	__FS,
	__RM_8, // inc and dec
	__RM_16_32_64,

	RM_8__R_8,
	RM_16_32_64__R_16_32_64,
	R_8__RM_8,
	R_16_32_64__RM_16_32_64,
	AL__IMM_8,
	RAX__IMM_16_32,
	RM_8__IMM_8,
	RM_16_32_64__IMM_16_32,
	RM_16_32_64__IMM_8,

	M_16__SREG,
	R_16_32_64__SREG,
	SREG__RM_16,

	AL__MOFFS_8,
	RAX__MOFFS_16_32_64,
	MOFFS_8__AL,
	MOFFS_16_32_64__RAX,

	R_8__IMM_8,
	R_16_32_64__IMM_16_32_64,
	// M_8__M_8, // they are not exists movs odes via rep movsq or just movsq
	// M_16_32_64__M_16_32_64, // it doesn have operands
	R_16_32_64__RM_16_32_64__IMM_16_32,
	R_16_32_64__RM_16_32_64__IMM_8,
};

#define NOT_FIELD 0
#define NUM_FIELD 1
#define REG_FIELD 2
#define PLUS_REGF 3

struct Cmnd {
	enum ICode inst;
	// instruction code
	uc cmd[4];
	// command bytes
	uc len;
	// cmd len
	uc o;
	// o Register/ Opcode Field
	//   0. NOT_FIELD just op code
	//   1. NUM_FIELD The value of the opcode extension values from 0 through 7
	//   - like ModR/M byte where Reg field is for o_num
	//   - - primary used with imm or ?const regs?
	//   - this "ModR/M" byte also have mod and if its just reg and imm then
	//   - - mod = 11 and R/M field means just reg code
	//   - if mod != 11 then it behaves as
	//   - - just usual mod and R/M fields with SIB if needed
	//   2. REG_FIELD r indicates that the ModR/M byte contains a register
	//   - operand and an r/m operand. 00 ADD
	//   - - primary used with
	//   - - - r__rm or rm__r, lea uses r__m
	//   - - - also sreg__rm or rm__sreg
	//   - - - also xmm, CRn, DRn and maybe more shit
	//   3. PLUS_REGF When just op code + reg code
	//   - BSWAP, PUSH r64/16, POP r64/16, XCHG r16/32/64 rAX
	//   - B0+r MOV r8 imm8, B8+r MOV r16/32/64 imm16/32/64
	uc o_num;
	// instruction number if o == NUM_FIELD
	enum OpsCode opsc;
	// instruction opperands
};

struct Ipcd {
	struct Inst *in;
	struct PList *not_plovs;
	struct BList *cmd;
	struct BList *data;
	const struct Cmnd *c;
	uc debug;
};
void get_ops_code(struct Ipcd *);

#define is_rm__r(c) ((c) == RM_8__R_8 || (c) == RM_16_32_64__R_16_32_64)
#define is_r__rm(c) ((c) == R_8__RM_8 || (c) == R_16_32_64__RM_16_32_64)
#define is_rel(c) ((c) == __REL_8 || (c) == __REL_32)
