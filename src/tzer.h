#include "ttypes.h"
#include "lists.h"

#define loop while(1)
#define lenofarr(arr) (sizeof((arr)) / sizeof((arr)[0]))

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
struct PList *tze(struct Tzer *, long);
