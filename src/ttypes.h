#include <stdlib.h>

enum TCode {
	// WORD
	ID, // identifier
	// OTHER
	SLASHN, // \n
	SEMI,	// ;
	COMM,	// comment
	EOF,
	// LITERALS
	INT,  // int literal
	REAL, // float literal
	STR,  // string literal with ""
	// MATH OPS
	PLUS,	// +
	INC,	// ++
	MINUS,	// -
	DEC,	// --
	MUL,	// *
	DIV,	// /
	SHL,	// //
	SHR,	// \\
	PLUSE,	// +=
	MINUSE, // -=
	IMULE,	// *=
	DIVE,	// /=
};

struct Token {
	char *view;
	enum TCode code;
	long line;
	long col;
	// literals reserved than may not always be used
	long number;
	double fpn; // Floatimg Point Number
	char *string;
	long stringLen;
};
