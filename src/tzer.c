#include "tzer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const COLOR_BLACK = "\x1B[30m";
const char *const COLOR_LIGHT_BLACK = "\033[90m";
const char *const COLOR_LIGHT_RED = "\x1B[91m";
const char *const COLOR_RED = "\x1B[31m";
const char *const COLOR_GREEN = "\x1B[32m";
const char *const COLOR_YELLOW = "\x1B[33m";
const char *const COLOR_BLUE = "\x1B[34m";
const char *const COLOR_PURPLE = "\x1B[35m";
const char *const COLOR_LIGHT_PURPLE = "\033[95m";
const char *const COLOR_GAY = "\x1B[36m";
const char *const COLOR_WHITE = "\x1B[37m";
const char *const COLOR_RESET = "\x1B[0m";
const char *const TEXT_TAB = "    ";

char *EMPTY_STR = "_";
char *EOF_STR = "_КОНЕЦ_ФАЙЛА_"; // конец файла

const char *write_ln(const char *line) {
	while (*line && *line != '\n') {
		if (*line == '\t')
			printf("%s", TEXT_TAB);
		else
			putchar(*line);
		line++;
	}
	if (*line == '\n')
		line++;
	return line;
}

void print_source_line(const char *source_code, uint32_t line,
					   const char *const color) {
	line--;
	const char *str_start = source_code;
	size_t nc = line;
	if (line)
		nc--;
	while (nc) {
		if (*str_start == '\n')
			nc--;
		str_start++;
	}

	printf("%s%5d |", COLOR_RESET, line);
	if (line)
		str_start = write_ln(str_start);
	putchar('\n');

	printf("%5d |%s", line + 1, color);
	str_start = write_ln(str_start);
	printf("%s\n", COLOR_RESET);

	printf("%5d |", line + 2);
	if (*str_start)
		write_ln(str_start);
	putchar('\n');
}

void ee(struct Fpfc *f, struct Pos *p, const char *const msg) { // error exit
	fprintf(stderr, "%s%s:%d:%d %sОШИБКА: %s\n", COLOR_WHITE, f->path, p->line,
			p->col, COLOR_RED, msg);
	print_source_line(f->code, p->line, COLOR_LIGHT_RED);
	exit(1);
}

struct Tzer *new_tzer(char *filename) {
	struct Tzer *t = malloc(sizeof(struct Tzer));
	struct Fpfc *f = malloc(sizeof(struct Fpfc));
	struct Pos *p = malloc(sizeof(struct Pos));
	t->f = f;
	t->p = p;

	p->line = 1;
	p->col = 1;
	t->pos = 0;

	FILE *file = fopen(filename, "r");
	if (!file)
		ee(f, p, "ОШИБКА В ОТКРЫТИИ ФАЙЛА");

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	char *text = (char *)malloc(size + 1);
	fread(text, 1, size, file);
	text[size] = '\0';
	fclose(file);

	f->path = filename;
	f->code = text;
	f->clen = size - 1;
	return t;
}

// #define next(t) (((t)->pos++, (t)->col++))
#define cur(t) ((t)->pos < (t)->f->clen ? (t)->f->code[(t)->pos] : '\0')
#define get(t, offset)                                                         \
	((t)->pos + (offset) < (t)->f->clen ? (t)->f->code[(t)->pos + (offset)]    \
										: '\0')
char next(struct Tzer *t) {
	t->pos++;
	t->p->col++;
	return cur(t);
}

enum TCode next_line(struct Tzer *t, struct Token *token) {
	next(t);
	t->p->col = 1;
	t->p->line++;
	token->view = EMPTY_STR;
	return SLASHN;
}

const char *const TOO_MUCH_DOTS = "Слишком много точек на одно не целое число";

enum TCode num_token(struct Tzer *t, struct Token *token) {
	uc c = cur(t), n;
	long start_pos = t->pos, num_len;
	enum TCode code = INT;
	char *num_view;

	int base = 10, minus_flag = 1;
	uint64_t value = 0, mnt = 0;
	double mnt_len_10_pow = 1;

	if (c == '-') {
		minus_flag = -1;
		c = next(t);
	} else if (c == '+')
		c = next(t);
	while (c == '0')
		c = next(t);
	n = get(t, 1);

	if (c == 'x' || c == 'X') {
		base = 16;
		c = next(t);
	} else if ((c == 0b11010001 && n == 0b10000101) ||
			   (c == 0b11010000 && n == 0b10100101)) {
		base = 16;
		next(t);
		c = next(t);
	} else if (c == 'b' || c == 'B') {
		base = 2;
		c = next(t);
	} else if (c == 0b11010000 && (n == 0b10110001 || n == 0b10010001)) {
		base = 2;
		next(t);
		c = next(t);
	}

	if (base == 10) {
		while (c != '.' && c) {
			if (c == '_') {
				c = next(t);
				continue;
			}
			if (c < '0' || c > '9') {
				token->number = value * minus_flag;
				goto __parsed;
			}
			value *= 10;
			value += c - '0';
			c = next(t);
		}
		if (c == '.')
			c = next(t);
		else {
			token->number = value * minus_flag;
			goto __parsed;
		}
		code = REAL;
		while (c) {
			if (c == '_') {
				c = next(t);
				continue;
			}
			if (c < '0' || c > '9')
				break;
			mnt *= 10;
			mnt += c - '0';
			c = next(t);
			mnt_len_10_pow *= 10.0;
		}
		token->fpn = (value + mnt / mnt_len_10_pow) * minus_flag;
	} else if (base == 2) {
		while (c == '_' || c == '0' || c == '1') {
			if (c != '_') {
				value <<= 1;
				value += c == '1';
			}
			c = next(t);
		}
		token->number = value * minus_flag;
	} else { // base = 16
		// абвгде
		// abcdef
		// абстиф
		loop {
			if (c == '_') {
				c = next(t);
				continue;
			}
			if (c >= '0' && c <= '9')
				value = (value << 4) + c - '0';
			else if (c >= 'A' && c <= 'F')
				value = (value << 4) + c - 'A' + 10;
			else if (c >= 'a' && c <= 'f')
				value = (value << 4) + c - 'a' + 10;
			else if (c == 0xd0) {
				n = get(t, 1);
				if (n == 0x90 || n == 0xb0) // А а
					value = (value << 4) + 0xa;
				else if (n == 0x91 || n == 0xb1) // Б б
					value = (value << 4) + 0xb;
				else if (n == 0x92 || n == 0xb2) // В в
					value = (value << 4) + 0xc;
				else if (n == 0x93 || n == 0xb3) // Г г
					value = (value << 4) + 0xd;
				else if (n == 0x94 || n == 0xb4) // Д д
					value = (value << 4) + 0xe;
				else if (n == 0x95 || n == 0xb5) // Е е
					value = (value << 4) + 0xf;

				else if (n == 0xa1) // С
					value = (value << 4) + 0xc;
				else if (n == 0xa2) // Т
					value = (value << 4) + 0xd;
				else if (n == 0x98 || n == 0xb8) // И и
					value = (value << 4) + 0xe;
				else if (n == 0xa4) // Ф
					value = (value << 4) + 0xf;
				else
					break;
				next(t); // skip c
			} else if (c == 0xd1) {
				n = get(t, 1);
				if (n == 0x81) // с
					value = (value << 4) + 0xc;
				else if (n == 0x82) // т
					value = (value << 4) + 0xd;
				else if (n == 0x84) // ф
					value = (value << 4) + 0xf;
				else
					break;
				next(t); // skip c
			} else
				break;
			c = next(t); // skip n, get next
		}
		token->number = value * minus_flag;
	}
__parsed:
	num_len = t->pos - start_pos;
	num_view = malloc(num_len + 1);
	num_view[num_len] = 0;
	strncpy(num_view, &t->f->code[start_pos], num_len);
	// printf("\t\tnum_view: %s value: %lx\n", num_view, value);

	token->view = num_view;
	return code;
}

void start_line(struct Tzer *t) {
	t->p->col = 1;
	t->p->line++;
}

struct S2Uc {
	const uc len;
	const char *s;
	const uc c;
	uc x;
	uc xl;
};
#define SUC_NONE 255
#define SUC_X 254

#define s2ucs_SIZE 13
struct S2Uc s2ucs[] = {
	{2, "\\t", '\t'},  {2, "\\n", '\n'},  {2, "\\r", '\r'},	 {3, "\\т", '\t'},
	{3, "\\н", '\n'},  {3, "\\р", '\r'},  {2, "\\0", '\0'},	 {2, "\\\"", '"'},
	{2, "\\\\", '\\'}, {2, "\\x", SUC_X}, {2, "\\X", SUC_X}, {3, "\\х", SUC_X},
	{3, "\\Х", SUC_X} // {2, "\'", '\''},
};
struct S2Uc none_s2uc = {0, "0", SUC_NONE};

// абвгде
// абстиф
#define rus_hex_SIZE 42
struct S2Uc rus_hex[] = {
	{2, "а", 0xa}, {2, "А", 0xA}, {2, "б", 0xb}, {2, "Б", 0xB}, {2, "в", 0xc},
	{2, "В", 0xC}, {2, "г", 0xd}, {2, "Г", 0xD}, {2, "д", 0xe}, {2, "Д", 0xE},
	{2, "е", 0xf}, {2, "Е", 0xF}, {2, "с", 0xc}, {2, "С", 0xC}, {2, "т", 0xd},
	{2, "Т", 0xD}, {2, "и", 0xe}, {2, "И", 0xE}, {2, "ф", 0xf}, {2, "Ф", 0xF},
	{1, "a", 0xa}, {1, "A", 0xA}, {1, "b", 0xb}, {1, "B", 0xB}, {1, "c", 0xc},
	{1, "C", 0xC}, {1, "d", 0xd}, {1, "D", 0xD}, {1, "e", 0xe}, {1, "E", 0xE},
	{1, "f", 0xf}, {1, "F", 0xF}, {1, "0", 0x0}, {1, "1", 0x1}, {1, "2", 0x2},
	{1, "3", 0x3}, {1, "4", 0x4}, {1, "5", 0x5}, {1, "6", 0x6}, {1, "7", 0x7},
	{1, "8", 0x8}, {1, "9", 0x9},
};
char *const ERR_EXPECTED_NUM_STR =
	"Ожидалась последовательность из двух 16-ричных чисел после комбинации "
	"символов \\х__.";

struct S2Uc *search_pattern(struct Tzer *t, const char *text,
							struct S2Uc *pattern, long p_sz) {
	struct S2Uc *suc, *rhex;
	char *s = malloc(6); // max len + terminator

	for (long i = 0; i < p_sz; i++) {
		suc = pattern + i;
		memcpy(s, text, suc->len);
		s[suc->len] = 0;

		if (sc(s, suc->s)) {
			if (suc->c == SUC_X) {
				text += suc->len;

				rhex = search_pattern(t, text, rus_hex, rus_hex_SIZE);
				if (rhex->c == SUC_NONE)
					ee(t->f, t->p, ERR_EXPECTED_NUM_STR);
				text += rhex->len;
				suc->xl = rhex->len;
				suc->x = rhex->c << 4;

				rhex = search_pattern(t, text, rus_hex, rus_hex_SIZE);
				if (rhex->c == SUC_NONE)
					ee(t->f, t->p, ERR_EXPECTED_NUM_STR);
				suc->xl += rhex->len;
				suc->x += rhex->c;
			}
			goto search_pattern_ret;
		}
	}
	suc = &none_s2uc;
search_pattern_ret:
	free(s);
	return suc;
}

enum TCode str_token(struct Tzer *t, struct Token *token) {
	long start_pos = t->pos, str_len = 2, suc_full_len;
	struct BList *str_str = new_blist(16);
	struct S2Uc *suc;

	next(t); // skip "
	while (cur(t) != '"' && cur(t)) {
		if (cur(t) == '\n')
			start_line(t);
		if (cur(t) == '\\') {
			suc = search_pattern(t, t->f->code + t->pos, s2ucs, s2ucs_SIZE);
			if (suc->c != SUC_NONE) {
				if (suc->c == SUC_X) {
					blist_add(str_str, suc->x);
					suc_full_len = suc->len + suc->xl;
				} else {
					blist_add(str_str, suc->c);
					suc_full_len = suc->len;
				}
				t->pos += suc_full_len;
				t->p->col += suc_full_len;
				str_len += suc_full_len;
			} else {
				if (next(t) == '\n') // skip slash
					start_line(t);
				next(t); // skip char
				str_len += 2;
			}
			continue;
		}
		blist_add(str_str, cur(t));
		next(t);
		str_len++;
	}
	next(t); // skip "

	char *str_view = malloc(str_len + 1);
	str_view[str_len] = 0;
	strncpy(str_view, t->f->code + start_pos, str_len);

	token->view = str_view;
	token->str = str_str;
	blist_cut(str_str);
	return STR;
}

// next_and_alloc
char *naa(struct Tzer *t, char str[], long len, enum TCode *codeptr,
		  enum TCode code) {
	for (uint32_t i = 0; i < len - 1; i++)
		next(t);
	char *s = malloc(len + 1);
	memcpy(s, str, len + 1);
	*codeptr = code;
	return s;
}
unsigned char char_in_str(char c, char *str) {
	for (int i = 0; str[i]; i++)
		if (c == str[i])
			return 1;
	return 0;
}
enum TCode usable_token(struct Tzer *t, struct Token *token) {
	char *view;
	enum TCode code;
	enum TCode *cp = &code;
	uc c = cur(t), n = get(t, 1), nn = get(t, 2);
	next(t);

	switch (c) {
	case ':':
		view = naa(t, ":", 1, cp, COLO);
		break;
	case '\\':
		if (n == '\\') {
			if (nn == '=')
				view = naa(t, "\\\\=", 3, cp, SHRE);
			else
				view = naa(t, "\\", 1, cp, SLASH);
			// there is no use for \\ yet
		} else
			view = naa(t, "\\", 1, cp, SLASH);
		break;
	case '+':
		if (n == '+')
			view = naa(t, "++", 2, cp, INC);
		else if (n == '=')
			view = naa(t, "+=", 2, cp, PLUSE);
		else
			view = naa(t, "+", 1, cp, PLUS);
		break;
	case '-':
		if (n == '-')
			view = naa(t, "--", 2, cp, DEC);
		else if (n == '=')
			view = naa(t, "-=", 2, cp, MINUSE);
		else
			view = naa(t, "-", 1, cp, MINUS);
		break;
	case '*':
		if (n == '=')
			view = naa(t, "*=", 2, cp, MULE);
		else
			view = naa(t, "*", 1, cp, MUL);
		break;
	case '/':
		if (n == '=')
			view = naa(t, "/=", 2, cp, DIVE);
		else if (n == '/') {
			if (nn == '=')
				view = naa(t, "//=", 3, cp, SHLE);
			else
				view = naa(t, "//", 2, cp, SEP);
		} else
			view = naa(t, "/", 1, cp, DIV);
		break;
	case '=':
		if (n == '=')
			view = naa(t, "==", 2, cp, EQUE);
		else
			view = naa(t, "=", 1, cp, EQU);
		break;
	// case ',':
	//	view = naa(t, ",", 1, cp, COMMA);
	//	break;
	case '(':
		view = naa(t, "(", 1, cp, PAR_L);
		break;
	case ')':
		view = naa(t, ")", 1, cp, PAR_R);
		break;
	default:
		ee(t->f, t->p, "НЕ ДОЛЖНО БЫТЬ ДОСТИЖИМО");
	}

	token->view = view;
	return code;
}

enum TCode com_token(struct Tzer *t, struct Token *token, uc is_long) {
	long start_pos = t->pos, com_len = 1;
	next(t);
	char *com_view;

	if (is_long) {
		next(t);
		while ((cur(t) != ';' || get(t, 1) != ';') && cur(t) != '\0') {
			next(t);
			com_len++;
		}
		next(t);
		com_len++;
		next(t);
	} else
		while (cur(t) != '\n' && cur(t) != '\0') {
			next(t);
			com_len++;
		}

	com_view = malloc(com_len + 1);
	com_view[com_len] = 0;
	strncpy(com_view, t->f->code + start_pos, com_len);

	token->view = com_view;
	return COM;
}

char *stop_id = " \r\t\n\"\\;:/+-*=()";
enum TCode id_token(struct Tzer *t, struct Token *token) {
	long start_pos = t->pos, id_len = 1;
	next(t);
	char *id_view;
	while (!char_in_str(cur(t), stop_id) && cur(t) != '\0') {
		next(t);
		id_len++;
	}

	id_view = malloc(id_len + 1);
	id_view[id_len] = 0;
	strncpy(id_view, t->f->code + start_pos, id_len);

	token->view = id_view;
	return ID;
}

char *usable_chars = ";:\\/+-*=()";
char *white_space = " \r\t";
struct Token *new_token(struct Tzer *t) {
	while (char_in_str(cur(t), white_space))
		next(t);

	uc c = cur(t);
	enum TCode code;
	struct Token *token = malloc(sizeof(struct Token));
	struct Pos *p = malloc(sizeof(struct Pos));
	p->line = t->p->line;
	p->col = t->p->col;
	token->p = p;

	// every of funcs that takes token shall assign view to token
	if (c == '\0')
		code = EF;
	else if (c == ';') {
		if (get(t, 1) == ';')
			code = com_token(t, token, 1);
		else
			code = com_token(t, token, 0);
	} else if (c == '\n')
		code = next_line(t, token);
	else if ((c >= '0' && c <= '9') ||
			 ((c == '-' || c == '+') && (get(t, 1) >= '0' && get(t, 1) <= '9')))
		code = num_token(t, token);
	else if (c == '"')
		code = str_token(t, token);
	else if (char_in_str(c, usable_chars))
		code = usable_token(t, token);
	else
		code = id_token(t, token);

	token->code = code;
	return token;
}

struct PList *tze(struct Tzer *t, long list_cap) {
	struct PList *l = new_plist(list_cap);
	struct Token *token = new_token(t);

	while (token->code != EF) {
		if (token->code != COM)
			plist_add(l, token);
		token = new_token(t);
		// printf("%s:%ld:%ld:%s\n", t->filename, token->line, token->col,
		// token->view);
	}
	token->view = EOF_STR;
	plist_add(l, token);

	return l;
}
