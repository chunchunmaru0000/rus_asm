#include "tzer.h"

uc sc(char *, const char *);

struct Pser {
	struct PList *ts; // tokens
	long pos;
};
struct Pser *new_pser(char *);
struct PList *pse(struct Pser *); // instructions

enum ICode {
	IEOI, // end of instructions
	IENTRY,
	ISEGMENT,
	ISECTION,
	ILABEL,
	ILET,
	//IMOV,
	IMOV_EAX_INT,
	IMOV_EDX_INT,
	IMOV_EDI_INT,
	IMOV_ESI_INT,
	IMOV_ESI_LABEL,
	IJMP,
	IADD,
	ISUB,
	IIMUL,
	ISYSCALL,
	ICALL,
	IRET,
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
