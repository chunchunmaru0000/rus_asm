#include "lbel.h"
#include <stdlib.h>

struct Usage *new_usage(uint64_t place, enum UT type) {
	struct Usage *u = malloc(sizeof(struct Usage));
	u->place = place;
	u->type = type;
	u->cmd_end = 0;
	u->hc = 0;
	return u;
}

struct Defn *new_not_plov(char *view, uint64_t place, enum UT ut) {
	struct Defn *np = malloc(sizeof(struct Defn));
	np->view = view;
	np->value = new_usage(place, ut);
	return np;
}
