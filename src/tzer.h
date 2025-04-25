#include "ttypes.h"

struct Tzer {
	char *filename;
	long line;
	long col;

	long pos;
	char *code;
};

struct Tzer *new_tzer(char *);
struct Token *new_token(struct Token *);
struct Token **get_tokens(struct Tzer *);
