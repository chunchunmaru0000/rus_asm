#include "pser.h"

struct Gner {
	struct PList *is;
	long pos;
	struct BList *prol;
	struct BList *text;
	//	struct BList *data;
	//	struct BList *bss;
};

struct Header {
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

struct Gner *new_gner(struct PList *);
void gen(struct Gner *, enum Target);
