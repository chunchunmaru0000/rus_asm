enum Target {
	Linux_ELF_86_64,
	Linux_OBJ_86_64,
	Windows_EXE_86_64,
};

enum TCode {
	// WORD
	ID, // identifier
	// OTHER
	SLASHN, // \n
	SEMI,	// ;
	COLO,	// :
	COM,	// comment
	SLASH,	// \ slash
	COMMA,	// ,
	SEP,	// // separator
	PAR_L,	// (
	PAR_R,	// )
	EF, // end file
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
	long line;
	long col;
	// literals reserved than may not always be used
	long number;
	double fpn; // Floatimg Point Number
	char *string;
	long string_len;
};
