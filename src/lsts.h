#include <stdlib.h>

#define uc unsigned char
#define BYTE 1
#define WORD 2
#define DWORD 4
#define QWORD 8

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
void plist_clear(struct PList *);
void plist_clear_items_free(struct PList *);

struct BList {
	uc *st; // start
	long cap_pace;
	long cap;
	long size;
};

struct BList *new_blist(long); // cap pace
long blist_add(struct BList *, uc);
long blist_cut(struct BList *);
uc blist_get(struct BList *, long);
uc blist_set(struct BList *, long, uc);
void blat(struct BList *, uc *, long);
void blist_clear(struct BList *);
void blist_print(struct BList *);
void blist_add_set(struct BList *, uc, long *, size_t);
