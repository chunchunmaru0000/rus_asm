#include "pser.h"

enum UT { // Usage Type
	ADDR,
	REL_ADDR,
};

struct Usage {
	uint64_t place;
	enum UT type;
	uint64_t cmd_end;
};
struct Usage *new_usage(uint64_t, enum UT);
