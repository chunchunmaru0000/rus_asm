#include "cmnd.h"
#include <stdint.h>

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

#define SHORT_JMP_CMND_SZ 2 // byte for op code and byte for rel 8
#define LONG_JMP_CMND_SZ 5	// (byte E9 for JMP rel16/32) + 4 bytes for rel 32
#define LONG_NOT_JMP_CMND_SZ 6 // 2 bytes for cmd + 4 bytes for rel 32
#define SHORTENED 1
#define UNSHORTABLE 0
#define SHORTABLE -1
#define inst_size(i) ((i)->cmd->size + (i)->data->size)
extern uc OPT_FLAG;

struct Jump {
	//	char *label;
	uint32_t lipos; // label ipos
	int addr;		// addres from beginning of text to place before jmp
	uint32_t ipos;	// instruction pos
	enum ICode code;
	uc size;
};

struct Ephs {
	int phs_c;
	uint32_t phs_cur_sz;
	uint32_t all_h_sz;
	struct PList *phs; // program headers
};

struct Gner {
	enum Target t;
	uc debug;
	struct PList *is;
	uint32_t pos;
	uint32_t compiled;
	struct BList *prol;
	struct BList *text;

	uint64_t pie;
	struct ELFH *elfh;
	struct PList *lps; // labels plovs
	struct Ephs *eps;
	// struct PList *phs;	// program headers
	struct PList *jmps; // jmps
	struct PList *shs;	// section headers
};

struct Gner *new_gner(struct PList *, enum Target, uc);
void gen(struct Gner *);
