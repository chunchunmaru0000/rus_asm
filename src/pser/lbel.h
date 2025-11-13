#include <stdint.h>

enum UT { // Usage Type
	ADDR,
	REL_ADDR, // 32
	REL_ADDR_8,
	HERE_ADDR,
	LET_HERE_ADDR,
	TUT_ADDR,
	LET_TUT_ADDR,
	BIN_OP_ADDR,
};

struct Defn {
	char *view;
	void *value;
};

struct Usage {
	uint64_t place;
	enum UT type;
	uint64_t cmd_end;
	uint64_t hc;   // header counter, supposedly ELFPH
	uint64_t ipos; // instruction counter
};
struct Usage *new_usage(uint64_t place, uint64_t cmd_end, enum UT type);
struct Defn *new_not_plov(char *view, uint64_t place, uint64_t cmd_end,
						  enum UT ut);
