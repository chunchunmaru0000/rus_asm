#include "../tzer/tzer.h"
#include "cdes.h"
#include "lbel.h"
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

void ee_token(struct Fpfc *f, struct Token *t, char *msg);
struct Token *gettp(struct Pser *p, long off);
#define cur_token(p) (gettp((p), 0))
struct Token *next_get(struct Pser *p, long off);
int is_size_word(char *v);
struct Defn *is_defn(struct Pser *p, char *v);

struct Pser *new_pser(char *, uc);
struct PList *pse(struct Pser *); // instructions

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
	enum RelFlags rel_flags;
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

struct Oper *expression(struct Pser *);
struct Oper *add_sub_expr(struct Pser *p);
#define bin_expr add_sub_expr
void print_oper(struct Oper *);
int get_reg_field(enum RegCode);

struct Word {
	char *view;
	enum ICode inst;
};

extern const struct Reg E_REGS[];
extern const struct Reg R_REGS[];
extern const struct Reg W_REGS[];
extern const struct Reg B_REGS[];
extern const struct Reg XMM_REGS[];
extern const struct Reg SEG_REGS[];
extern const struct Reg MM_REGS[];
extern const struct Word VAR_OPS_WORDS[];
extern const struct Word TWO_OPS_WORDS[];
extern const struct Word ZERO_OPS_WORDS[];
extern const struct Word ONE_OPS_WORDS[];
extern const struct Word TRI_OPS_WORDS[];

extern const uint32_t E_REGS_LEN;
extern const uint32_t R_REGS_LEN;
extern const uint32_t W_REGS_LEN;
extern const uint32_t B_REGS_LEN;
extern const uint32_t XMM_REGS_LEN;
extern const uint32_t SEG_REGS_LEN;
extern const uint32_t MM_REGS_LEN;
extern const uint32_t VAR_OPS_WORDS_LEN;
extern const uint32_t TWO_OPS_WORDS_LEN;
extern const uint32_t ZERO_OPS_WORDS_LEN;
extern const uint32_t ONE_OPS_WORDS_LEN;
extern const uint32_t TRI_OPS_WORDS_LEN;

extern const char *const STR__HERE;
extern const char *const STR__TUT;
#define arr_l(arr) (arr##_LEN)

struct Inst {
	enum ICode code;
	struct PList *os; // operands
	struct Fpfc *f;
	struct Pos *p;
};
struct Inst *new_inst(struct Pser *, enum ICode, struct PList *,
					  struct Token *);
void pw(struct Fpfc *f, struct Pos *p, const char *const msg);

#define is_r8h(o) ((o)->code == OREG && ((o)->rm >= R_AH && (o)->rm <= R_BH))
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
#define is_xr_new(o) (is_x_new((o)) || is_r_new((o)))

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
#define is_imm_can_be_a_byte(o) ((o)->code == OINT && is_in_byte((o)->t->num))
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
