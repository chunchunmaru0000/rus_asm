#include <stdint.h>

enum UT { // Usage Type
	ADDR,
	REL_ADDR, // 32
	REL_ADDR_8,
	HERE_ADDR,
};

struct Defn {
	char *view;
	void *value;
};

struct Usage {
	uint64_t place;
	enum UT type;
	uint64_t cmd_end;
	uint64_t hc; // header counter, supposedly ELFPH
	uint64_t ic; // instruction counter
};
struct Usage *new_usage(uint64_t, enum UT);
struct Defn *new_not_plov(char *view, uint64_t place, enum UT ut);
