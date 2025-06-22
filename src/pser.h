#include "tzer.h"
#include <stdint.h>

#define RESERVED -1
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
extern uc NEED_WARN;

struct Pser {
	struct Fpfc *f;
	struct PList *ts; // tokens
	size_t pos;
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
	OXMM,	 // single instruction multiple data registers
	OSREG,	 // segment registers cs, ds, fs, gs
	OMOFFS,
	// OCR // control registers?
	// ODR // debug registers?
	// OIOR // i/o registers? are there even these
	OFPU, // FPU registers and things
	OMM,  // mm registers
};

enum RegCode {
	R_NONE,

	R_ES,
	R_CS,
	R_SS,
	R_DS,
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

	R_XMM0,
	R_XMM1,
	R_XMM2,
	R_XMM3,
	R_XMM4,
	R_XMM5,
	R_XMM6,
	R_XMM7,
	R_XMM8,
	R_XMM9,
	R_XMM10,
	R_XMM11,
	R_XMM12,
	R_XMM13,
	R_XMM14,
	R_XMM15,

	R_MM0,
	R_MM1,
	R_MM2,
	R_MM3,
	R_MM4,
	R_MM5,
	R_MM6,
	R_MM7,
};

enum ICode {
	// directives
	INONE,
	IEOI, // end of instructions
	IENTRY,
	ISEGMENT,
	ISECTION,
	ILABEL,
	ILET,
	IINCLUDE,
	IALIGN,
	IDATA,
	// two ops
	IADD,
	IOR,
	IADC,
	ISBB,
	IAND,
	ISUB,
	IXOR,
	ICMP,
	ITEST,
	IMOV, // IMOVSXD, its just mov here
	IMUL,
	IIMUL,
	IDIV,
	IIDIV,
	IXCHG,
	ILEA,
	IROL,
	IROR,
	IRCL,
	IRCR,
	ISHL,
	ISHR,
	ISAR,
	IINPUT,
	IOUTPUT,
	// one op
	IROL1,
	IROR1,
	IRCL1,
	IRCR1,
	ISHL1,
	ISHR1,
	ISAR1,
	ICALL,
	IPUSH,
	IPOP,
	IINT,
	IINC,
	IDEC,
	ICALLF,
	IJMPF,

	IJMP,
	IJO,
	IJNO,
	IJB,
	IJNB,
	IJE,
	IJNE,
	IJBE,
	IJA,
	IJS,
	IJNS,
	IJP,
	IJNP,
	IJL,
	IJNL,
	IJLE,
	IJG,

	ILOOPNZ,
	ILOOPZ,
	ILOOP,
	IJECXZ,
	IJRCXZ,
	INEG,
	INOT,
	// zero ops
	IUSEFS,
	IUSEGS,
	IREPNZ,
	IREP,
	ICLC,
	ISTC,
	ICLI,
	ISTI,
	ICLD,
	ISTD,
	IINT3,
	IINT1,
	IHLT,
	ICMC,
	ISYSCALL,
	INOP,
	IRET,
	IRETF,
	IRETFQ,
	ILEAVE,
	IENTER,
	ISYSRET,
	ILOCK,
	IINSB,
	IINSW,
	IINSD,
	IOUTSB,
	IOUTSW,
	IOUTSD,
	IPAUSE,
	IWAIT,
	ICBW,
	ICWDE,
	ICDQE,
	ICWD,
	ICDQ,
	ICQO,
	IPUSHF,
	IPOPF,
	ISAHF,
	ILAHF,
	IMOVSB,
	IMOVSW,
	IMOVSD,
	IMOVSQ,
	ICMPSB,
	ICMPSW,
	ICMPSD,
	ICMPSQ,
	ISCASB,
	ISCASW,
	ISCASD,
	ISCASQ,
	ILODSB,
	ILODSW,
	ILODSD,
	ILODSQ,
	ISTOSB,
	ISTOSW,
	ISTOSD,
	ISTOSQ,
	// xmm
	I_XMM_INSTRUCTIONS_BEGIN,
	// __
	I_XMM_NO_PREFIX_BEGIN,

	IMOVUPS,
	IMOVHLPS,
	IMOVLPS,
	IUNPCKLPS,
	IUNPCKHPS,
	IMOVLHPS,
	IMOVHPS,
	IMOVAPS,
	ICVTPI2PS,
	IMOVNTPS,
	ICVTTPS2PI,
	ICVTPS2PI,
	IUCOMISS,
	ICOMISS,

	I_XMM_NO_PREFIX_END,
	// 66
	I_XMM_66_PREFIX_BEGIN,

	IMOVUPD,
	IMOVLPD,
	IUNPCKLPD,
	IUNPCKHPD,
	IMOVHPD,
	IUCOMISD,
	ICOMISD,
	IMOVNTPD,
	IMOVAPD,
	ICVTPI2PD,
	ICVTTPD2PI,
	ICVTPD2PI,

	I_XMM_66_PREFIX_END,
	// F2
	I_XMM_F2_PREFIX_BEGIN,

	IMOVSD_XMM,
	IMOVDDUP,
	ICVTSI2SD,
	ICVTTSD2SI,
	ICVTSD2SI,

	I_XMM_F2_PREFIX_END,
	// F3
	I_XMM_F3_PREFIX_BEGIN,

	IMOVSS,
	IMOVSLDUP,
	IMOVSHDUP,
	ICVTSI2SS,
	ICVTTSS2SI,
	ICVTSS2SI,

	I_XMM_F3_PREFIX_END,
	// 66 0F 3A 08 - 66 0F 3A 63
	// 0f 50 - F3 0F 7F
	// 0F C2 - 66 0F FE
};
// TODO: 0f 00 - 0f 0d

struct Reg {
	enum RegCode c;
	const char *v;
};

struct Oper { // operand
	enum OCode code;
	struct Token *t;
	uc sz;
	uc forsed_sz;	 // if like size override(like mov rax, qword[rax])
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
};
void print_oper(struct Oper *);
int get_reg_field(enum RegCode);

struct Defn {
	char *view;
	void *value;
};

struct Word {
	char *view;
	enum ICode inst;
};

struct Inst {
	enum ICode code;
	struct PList *os; // operands
	struct Fpfc *f;
	struct Pos *p;
};
struct Inst *new_inst(struct Pser *, enum ICode, struct PList *,
					  struct Token *);
void pw(struct Fpfc *f, struct Pos *p, const char *const msg);

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
#define is_x_new(o)                                                            \
	((o)->code == OXMM && ((o)->rm >= R_XMM8 && (o)->rm <= R_XMM15))

#define is_f_reg8(rm) ((rm) >= R_AL && (rm) <= R_R15B)
#define is_f_reg16(rm) ((rm) >= R_AX && (rm) <= R_R15W)
#define is_f_reg32(rm) ((rm) >= R_EAX && (rm) <= R_R15D)
#define is_f_reg64(rm) ((rm) >= R_RAX && (rm) <= R_R15)
#define is_f_xmm(rm) ((rm) >= R_XMM0 && (rm) <= R_XMM15)
#define is_f_mm(rm) ((rm) >= R_MM0 && (rm) <= R_MM7)
#define is_f_seg(rm) ((rm) >= R_ES && (rm) <= R_GS)

#define is_fs(o) ((o)->code == OSREG && (o)->rm == R_FS)
#define is_gs(o) ((o)->code == OSREG && (o)->rm == R_GS)
#define is_reg(o) ((o)->code == OREG)
#define is_mem(o) ((o)->code == OMEM)
#define is_rm(o) (is_reg((o)) || is_mem((o)))
#define is_imm(o) ((o)->code == OINT || (o)->code == OFPN || (o)->code == OREL)
#define is_seg(o) ((o)->code == OSREG)
#define is_xmm(o) ((o)->code == OXMM)
#define is_mm(o) ((o)->code == OMM)
#define is_mmm(o) (is_mem((o)) || is_mm((o)))
#define is_xm(o) (is_xmm((o)) || is_mem((o)))
#define is_8(o) ((o)->sz == BYTE)
#define is_16(o) ((o)->sz == WORD)
#define is_32(o) ((o)->sz == DWORD)
#define is_64(o) ((o)->sz == QWORD)
#define is_al(o) ((o)->code == OREG && (o)->rm == R_AL)
#define is_rA(o)                                                               \
	((o)->code == OREG &&                                                      \
	 ((o)->rm == R_AX || (o)->rm == R_EAX || (o)->rm == R_RAX))
#define is_eA(o) ((o)->code == OREG && ((o)->rm == R_AX || (o)->rm == R_EAX))
#define is_dx(o) ((o)->code == OREG && (o)->rm == R_DX)
#define is_moffs(o) ((o)->code == OMOFFS)
// opcode reg field, meaningless name
#define is_sib(o) ((o)->rm == R_RSI)
#define is_addr32(o)                                                           \
	(((o)->code == OMEM || (o)->code == OMOFFS) && (o)->mem_sz == DWORD)
#define is_in_byte(i) ((i) >= -128 && (i) <= 127)
#define is_imm_can_be_a_byte(o)                                                \
	((o)->code == OINT && is_in_byte((o)->t->number))
#define is_rel8_shortable(c) ((c) >= IJMP && (c) <= IJG)
#define is_NO(code)                                                            \
	((code) > I_XMM_NO_PREFIX_BEGIN && (code) < I_XMM_NO_PREFIX_END)
#define is_66(code)                                                            \
	((code) > I_XMM_66_PREFIX_BEGIN && (code) < I_XMM_66_PREFIX_END)
#define is_F2(code)                                                            \
	((code) > I_XMM_F2_PREFIX_BEGIN && (code) < I_XMM_F2_PREFIX_END)
#define is_F3(code)                                                            \
	((code) > I_XMM_F3_PREFIX_BEGIN && (code) < I_XMM_F3_PREFIX_END)

enum RegCode get_mem_reg(enum RegCode);
