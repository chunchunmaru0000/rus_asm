#include "cmnd.h"

const enum OpsCode RM_L[] = {
	RM_8__R_8,			RM_16_32_64__R_16_32_64,
	RM_8__IMM_8,		RM_16_32_64__IMM_16_32,
	RM_16_32_64__IMM_8, M_32_64__R_32_64,
};
const enum OpsCode RM_R[] = {
	R_8__RM_8,
	R_16_32_64__RM_16_32_64,
	SREG__RM_16,
	R_16_32_64__RM_16_32_64__IMM_8,
	R_16_32_64__RM_16_32_64__IMM_16_32,
	R_64__RM_32,
	R_16_32_64__M,
};
// this doesnt have imm so its needed
const enum OpsCode RM__R_ARR[] = {
	RM_8__R_8,		  RM_16_32_64__R_16_32_64, M_16__SREG,
	R_16_32_64__SREG, M_32_64__R_32_64,
};
const enum OpsCode R__RM_ARR[] = {
	R_8__RM_8,	   R_16_32_64__RM_16_32_64,
	R_64__RM_32,   R_16_32_64__RM_16_32_64__IMM_16_32,
	SREG__RM_16,   R_16_32_64__RM_16_32_64__IMM_8,
	R_16_32_64__M,
};
const enum OpsCode IMM_R[] = {
	AL__IMM_8,	RM_16_32_64__IMM_16_32,	  RM_8__IMM_8,	  RM_16_32_64__IMM_8,
	R_8__IMM_8, R_16_32_64__IMM_16_32_64, RAX__IMM_16_32,
};

// xmm

const enum OpsCode XM_L[] = {
	XM_32__X,			XM_64__X,
	M_128__X,			M_64__X,
	XM_128__X,

	RM_32__X__IMM_8,	R_32_64_M_16__X__IMM_8,
	RM_32_64__X__IMM_8, R_32_64_M_8__X__IMM_8,
	RM_32_64__X,
};
const enum OpsCode XM_R[] = {
	X__IMM_8,		  R_32_64__X,
	X__XM_128,		  X__XM_32,
	X__XM_64,		  X__M_64,
	X__MMM_64,		  X__RM_32_64,
	MM__XM_64,		  MM__XM_128,
	R_32_64__XM_32,	  R_32_64__XM_64,
	MM__MMM_64,		  X__X,

	X__XM_128__IMM_8, X__R_32_64_M_8__IMM_8,
	X__XM_32__IMM_8,  X__R_32_64_M_16__IMM_8,
	X__XM_64__IMM_8,  X__RM_32_64__IMM_8,
};

// these are needed for checking qword mem rex
const enum OpsCode X__RM_ARR[] = {
	X__RM_32_64,
	X__R_32_64_M_8__IMM_8,
	X__RM_32_64__IMM_8,
	X__R_32_64_M_16__IMM_8,
};
const enum OpsCode RM__X_ARR[] = {
	RM_32__X__IMM_8,	R_32_64_M_16__X__IMM_8,
	RM_32_64__X__IMM_8, R_32_64_M_8__X__IMM_8,
	RM_32_64__X,
};

const enum OpsCode XRM__XR_ARR[] = {
	XM_32__X,			XM_64__X,
	M_128__X,			M_64__X,
	XM_128__X,			RM_32_64__X,

	RM_32__X__IMM_8,	R_32_64_M_16__X__IMM_8,
	RM_32_64__X__IMM_8, R_32_64_M_8__X__IMM_8,
};
const enum OpsCode XR__XRM_ARR[] = {
	X__IMM_8,		  R_32_64__X,
	X__XM_128,		  X__XM_32,
	X__XM_64,		  X__M_64,
	X__MMM_64,		  X__RM_32_64,
	MM__XM_64,		  MM__XM_128,
	R_32_64__XM_32,	  R_32_64__XM_64,
	MM__MMM_64,		  X__X,

	X__XM_128__IMM_8, X__R_32_64_M_8__IMM_8,
	X__XM_32__IMM_8,  X__R_32_64_M_16__IMM_8,
	X__XM_64__IMM_8,  X__RM_32_64__IMM_8,
};
// TODO: are (XM_L and XRM__XR_ARR) and (XM_R and XR__XRM_ARR) equal

const long RM_L___LEN = lenofarr(RM_L);
const long RM_R___LEN = lenofarr(RM_R);
const long RM__R_ARR___LEN = lenofarr(RM__R_ARR);
const long R__RM_ARR___LEN = lenofarr(R__RM_ARR);
const long IMM_R___LEN = lenofarr(IMM_R);
const long XM_L___LEN = lenofarr(XM_L);
const long XM_R___LEN = lenofarr(XM_R);
const long X__RM_ARR___LEN = lenofarr(X__RM_ARR);
const long RM__X_ARR___LEN = lenofarr(RM__X_ARR);
const long XRM__XR_ARR___LEN = lenofarr(XRM__XR_ARR);
const long XR__XRM_ARR___LEN = lenofarr(XR__XRM_ARR);

int is_in_opsc(enum OpsCode c, const enum OpsCode arr[], size_t arr_len) {
	for (size_t i = 0; i < arr_len; i++)
		if (c == arr[i])
			return 1;
	return 0;
}

struct OpsCodeText {
	enum OpsCode code;
	const char *text;
};

const struct OpsCodeText OPS_CODES_TEXTS[] = {
	{OPC_INVALID, ""},
	{__REL_8, ""},
	{__REL_32, ""},
	{__IMM_8, ""},
	{__IMM_16, ""},
	{__IMM_32, ""},
	{__R_16_64, ""},
	{__RM_16_64, ""},
	{__GS, ""},
	{__FS, ""},
	{__RM_8, ""},
	{__RM_16_32_64, ""},
	{RM_8__R_8, "р/п8 р8"},
	{RM_16_32_64__R_16_32_64, ""},
	{R_8__RM_8, ""},
	{R_16_32_64__RM_16_32_64, ""},
	{AL__IMM_8, ""},
	{RAX__IMM_16_32, ""},
	{RM_8__IMM_8, ""},
	{RM_16_32_64__IMM_16_32, ""},
	{RM_16_32_64__IMM_8, ""},
	{M_16__SREG, ""},
	{R_16_32_64__SREG, ""},
	{SREG__RM_16, ""},
	{AL__MOFFS_8, ""},
	{RAX__MOFFS_16_32_64, ""},
	{MOFFS_8__AL, ""},
	{MOFFS_16_32_64__RAX, ""},
	{R_8__IMM_8, ""},
	{R_16_32_64__IMM_16_32_64, ""},
	{R_16_32_64__RM_16_32_64__IMM_16_32, ""},
	{R_16_32_64__RM_16_32_64__IMM_8, ""},
	{R_64__RM_32, ""},
	{R_16_32_64__RAX, ""},
	{R_16_32_64__M, ""},
	{IMM_16__IMM_8, ""},
	{EAX__IMM_8, ""},
	{IMM_8__AL, ""},
	{IMM_8__EAX, ""},
	{AL__DX, ""},
	{EAX__DX, ""},
	{DX__AL, ""},
	{DX__EAX, ""},
	{X__XM_128, ""},
	{X__XM_32, ""},
	{X__XM_64, ""},
	{XM_128__X, ""},
	{XM_32__X, ""},
	{XM_64__X, ""},
	{X__M_64, ""},
	{M_64__X, ""},
	{X__X, ""},
	{X__MMM_64, ""},
	{X__RM_32_64, ""},
	{M_128__X, ""},
	{MM__XM_64, ""},
	{R_32_64__XM_32, ""},
	{MM__XM_128, ""},
	{R_32_64__XM_64, ""},
	{MM__MMM_64, ""},
	{X__XM_128__IMM_8, ""},
	{X__XM_32__IMM_8, ""},
	{X__XM_64__IMM_8, ""},
	{R_32_64_M_8__X__IMM_8, ""},
	{R_32_64_M_16__X__IMM_8, ""},
	{RM_32_64__X__IMM_8, ""},
	{RM_32__X__IMM_8, ""},
	{X__R_32_64_M_8__IMM_8, ""},
	{X__R_32_64_M_16__IMM_8, ""},
	{X__RM_32_64__IMM_8, ""},
	{X__IMM_8, ""},
	{R_32_64__X, ""},
	{RM_32_64__X, ""},
	{M_32_64__R_32_64, ""},
};

const char *get_ops_text(enum OpsCode opsc) {
	const struct OpsCodeText *ct;

	for (size_t i = 0; i < lenofarr(OPS_CODES_TEXTS); i++) {
		ct = OPS_CODES_TEXTS + i;

		if (ct->code == opsc)
			return ct->text;
	}

	exit(222);
}
