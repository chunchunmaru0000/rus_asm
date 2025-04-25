#include <stdlib.h>

enum TCode {
	// WORD
	ID,		// identifier
	// OTHER
	SLASHN, // \n
	SEMI,	// ;
	// LITERALS
	INT,  	// int literal
	REAL, 	// float literal
	STR,  	// string literal with ""
	// MATH OPS
	PLUS,	// +
	MINUS,	//	-
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
	char *string;
	long stringLen;
};
