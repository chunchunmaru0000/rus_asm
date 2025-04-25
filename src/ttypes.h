enum TCode {
	// WORD
	ID, // identifier
	// OTHER
	SLASHN, // \n
	SEMI,	// ;
	COLO,	// :
	COM,	// comment
	SLASH,	// \_
	COMMA,	// ,
	EOF,
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
	IMUL,	// *
	DIV,	// /
	EQUE,	// ==
	PLUSE,	// +=
	MINUSE, // -=
	IMULE,	// *=
	DIVE,	// /=
	SHL,	// //
	SHR,	// \\_
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
