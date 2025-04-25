struct PList {
	void **st; // start
	long cap_pace;
	long cap;
	long size;
};

struct PList *new_list(long); // cap pace
long list_add(struct PList *, void *);
void *list_get(struct PList *, long);
void *list_set(struct PList *, long, void *);
