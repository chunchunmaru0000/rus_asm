#include "pser.h"

enum UT { // Usage Type
	ADDR,
	REL_ADDR, // 32
	REL_ADDR_8,
};

struct Usage {
	uint64_t place;
	enum UT type;
	uint64_t cmd_end;
	uint64_t hc; // header counter, supposedly ELFPH
	uint64_t ic; // instruction counter
};
struct Usage *new_usage(uint64_t, enum UT);
