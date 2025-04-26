struct PList {
	void **st; // start
	long cap_pace;
	long cap;
	long size;
};

struct PList *new_plist(long); // cap pace
long plist_add(struct PList *, void *);
void *plist_get(struct PList *, long);
void *plist_set(struct PList *, long, void *);
