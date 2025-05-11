#include "pser.h"
#include <stdint.h>

// for label and maybe variables that have a string view and ptr to value
struct Plov {		  // Pointer Label Of Value
	char *l;		  // label
	uint64_t a;		  // adress
	uint64_t ra;	  // relative adress in executable file from file start
	int si;			  //  segment place
	struct PList *us; // usages
					  // uint64_t size; // like for db dd dw dq
};

enum OpsCode {
	OPC_INVALID,
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
};

enum UT { // Usage Type
	ADDR,
	REL_ADDR,
};

struct Usage {
	uint64_t place;
	enum UT type;
};

struct ELFH { // ELF Header
	uc mag[4];
	uc elf_class;
	uc data;
	uc elf_version;
	uc osabi;
	uc indent[8];
	uc type[2];
	uc machine[2];
	uc format_version[4];
	uint64_t entry;
	uc phoff[8];
	uc shoff[8];
	uc flags[4];
	uc ehsize[2];
	uc phsize[2];
	uc phnum[2];
	uc shsize[2]; // and what these
	uc shnum[2];
	uc shstrndx[2]; // what this
};

struct ELFPH { // ELF Program Header
	int type;
	int flags; // bits
	uint64_t offset;
	uint64_t vaddr;	 // addr
	uint64_t paddr;	 // addr
	uint64_t filesz; // size
	uint64_t memsz;	 // size
	uint64_t align;
};

struct ELFSH { // ELF Segment Header
};

struct Gner {
	enum Target t;
	uc debug;
	struct PList *is;
	long pos;
	struct BList *prol;
	struct BList *text;

	uint64_t pie;
	struct ELFH *elfh;
	struct PList *lps; // labels plovs
	struct PList *phs; // program headers
	struct PList *shs; // section headers
};

struct Gner *new_gner(struct PList *, enum Target, uc);
void gen(struct Gner *);

#define NOT_FIELD 0
#define REG_FIELD 1
#define NUM_FIELD 2
struct Cmnd {
	enum ICode inc;
	// instruction code
	uc cmd[4];
	// command bytes
	uc len;
	// cmd len
	uc o;
	// o Register/ Opcode Field
	//   1. NUM_FIELD The value of the opcode extension values from 0 through 7
	//   2. REG_FIELD r indicates that the ModR/M byte contains a register
	//   operand and an r/m operand. 00 ADD
	uc o_num;
	// instruction number if o == NUM_FIELD
	enum OpsCode opsc;
	// instruction opperands
};
