#include "lsts.h"
#include <stdint.h>

enum Target {
	Linux_ELF_86_64,
	Linux_OBJ_86_64,
	Windows_EXE_86_64,
};

struct Fpfc { // File Path File Code
	const char *path;
	const char *code;
	size_t clen; // code len
};

struct Pos {
	uint32_t line;
	uint32_t col;
};

enum TCode {
	// WORD
	ID, // identifier
	// OTHER
	SLASHN, // \n
	COLO,	// :
	COM,	// comment
	SLASH,	// \ slash
	COMMA,	// ,
	SEP,	// // separator
	PAR_L,	// (
	PAR_R,	// )
	EF,		// end file
	// LITERALS
	INT,  // int literal
	REAL, // float literal
	STR,  // string literal with ""
	// MATH OPS
	INC,	// ++
	DEC,	// --
	EQU,	// =
	PLUS,	// +
	MINUS,	// -
	MUL,	// *
	DIV,	// /
	EQUE,	// ==
	PLUSE,	// +=
	MINUSE, // -=
	MULE,	// *=
	DIVE,	// /=
	SHLE,	// //=
	SHRE,	// \\=
};

struct Token {
	char *view;
	enum TCode code;
	struct Pos *p;
	// literals reserved than may not always be used
	long num;
	double real;
	struct BList *str;
};
