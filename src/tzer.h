#include "ttypes.h"

struct Tzer {
	char *filename;
	long line;
	long col;

	long pos;
	char *code;
};

struct Tzer *new_tzer(char *);
struct Token *new_token(struct Tzer *);
struct List *get_tokens(struct Tzer *);
