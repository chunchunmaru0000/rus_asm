#include "pser.h"
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
extern const char *const ERR_WRONG_OPS_FOR_THIS_INST;
extern const char *const OPS_CODE_INVALID;
extern const char *const WARN_IMM_SIZE_WILL_BE_CHANGED;
extern const char *const WARN_CHANGE_IMM_SIZE;
extern const char *const MEM_IMM_SIZE_QWORD;
extern const char *const ERR_WRONG_BYTE_REG;
extern const char *const EXPEXTED_DWORD_OR_QWORD;

// for label and maybe variables that have a string view and ptr to value
struct Plov {		   // Pointer Label Of Value
	char *label;	   // label
	uint64_t addr;	   // adress
	uint64_t rel_addr; // relative adress in executable file from file start
	int si;			   // segment place
	struct PList *us;  // usages
	uint32_t ipos;
	uc declared;
};

enum OpsCode {
	OPC_INVALID,
	__REL_8, // rel is just a number
	__REL_32,
	__IMM_8,
	__IMM_16,
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
	R_16_32_64__RM_16_32_64__IMM_16_32,
	R_16_32_64__RM_16_32_64__IMM_8,
	R_64__RM_32,
	R_16_32_64__RAX,
	R_16_32_64__M,
	IMM_16__IMM_8,

	EAX__IMM_8,
	IMM_8__AL,
	IMM_8__EAX,
	AL__DX,
	EAX__DX,
	DX__AL,
	DX__EAX,

	X__XM_128,
	X__XM_32,
	X__XM_64,
	XM_128__X,
	XM_32__X,
	XM_64__X,
	X__M_64,
	M_64__X,
	X__X,
	X__MMM_64,
	X__RM_32_64,
	M_128__X,
	MM__XM_64,
	R_32_64__XM_32,
	MM__XM_128,
	R_32_64__XM_64,

	MM__MMM_64,
	X__XM_128__IMM_8,
	X__XM_32__IMM_8,
	X__XM_64__IMM_8,
	R_32_64_M_8__X__IMM_8,
	R_32_64_M_16__X__IMM_8,
	RM_32_64__X__IMM_8,
	RM_32__X__IMM_8,
	X__R_32_64_M_8__IMM_8,
	X__R_32_64_M_16__IMM_8,
	X__RM_32_64__IMM_8,
	X__IMM_8,
	R_32_64__X,
	RM_32_64__X,
	M_32_64__R_32_64,
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

	struct Oper *l;
	struct Oper *r;
	struct Oper *o;
};
// public
void get_ops_code(struct Ipcd *);
void get_align(struct Ipcd *, int, int, int);
#define is_rel(c) ((c) == __REL_8 || (c) == __REL_32)
// private
// * shared
extern const struct Cmnd cmnds0[];
extern const struct Cmnd cmnds1[];
extern const struct Cmnd cmnds2[];
extern const struct Cmnd cmnds3[];
extern const long cmnds0_len;
extern const long cmnds1_len;
extern const long cmnds2_len;
extern const long cmnds3_len;

const struct Cmnd *get_cmnd(struct Ipcd *, const struct Cmnd[], const long,
							enum OpsCode);
void add_sib(struct BList *, struct Oper *);
void add_disp(struct Ipcd *, struct Oper *, uc);
void add_imm_data(struct Ipcd *, struct Oper *);
void add_mem(struct Ipcd *, struct Oper *m);
// * ___
int is_in_opsc(enum OpsCode, const enum OpsCode[], size_t);
extern const enum OpsCode RM_L[];
extern const enum OpsCode RM_R[];
extern const enum OpsCode RM__R_ARR[];
extern const enum OpsCode R__RM_ARR[];
extern const enum OpsCode IMM_R[];
extern const enum OpsCode XM_L[];
extern const enum OpsCode XM_R[];
extern const enum OpsCode X__RM_ARR[];
extern const enum OpsCode RM__X_ARR[];
extern const enum OpsCode XRM__XR_ARR[];
extern const enum OpsCode XR__XRM_ARR[];
extern const long RM_L___LEN;
extern const long RM_R___LEN;
extern const long RM__R_ARR___LEN;
extern const long R__RM_ARR___LEN;
extern const long IMM_R___LEN;
extern const long XM_L___LEN;
extern const long XM_R___LEN;
extern const long X__RM_ARR___LEN;
extern const long RM__X_ARR___LEN;
extern const long XRM__XR_ARR___LEN;
extern const long XR__XRM_ARR___LEN;
#define is_rm_l(c) (is_in_opsc((c), RM_L, RM_L___LEN))
#define is_rm_r(c) (is_in_opsc((c), RM_R, RM_R___LEN))
#define is_rm__r(c) (is_in_opsc((c), RM__R_ARR, RM__R_ARR___LEN))
#define is_r__rm(c) (is_in_opsc((c), R__RM_ARR, R__RM_ARR___LEN))
#define is_xm_l(c) (is_in_opsc((c), XM_L, XM_L___LEN))
#define is_xm_r(c) (is_in_opsc((c), XM_R, XM_R___LEN))
#define is_xrm__xr(c) (is_in_opsc((c), XRM__XR_ARR, XRM__XR_ARR___LEN))
#define is_xr__xrm(c) (is_in_opsc((c), XR__XRM_ARR, XR__XRM_ARR___LEN))
#define is_x__rm(c) (is_in_opsc((c), X__RM_ARR, X__RM_ARR___LEN))
#define is_rm__x(c) (is_in_opsc((c), RM__X_ARR, RM__X_ARR___LEN))
#define is_imm_r(c) (is_in_opsc((c), IMM_R, IMM_R___LEN))
// * change, warn
void change_mem_size(struct Inst *in, struct Oper *o, uc sz);
void change_imm_size(struct Inst *in, struct Oper *o, uc sz);
#define change_size_lr(in, l, r) (change_imm_size((in), (r), (l)->sz))
void change_m_sz(struct Inst *in, struct Oper *r, struct Oper *rm);
void warn_change_to_eq_size_lr(struct Inst *i, struct Oper *l, struct Oper *r);
int warn_change_size_lr(struct Inst *in, struct Oper *l, struct Oper *r);
int try_change_imm_to_byte(struct Inst *in, struct Oper *o);
// * 0
void get_zero_ops_code(struct Ipcd *);
// * 1
void get_one_ops_code(struct Ipcd *);
enum OpsCode get_one_opscode(struct Ipcd *);
void get_one_ops_prefs(struct Ipcd *, enum OpsCode);
void fill_one_ops_cmd_and_data(struct Ipcd *);
// * 2
void get_two_ops_code(struct Ipcd *);
enum OpsCode get_two_opscode(struct Ipcd *);
void get_two_ops_prefs(struct Ipcd *, enum OpsCode);
void fill_two_ops_cmd_and_data(struct Ipcd *);
// * 3
void get_tri_ops_code(struct Ipcd *);
enum OpsCode get_tri_opscode(struct Ipcd *);
void get_tri_ops_prefs(struct Ipcd *, enum OpsCode);
void fill_tri_ops_cmd_and_data(struct Ipcd *);
