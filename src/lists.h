#define uc unsigned char

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
void plist_free(struct PList *);

struct BList {
	uc *st; // start
	long cap_pace;
	long cap;
	long size;
};

struct BList *new_blist(long); // cap pace
long blist_add(struct BList *, uc);
uc blist_get(struct BList *, long);
uc blist_set(struct BList *, long, uc);
void blat(struct BList *, uc *, long);
void blist_clear(struct BList *);
