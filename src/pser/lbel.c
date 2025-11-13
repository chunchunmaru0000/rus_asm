#include "lbel.h"
#include <stdlib.h>

struct Usage *new_usage(uint64_t place, uint64_t cmd_end, enum UT type) {
	struct Usage *u = malloc(sizeof(struct Usage));
	u->place = place;
	u->cmd_end = cmd_end;
	u->type = type;
	u->hc = 0;
	return u;
}

struct Defn *new_not_plov(char *view, uint64_t place, uint64_t cmd_end,
						  enum UT ut) {
	struct Defn *np = malloc(sizeof(struct Defn));
	np->view = view;
	np->value = new_usage(place, cmd_end, ut);
	return np;
}
