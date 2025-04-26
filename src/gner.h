#include "pser.h"

struct Gner {
	struct PList *is;
	long pos;
	struct BList *prol;
	struct BList *text;
	//	struct BList *data;
	//	struct BList *bss;
};

struct Gner *new_gner(struct PList *);
void gen(struct Gner *, enum Target);
