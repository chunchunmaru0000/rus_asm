struct List {
	long e_size; // one element size
	void *start;
	long cap;
};

struct List *new_list(long, long);
long list_inc(struct List *, long); // realloc and INCrease size
#define list_get(l, t, i) (( (t*)l->start )[i])
#define list_set(l, t, i, v) (( (t*)l->start )[i] = v)
#define list_add(l, t, v) do { \
		list_inc(l, 1); \
		list_set(l, t, l->cap - 1, v); \
	} while(0)
