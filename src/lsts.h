#include <stdint.h>
#include <stdlib.h>

#define uc unsigned char
#define BYTE 1
#define WORD 2
#define DWORD 4
#define QWORD 8
#define XWORD 16 // эбайт
#define YWORD 32 // юбайт
#define ZWORD 64 // ябайт

struct PList {
	void **st; // start
	uint32_t cap_pace;
	uint32_t cap;
	uint32_t size;
};

struct PList *new_plist(uint32_t); // cap pace
uint32_t plist_add(struct PList *, void *);
void *plist_get(struct PList *, uint32_t);
void *plist_set(struct PList *, uint32_t, void *);
void plist_free(struct PList *);
#define plist_clear(l) ((l)->size = 0)
void plist_re(struct PList *l);
void plist_clear_items_free(struct PList *);
void plist_free_items_free(struct PList *);
#define p_last(l) (plist_get((l), (l)->size - 1))

struct BList {
	uc *st; // start
	uint32_t cap_pace;
	uint32_t cap;
	uint32_t size;
};

struct BList *new_blist(uint32_t cap_pace);
struct BList *blist_from_str(char *str, uint32_t str_len);
void convert_blist_to_blist_from_str(struct BList *l);
uint32_t blist_add(struct BList *, uc);
uint32_t blist_cut(struct BList *);
uc blist_get(struct BList *, uint32_t);
uc blist_set(struct BList *, uint32_t, uc);
void blat(struct BList *, uc *, uint32_t);
#define blat_blist(l, o) (blat((l), (o)->st, (o)->size))
#define blist_clear(l) ((l)->size = 0)
void blist_clear_free(struct BList *);
void blist_print(struct BList *);
void blist_add_set(struct BList *, uc, long *, size_t);
struct BList *copy_str(struct BList *src);
struct BList *copy_blist(struct BList *l);
#define b_last(l) (blist_get((l), (l)->size - 1))

#define loop while (1)
#define loa(arr) (sizeof((arr)) / sizeof((arr)[0]))
// String Compare
#define sc(str1, str2) (strcmp((str1), (str2)) == 0)
// View Compare
#define vc(t1, t2) (sc((char *)(t1)->view->st, (char *)(t2)->view->st))
// View Compare String
#define vcs(t, str) (sc((char *)(t)->view->st, (str)))
// View String
#define vs(t) ((char *)(t)->view->st)
// String String
#define ss(t) ((char *)(t)->str->st)
