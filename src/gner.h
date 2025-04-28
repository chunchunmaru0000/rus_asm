#include "pser.h"
#include <stdint.h>

struct Gner {
	enum Target t;
	struct PList *is;
	long pos;
	struct BList *prol;
	struct BList *text;

	uint64_t entry;
	struct PList *phs; // program headers
	struct PList *shs; // section headers
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
	uc entry[8];
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
	uint64_t vaddr;  // addr
	uint64_t paddr;  // addr
	uint64_t filesz; // size
	uint64_t memsz;  // size
	uint64_t align;
};

struct ELFSH { // ELF Segment Header
};

struct Gner *new_gner(struct PList *, enum Target t);
void gen(struct Gner *);
