#include "gner.h"

enum CMD_FLAG {
	CF_DEBUG,
	CF_TARGET,
	CF_WARNS,
};

struct Flag {
	char *v;
	enum CMD_FLAG f;
};

struct Flags {
	char *filename;
	char *outname;
	enum Target tr;
	uc debug;
};

struct Flags *get_flags(int, char **);
