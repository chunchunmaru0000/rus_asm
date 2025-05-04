#include "tzer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *EMPTY_STR = "_";
char *EOF_STR = "_КОНЕЦ_ФАЙЛА_"; // конец файла

void ee(struct Tzer *t, char *msg) { // error exit
	fprintf(stderr, "%s:%ld:%ld %s\n", t->filename, t->line, t->col, msg);
	exit(1);
}

struct Tzer *new_tzer(char *filename) {
	struct Tzer *t = (struct Tzer *)malloc(sizeof(struct Tzer));
	t->filename = filename;
	t->line = 1;
	t->col = 1;
	t->pos = 0;

	FILE *file = fopen(filename, "r");
	if (!file)
		ee(t, "ОШИБКА В ОТКРЫТИИ ФАЙЛА");

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	char *text = (char *)malloc(size + 1);
	fread(text, 1, size, file);
	text[size] = '\0';
	fclose(file);

	t->code = text;
	t->codeLen = size - 1;
	return t;
}

// #define next(t) (((t)->pos++, (t)->col++))
#define cur(t) ((t)->pos < (t)->codeLen ? (t)->code[(t)->pos] : '\0')
#define get(t, offset)                                                         \
	((t)->pos + (offset) < (t)->codeLen ? (t)->code[(t)->pos + (offset)] : '\0')
char next(struct Tzer *t) {
	t->pos++;
	t->col++;
	return cur(t);
}

enum TCode next_line(struct Tzer *t, struct Token *token) {
	next(t);
	t->col = 1;
	t->line++;
	token->view = EMPTY_STR;
	return SLASHN;
}
uint64_t parse_decimal_num(char *);
uint64_t parse_binary_num(char *);
uint64_t parse_hex_num(char *);

long s_cyrilic_toi(struct Tzer *t, char *s) {
	int base = 10, minus_flag = 1;
	int64_t value = 0;
	uc c, n;

	if (s[0] == '-') {
		minus_flag = -1;
		s++;
	}
	while (s[0] == '0')
		s++;

	c = s[0], n = s[1];

	if (c == 'x' || c == 'X') {
		base = 16;
		s++;
	} else if ((c == 0b11010001 && n == 0b10000101) ||
			   (c == 0b11010000 && n == 0b10100101)) {
		base = 16;
		s += 2;
	} else if (c == 'b' || c == 'B') {
		base = 2;
		s++;
	} else if (c == 0b11010000 && (n == 0b10110001 || n == 0b10010001)) {
		base = 2;
		s += 2;
	}

	if (base == 10)
		value = parse_decimal_num(s);
	else if (base == 2)
		value = parse_binary_num(s);
	else if (base == 16)
		value = parse_hex_num(s);
	else
		ee(t, "Неверное основание числа");

	return value * minus_flag;
}

uint64_t parse_decimal_num(char *s) {
	uint64_t value = 0;
	while (*s != 0) {
		if (*s != '_') {
			value *= 10;
			value += *s - '0';
		}
		s++;
	}
	return value;
}
uint64_t parse_binary_num(char *s) {
	uint64_t value = 0;
	while (*s != 0) {
		if (*s != '_') {
			value <<= 1;
			value += *s == '1';
		}
		s++;
	}
	return value;
}
uint64_t parse_hex_num(char *s) {
	uint64_t value = 0;
	while (*s != 0) {
		if (*s != '_') {
			value <<= 4;
			if (*s >= '0' && *s <= '9')
				value += *s - '0';
			else if (*s >= 'A' && *s <= 'F')
				value += *s - 'A' + 10;
			else if (*s >= 'a' && *s <= 'f')
				value += *s - 'a' + 10;
			else if ((uc)*s == 0b11010000) {
				s++;
				if ((uc)*s >= 0b10110000 && (uc)*s <= 0b10110110)
					value += (uc)*s - (0b10110000 - 10) + 10;
				else
					value += (uc)*s - (0b10010000 - 10) + 10;
			}
		}
		s++;
	}
	return value;
}

enum TCode num_token(struct Tzer *t, struct Token *token) {
	uc c = cur(t), n = get(t, 1), nn = get(t, 2), fpn = 0;
	long start_pos = t->pos, num_len;
	enum TCode code = INT;
	char *num_view;

	int base = 10, minus_flag = 1;
	uint64_t value = 0;

	if (c == '-') {
		minus_flag = -1;
	}
	while (c == '0')
		c = next(t);
	n = next(t);

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
		while (c >= '0' && c <= '9') {
			c = next(t);
			if (c == '.') {
				fpn++;
				if (fpn > 1)
					ee(t, "Слишком много точек на одно не целое число");
				code = REAL;
				c = next(t);
			}
		}
	} else if (base == 2) {

	} else { // base = 16
	}

/*
	if (c == '0' && (n == 'x' || n == 'X' || n == 'b' || n == 'B' ||
					 (n == 0b11010001 && nn == 0b10000101) ||  // х cyrillic
					 (n == 0b11010000 && nn == 0b10100101) ||  // Х cyrillic
					 (n == 0b11010000 && nn == 0b10110001) ||  // б
					 (n == 0b11010000 && nn == 0b10010001))) { // Б
		if (cur(t) == '-')
			minus_flag = -1;
		c = next(t); // skip 0
		n = get(t, 1);
		base = c == 'x' || c == 'X' || (c == 0b11010001 && n == 0b10000101) ||
					   (c == 0b11010000 && n == 0b10100101)
				   ? 16
				   : 2;
		c = next(t);
		if ((c == 'x' || c == 'X' || c == 'b' || c == 'B') == 0)
			c = next(t);
		// абвгдж
		// abcdef
		// абстеф
		if (base == 16) {
			loop {
				while (c == '_')
					c = next(t);
				n = get(t, 1);
				if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
					(c >= 'a' && c <= 'f'))
					c = next(t);
				else if ((uc)c == 0b11010000 &&
						 (((uc)n >= 0b10110000 && (uc)n <= 0b10110110) ||
						  ((uc)n >= 0b10010000 && (uc)n <= 0b10010110))) {
					next(t);
					c = next(t);
				} else
					break;
			}
		} else {
			c = next(t);
			while (c == '0' || c == '1' || c == '_')
				c = next(t);
		}
	} else {
		loop {
			c = next(t);
			if (c == '.') {
				fpn++;
				if (fpn > 1)
					ee(t, "Слишком много точек на одно не целое число");
				code = REAL;
				c = next(t);
			}
			if (c < '0' || c > '9')
				break;
		}
	}
*/
	num_len = t->pos - start_pos;
	num_view = malloc(num_len + 1);
	num_view[num_len] = 0;
	strncpy(num_view, &t->code[start_pos], num_len);
	printf("\t\tnum_view: %s\n", num_view);

	if (code == INT)
		token->number = s_cyrilic_toi(t, num_view);
	else
		token->fpn = atof(num_view);

	token->view = num_view;
	return code;
}

enum TCode str_token(struct Tzer *t, struct Token *token) {
	long start_pos = t->pos, str_len = 2;
	char *str_view;
	next(t);
	// TODO: maybe do like \т\н\0\t\n\0
	while (cur(t) != '"') {
		if (cur(t) == '\n') {
			t->col = 1;
			t->line++;
		}
		next(t);
		str_len++;
	}
	next(t);

	str_view = malloc(str_len + 1);
	str_view[str_len] = 0;
	strncpy(str_view, &t->code[start_pos], str_len);

	token->view = str_view;
	token->string = str_view + 1;
	token->string_len = str_len - 2;
	return STR;
}

char *alloc_str_and_set_code(char str[], long len, enum TCode *codeptr,
							 enum TCode code) {
	char *s = malloc(len + 1);
	memcpy(s, str, len + 1);
	*codeptr = code;
	return s;
}
char *next_and_alloc(struct Tzer *t, char str[], long len, enum TCode *codeptr,
					 enum TCode code) {
	next(t);
	return alloc_str_and_set_code(str, len, codeptr, code);
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
		view = alloc_str_and_set_code(":", 1, cp, COLO);
		break;
	case '\\':
		if (n == '\\')
			view = next_and_alloc(t, "\\\\", 2, cp, SHR);
		else
			view = alloc_str_and_set_code("\\", 1, cp, SLASH);
		break;
	case '+':
		if (n == '+')
			view = next_and_alloc(t, "++", 2, cp, INC);
		else if (n == '=')
			view = next_and_alloc(t, "+=", 2, cp, PLUSE);
		else
			view = alloc_str_and_set_code("+", 1, cp, PLUS);
		break;
	case '-':
		if (n == '-')
			view = next_and_alloc(t, "--", 2, cp, DEC);
		else if (n == '=')
			view = next_and_alloc(t, "-=", 2, cp, MINUSE);
		else
			view = alloc_str_and_set_code("-", 1, cp, MINUS);
		break;
	case '*':
		if (n == '=')
			view = next_and_alloc(t, "*=", 2, cp, IMULE);
		else
			view = alloc_str_and_set_code("*", 1, cp, IMUL);
		break;
	case '/':
		if (n == '=')
			view = next_and_alloc(t, "/=", 2, cp, DIVE);
		else if (n == '/')
			view = next_and_alloc(t, "//", 2, cp, SHL);
		else
			view = alloc_str_and_set_code("/", 1, cp, DIV);
		break;
	case '=':
		if (n == '=')
			view = next_and_alloc(t, "==", 2, cp, EQUE);
		else
			view = alloc_str_and_set_code("=", 1, cp, EQU);
		break;
	case ',':
		view = alloc_str_and_set_code(",", 1, cp, COMMA);
		break;
	default:
		ee(t, "НЕ ДОЛЖНО БЫТЬ ДОСТИЖИМО");
	}

	token->view = view;
	return code;
}

enum TCode com_token(struct Tzer *t, struct Token *token) {
	long start_pos = t->pos, com_len = 1;
	next(t);
	char *com_view;
	while (cur(t) != '\n' && cur(t) != '\0') {
		next(t);
		com_len++;
	}

	com_view = malloc(com_len + 1);
	com_view[com_len] = 0;
	strncpy(com_view, &t->code[start_pos], com_len);

	token->view = com_view;
	return COM;
}

char *stop_id = " \r\t\n\"\\;:/+-*=,";
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
	strncpy(id_view, &t->code[start_pos], id_len);

	token->view = id_view;
	return ID;
}

char *usable_chars = ";:\\/+-*=,";
char *white_space = " \r\t";
struct Token *new_token(struct Tzer *t) {
	while (char_in_str(cur(t), white_space))
		next(t);

	uc c = cur(t), n = get(t, 1);
	enum TCode code;
	struct Token *token = malloc(sizeof(struct Token));
	token->line = t->line;
	token->col = t->col;

	// every of funcs that takes token shall assign view to token
	if (c == '\0')
		code = EF;
	else if (c == ';')
		code = com_token(t, token);
	else if (c == '\n')
		code = next_line(t, token);
	else if ((c >= '0' && c <= '9') ||
			 ((c == '-' || c == '+') && (n >= '0' && n <= '9')))
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
