#include "lbel.h"

struct Usage *new_usage(uint64_t place, enum UT type) {
	struct Usage *u = malloc(sizeof(struct Usage));
	u->place = place;
	u->type = type;
	u->cmd_end = 0;
	//u->
	return u;
}
