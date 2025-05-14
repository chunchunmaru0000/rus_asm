#include "tzer.h"
#include <stdint.h>

#define BYTE 1
#define WORD 2
#define DWORD 4
#define QWORD 8
#define SCALE_1 0b00
#define SCALE_2 0b01
#define SCALE_4 0b10
#define SCALE_8 0b11
#define MOD_MEM 0b00
#define MOD_MEM_D8 0b01
#define MOD_MEM_D32 0b10
#define MOD_REG 0b11
#define REX_B 0b0001
#define REX_X 0b0010
#define REX_R 0b0100
#define REX_W 0b1000

extern const char *const COLOR_BLACK;
extern const char *const COLOR_RED;
extern const char *const COLOR_GREEN;
extern const char *const COLOR_YELLOW;
extern const char *const COLOR_BLUE;
extern const char *const COLOR_PURPLE;
extern const char *const COLOR_GAY;
extern const char *const COLOR_WHITE;
extern const char *const COLOR_RESET;
#define color_print(c, msg) (printf("%s%s%s", (c), (msg), COLOR_RESET))

uc sc(char *, const char *);
#define REL_SIZE 4
enum UT { // Usage Type
	ADDR,
	REL_ADDR,
};

struct Usage {
	uint64_t place;
	enum UT type;
};
struct Usage *new_usage(uint64_t, enum UT);

struct Pser {
	struct PList *ts; // tokens
	long pos;
	uc debug;
	struct PList *ds; // #define's
};
struct Pser *new_pser(char *, uc);
struct PList *pse(struct Pser *); // instructions

enum OCode { // operand type codes
	OINT,	 // integer numbers negative number are handled in tzer
	OFPN,	 // floating point numbers that just different for the parser
	OREL,	 // labels and vars relative offsets
	OREG,	 // general purpose registers
	OMEM,	 // [] size is size of entire op
	OSIMD,	 // single instruction multiple data registers
	// like word[rax] is size of word, not qword
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

	R_AL,
	R_CL,
	R_DL,
	R_BL,
	R_AH,
	R_CH,
	R_DH,
	R_BH,
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
	// TODO: LOCK will be an instruction as its always first pref
};

struct Reg {
	enum RegCode c;
	const char *v;
};

struct Oper { // operand
	enum OCode code;
	struct Token *t;
	uc sz;
	enum RegCode rm; // OMEM and rsp then does SIB, mod 00 and rbp does rel
	uc mem_sz; // size of regs in addr DWORD or QWORD, DWORD means 0x67 flag
	uc rex;
	// ---SIB---
	enum RegCode base;
	enum RegCode index;
	uc scale; // 1 2 4 8
	// ---displacement---
	uc disp_is_rel_flag; // if 0 then use disp else view of *rel
	uc mod;				 // mod specifies displaacement size or r/m
	char *rel_view;
	int disp; // displacement

	// enum SimdCode scode;
	// all other needed enums also
};
void print_oper(struct Oper *);
int get_reg_field(enum RegCode);

struct Defn {
	char *view;
	void *value;
};

struct Inst {
	enum ICode code;
	struct PList *os; // operands
	long col;
	long line;
	char *file;
};
struct Inst *new_inst(enum ICode, struct PList *, struct Token *);
void pwi(const char *const, const char *, struct Inst *);

#define is_r8(o) ((o)->code == OREG && ((o)->rm >= R_AL && (o)->rm <= R_R15B))
#define is_r16(o) ((o)->code == OREG && ((o)->rm >= R_AX && (o)->rm <= R_R15W))
#define is_r32(o) ((o)->code == OREG && ((o)->rm >= R_EAX && (o)->rm <= R_R15D))
#define is_r64(o) ((o)->code == OREG && ((o)->rm >= R_RAX && (o)->rm <= R_R15))
#define is_rsp_addr(o)                                                         \
	((o)->code == OREG && ((o)->rm == R_RSP || (o)->rm == R_ESP))
#define is_r12_addr(o)                                                         \
	((o)->code == OREG && ((o)->rm == R_R12 || (o)->rm == R_R12D))
#define is_rbp_addr(o)                                                         \
	((o)->code == OREG && ((o)->rm == R_RBP || (o)->rm == R_EBP))
#define is_r13_addr(o)                                                         \
	((o)->code == OREG && ((o)->rm == R_R13 || (o)->rm == R_R13D))

#define is_r_new(o)                                                            \
	((o)->code == OREG && (((o)->rm >= R_R8 && (o)->rm <= R_R15) ||            \
						   ((o)->rm >= R_R8D && (o)->rm <= R_R15D) ||          \
						   ((o)->rm >= R_R8W && (o)->rm <= R_R15W) ||          \
						   ((o)->rm >= R_R8B && (o)->rm <= R_R15B)))

#define is_f_reg8(rm) ((rm) >= R_AL && (rm) <= R_R15B)
#define is_f_reg16(rm) ((rm) >= R_AX && (rm) <= R_R15W)
#define is_f_reg32(rm) ((rm) >= R_EAX && (rm) <= R_R15D)
#define is_f_reg64(rm) ((rm) >= R_RAX && (rm) <= R_R15)

#define is_reg(o) ((o)->code == OREG)
#define is_mem(o) ((o)->code == OMEM)
#define is_rm(o) (is_reg((o)) || is_mem((o)))
#define is_imm(o) ((o)->code == OINT || (o)->code == OFPN || (o)->code == OREL)
#define is_seg(o) ((o)->code == OSREG)
#define is_8(o) ((o)->sz == BYTE)
#define is_16(o) ((o)->sz == WORD)
#define is_32(o) ((o)->sz == DWORD)
#define is_64(o) ((o)->sz == QWORD)
#define is_al(o) ((o)->code == OREG && (o)->rm == R_AL)
#define is_rA(o)                                                               \
	((o)->code == OREG &&                                                      \
	 ((o)->rm == R_AX || (o)->rm == R_EAX || (o)->rm == R_RAX))
#define is_moffs(o) ((o)->code == OMOFFS)
// opcode reg field, meaningless name
#define is_sib(o) ((o)->rm == R_RSI)
#define is_mem32(o) ((o)->code == OMEM && (o)->mem_sz == DWORD)
#define is_imm_can_be_a_byte(o)                                                \
	((o)->code == OINT && o->t->number >= -128 && o->t->number <= 127)

enum RegCode get_mem_reg(enum RegCode);
void eeg(const char *, struct Inst *);
