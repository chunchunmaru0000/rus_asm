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
	{OPC_INVALID, "без операндов"},
	{__REL_8, "о8"},
	{__REL_32, "о32"},
	{__IMM_8, "ч8"},
	{__IMM_16, "ч16"},
	{__IMM_32, "ч32"},
	{__R_16_64, "р16/32"},
	{__RM_16_64, "рп16/64"},
	{__GS, "ГС"},
	{__FS, "ФС"},
	{__RM_8, "рп8"},
	{__RM_16_32_64, "рп16/32/64"},
	{RM_8__R_8, "рп8\t\tр8"},
	{RM_16_32_64__R_16_32_64, "рп16/32/64\tр16/32/64"},
	{R_8__RM_8, "р8\t\tрп8"},
	{R_16_32_64__RM_16_32_64, "р16/32/64\tрп16/32/64"},
	{AL__IMM_8, "АЛ\t\tч8"},
	{RAX__IMM_16_32, "рАХ\t\tч16/23"},
	{RM_8__IMM_8, "рп8\t\tч8"},
	{RM_16_32_64__IMM_16_32, "рп16/32/64\tч16/32"},
	{RM_16_32_64__IMM_8, "рп16/32/64\tч8"},
	{M_16__SREG, "п16\t\tуч"},
	{R_16_32_64__SREG, "р16/32/64\tуч"},
	{SREG__RM_16, "уч\t\tрп16"},
	{AL__MOFFS_8, "АЛ\t\tадр8"},
	{RAX__MOFFS_16_32_64, "рАХ\t\tадр16/32/64"},
	{MOFFS_8__AL, "адр8\tАЛ"},
	{MOFFS_16_32_64__RAX, "адр16/32/64\tрАХ"},
	{R_8__IMM_8, "р8\t\tч8"},
	{R_16_32_64__IMM_16_32_64, "р16/32/64\tч16/32/64"},
	{R_16_32_64__RM_16_32_64__IMM_16_32, "р16/32/64\tрп16/32/64\tч16/32"},
	{R_16_32_64__RM_16_32_64__IMM_8, "р16/32/64\tрп16/32/64\tч8"},
	{R_64__RM_32, "р64\t\tрп32"},
	{R_16_32_64__RAX, "р16/32/64\tрАХ"},
	{R_16_32_64__M, "р16/32/64\tп"},
	{IMM_16__IMM_8, "ч16\t\tч8"},
	{EAX__IMM_8, "ЕАХ\t\tч8"},
	{IMM_8__AL, "ч8\t\tАЛ"},
	{IMM_8__EAX, "ч8\t\tЕАХ"},
	{AL__DX, "АЛ\t\tДХ"},
	{EAX__DX, "ЕАХ\t\tДХ"},
	{DX__AL, "ДХ\t\tАЛ"},
	{DX__EAX, "ДХ\t\tЕАХ"},
	{X__XM_128, "э\t\tэм128"},
	{X__XM_32, "э\t\tэм32"},
	{X__XM_64, "э\t\tэм64"},
	{XM_128__X, "эм128\tэ"},
	{XM_32__X, "эм32\tэ"},
	{XM_64__X, "эм64\tэ"},
	{X__M_64, "э\t\tм64"},
	{M_64__X, "п64\t\tэ"},
	{X__X, "э\t\tэ"},
	{X__MMM_64, "э\t\tммп64"},
	{X__RM_32_64, "э\t\tрп32/64"},
	{M_128__X, "п128\tэ"},
	{MM__XM_64, "мм\t\tэм64"},
	{R_32_64__XM_32, "р32/64\tэм32"},
	{MM__XM_128, "мм\t\tэм128"},
	{R_32_64__XM_64, "р32/64\tэм64"},
	{MM__MMM_64, "мм\t\tммп64"},
	{X__XM_128__IMM_8, "э\t\tэм128\t"},
	{X__XM_32__IMM_8, "э\t\tэм32\t\tч8"},
	{X__XM_64__IMM_8, "э\t\tэм64\t\tч8"},
	{R_32_64_M_8__X__IMM_8, "р32/64п8\tэ\t\tч8"},
	{R_32_64_M_16__X__IMM_8, "р32/64п16\tэ\t\tч8"},
	{RM_32_64__X__IMM_8, "рп32/64\tэ\t\tч8"},
	{RM_32__X__IMM_8, "рп32\tэ\t\tч8"},
	{X__R_32_64_M_8__IMM_8, "э\t\tр32/64п8\tч8"},
	{X__R_32_64_M_16__IMM_8, "э\t\tр32/64п16\tч8"},
	{X__RM_32_64__IMM_8, "э\t\tрп32/64\tч8"},
	{X__IMM_8, "э\t\tч8"},
	{R_32_64__X, "р32/64\tэ"},
	{RM_32_64__X, "рп32/64\tэ"},
	{M_32_64__R_32_64, "п32/64\tр32/64"},
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
