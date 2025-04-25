#include "ttypes.h"
#include "plist.h"

struct Tzer {
	char *filename;
	long line;
	long col;

	long pos;
	char *code;
	long codeLen;
};

struct Tzer *new_tzer(char *);
struct Token *new_token(struct Tzer *);
struct PList *get_tokens(struct Tzer *, long);
