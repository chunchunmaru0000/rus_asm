#include "../some/tpes.h"
#include <string.h>

#define loop while (1)
#define lenofarr(arr) (sizeof((arr)) / sizeof((arr)[0]))

extern const char *const COLOR_BLACK;
extern const char *const COLOR_RED;
extern const char *const COLOR_LIGHT_RED;
extern const char *const COLOR_GREEN;
extern const char *const COLOR_YELLOW;
extern const char *const COLOR_BLUE;
extern const char *const COLOR_PURPLE;
extern const char *const COLOR_LIGHT_PURPLE;
extern const char *const COLOR_GAY;
extern const char *const COLOR_WHITE;
extern const char *const COLOR_RESET;
#define color_print(c, msg) (printf("%s%s%s", (c), (msg), COLOR_RESET))

struct Tzer {
	struct Fpfc *f;
	struct Pos *p;
	size_t pos;
};

struct Tzer *new_tzer(char *);
struct Token *new_token(struct Tzer *);
struct PList *tze(struct Tzer *, long);
void print_source_line(const char *, uint32_t, const char *const);
void ee(struct Fpfc *, struct Pos *, const char *const);
