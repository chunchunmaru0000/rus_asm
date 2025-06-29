#include "pser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void ee_token(struct Fpfc *f, struct Token *t, char *msg) { // error exit
	fprintf(stderr, "%s%s:%d:%d:%s ОШИБКА: %s [%s]:[%d]%s\n", COLOR_WHITE,
			f->path, t->p->line, t->p->col, COLOR_RED, msg, t->view, t->code,
			COLOR_RESET);
	print_source_line(f->code, t->p->line, COLOR_LIGHT_RED);
	exit(1);
}

// print warning
void pw(struct Fpfc *f, struct Pos *p, const char *const msg) {
	if (!NEED_WARN)
		return;
	fprintf(stderr, "%s%s:%d:%d%s ПРЕДУПРЕЖДЕНИЕ: %s%s\n", COLOR_WHITE, f->path,
			p->line, p->col, COLOR_LIGHT_PURPLE, msg, COLOR_RESET);
	print_source_line(f->code, p->line, COLOR_LIGHT_PURPLE);
}

int get_reg_field(enum RegCode rm) {
	int f = -1;
	if (is_f_reg8(rm))
		f = rm - R_AL;
	else if (is_f_reg16(rm))
		f = rm - R_AX;
	else if (is_f_reg32(rm))
		f = rm - R_EAX;
	else if (is_f_reg64(rm))
		f = rm - R_RAX;
	else if (is_f_xmm(rm))
		f = rm - R_XMM0;
	else if (is_f_mm(rm))
		f = rm - R_MM0;
	else if (is_f_seg(rm))
		f = rm - R_ES;
	if (f > 0b111)
		f -= 0b1000;
	return f;
}

void print_oper(struct Oper *o) {
	printf("[view:%s][oc:%d][rm:%d]\t[base:%d]"
		   "[scale:%d][index:%d][disp:%d][mod:%d][rex:%d]\n",
		   o->t->view, o->code, get_reg_field(o->rm), get_reg_field(o->base),
		   1 << o->scale, get_reg_field(o->index), o->disp, o->mod, o->rex);
}

struct Pser *new_pser(char *filename, uc debug) {
	struct Pser *p = malloc(sizeof(struct Pser));
	struct Tzer *t = new_tzer(filename);
	p->f = t->f;

	struct PList *ts = tze(t, 10);
	free(t);
	p->pos = 0;
	p->ts = ts;
	p->debug = debug;
	p->ds = new_plist(3);
	return p;
}

struct Inst *new_inst(struct Pser *p, enum ICode code, struct PList *os,
					  struct Token *t) {
	struct Inst *i = malloc(sizeof(struct Inst));
	struct Pos *pos = malloc(sizeof(struct Pos));
	i->f = p->f;
	pos->col = t->p->col;
	pos->line = t->p->line;
	i->p = pos;

	i->code = code;
	i->os = os;
	return i;
}

struct Token *gettp(struct Pser *p, long off) {
	long i = p->pos + off;
	return p->ts->size > i ? p->ts->st[i] : p->ts->st[p->ts->size - 1];
}

struct Token *next_get(struct Pser *p, long off) {
	p->pos++;
	return gettp(p, off);
}

// parser directives
const char *STR_DEFINE = "вот";
const char *STR_INCLUDE = "влечь";
// directives
const char *STR_ENTRY = "вход";
const char *STR_SEG = "участок";
const char *STR_SEG_R = "чит";
const char *STR_SEG_W = "изм";
const char *STR_SEG_X = "исп";
const char *STR_EOF = "_КОНЕЦ_ФАЙЛА_";
const char *STR_LET = "пусть";
const char *STR_ADDR = "адр";
const char *STR_ALIGN = "равнять";
// instruction words
// load - выг рузить
// store - заг рузить
// scan - осм отреть
const struct Word VAR_OPS_WORDS[] = {
	{"возд", IRET}, {"воздф", IRETF}, {"зумн", IIMUL}, {"врщл", IROL},
	{"врщп", IROR}, {"врсл", IRCL},	  {"врсп", IRCR},  {"сдвл", ISHL},
	{"сдал", ISHL}, {"сдвп", ISHR},	  {"сдап", ISAR},
};
const struct Word ZERO_OPS_WORDS[] = {
	{"перебд", ICBW},	{"передч", ICWDE},	{"перечв", ICDQE},
	{"превдч", ICWD},	{"превчв", ICDQ},	{"преввш", ICQO},
	{"пауза", IPAUSE},	{"ожид", IWAIT},	{"сзов", ISYSCALL},
	{"воздф", IRETF},	{"атом", ILOCK},	{"ыыы", INOP},
	{"ввсб", IINSB},	{"ввсд", IINSW},	{"ввсч", IINSD},
	{"вывсб", IOUTSB},	{"вывсд", IOUTSW},	{"вывсч", IOUTSD},
	{"толкф", IPUSHF},	{"вытф", IPOPF},	{"загахф", ISAHF},
	{"выгахф", ILAHF},	{"бытьсб", IMOVSB}, {"бытьсд", IMOVSW},
	{"бытьсч", IMOVSD}, {"бытьсв", IMOVSQ}, {"сравсб", ICMPSB},
	{"сравсд", ICMPSW}, {"сравсч", ICMPSD}, {"сравсв", ICMPSQ},
	{"осмсб", ISCASB},	{"осмсд", ISCASW},	{"осмсч", ISCASD},
	{"осмсв", ISCASQ},	{"выгсб", ILODSB},	{"выгсд", ILODSW},
	{"выгсч", ILODSD},	{"выгсв", ILODSQ},	{"загсб", ISTOSB},
	{"загсд", ISTOSW},	{"загсч", ISTOSD},	{"загсв", ISTOSQ},
	{"прер3", IINT3},	{"воздфв", IRETFQ}, {"выйти", ILEAVE},
	{"прер1", IINT1},	{"ост", IHLT},		{"нсф", ICMC},
	{"осф", ICLC},		{"усф", ISTC},		{"опрерф", ICLI},
	{"упрерф", ISTI},	{"орф", ICLD},		{"урф", ISTD},
	{"испфс", IUSEFS},	{"испгс", IUSEGS},	{"повтн0", IREPNZ},
	{"повтнр", IREPNZ}, {"повт", IREP},
};
// not			н 	не
// overflow 	и	избыток
// below		п	под
// above		в 	выше
// equ			р 	равно
// carry		с 	снести
// zero			0 	ноль
// sign			з 	знак
// parity_even	ч 	чет
// parity_odd 	нч 	не чет
// less			м 	меньше
// greater		б	больше

// TODO: make segments addresation, like cs:123
// TODO: make AVX
// TODO: make FPU
// TODO: make mm
// TODO: make string ops with 32-bit addressation
const struct Word ONE_OPS_WORDS[] = {
	{"зов", ICALL},		{"идти", IJMP},		{"прер", IINT},
	{"выт", IPOP},		{"толк", IPUSH},	{"идтиф", IJMPF},
	{"зовф", ICALLF},	{"увлч", IINC},		{"умнш", IDEC},
	{"воздф", IRETF},	{"возд", IRET},		{"дел", IDIV},
	{"здел", IIDIV},	{"умн", IMUL},		{"врщл1", IROL1},
	{"врщп1", IROR1},	{"врсл1", IRCL1},	{"врсп1", IRCR1},
	{"сдвл1", ISHL1},	{"сдал1", ISHL1},	{"сдвп1", ISHR1},
	{"сдап1", ISAR1},	{"цклн0", ILOOPNZ}, {"цклнр", ILOOPNZ},
	{"цкл0", ILOOPZ},	{"цклр", ILOOPZ},	{"цкл", ILOOP},
	{"идесх0", IJECXZ}, {"идрсх0", IJRCXZ}, {"не", INOT},
	{"нег", INEG},

	{"идни", IJNO},		{"идп", IJB},		{"иднвр", IJB},
	{"идс", IJB},		{"иднп", IJNB},		{"идвр", IJNB},
	{"иднс", IJNB},		{"ид0", IJE},		{"идр", IJE},
	{"идн0", IJNE},		{"иднр", IJNE},		{"идпр", IJBE},
	{"иднв", IJBE},		{"иднпр", IJA},		{"идв", IJA},
	{"идз", IJS},		{"иднз", IJNS},		{"идч", IJP},
	{"идчр", IJP},		{"иднч", IJNP},		{"иднч", IJNP},
	{"идм", IJL},		{"иднбр", IJL},		{"иднм", IJNL},
	{"идбр", IJNL},		{"идмр", IJLE},		{"иднб", IJLE},
	{"иднмр", IJG},		{"идб", IJG},		{"иди", IJO},

	{"устни", ISETNO},	{"устп", ISETB},	{"устнвр", ISETB},
	{"устс", ISETB},	{"устнп", ISETNB},	{"уствр", ISETNB},
	{"устнс", ISETNB},	{"уст0", ISETE},	{"устр", ISETE},
	{"устн0", ISETNE},	{"устнр", ISETNE},	{"устпр", ISETBE},
	{"устнв", ISETBE},	{"устнпр", ISETA},	{"уств", ISETA},
	{"устз", ISETS},	{"устнз", ISETNS},	{"устч", ISETP},
	{"устчр", ISETP},	{"устнч", ISETNP},	{"устнч", ISETNP},
	{"устм", ISETL},	{"устнбр", ISETL},	{"устнм", ISETNL},
	{"устбр", ISETNL},	{"устмр", ISETLE},	{"устнб", ISETLE},
	{"устнмр", ISETG},	{"устб", ISETG},	{"усти", ISETO},
};
// Single		о один
// Double		д два
// Scalar		с скалярный
// Un aligned	нр не ровный
// Packed		у упакованный
// Duplicate	удв удвоить
// High			в высок
// Low			н низ
// Unpack		рсп распаковать
// Convert		пре преобразовать
// Compare		срав сравнить
// Unordered	ну не упорядоченный
// Integers		ч число
// Non temp		нв не временный
// Truncate		о округлять
// Memory		память
// Round		окр
// Blend		смш
// Align		рвн
// Extract		извл
// Insert		вств
// Dot Product	спр скалярное произведение
// SAD			сумар
// Multiple		м
// Explicit		я
// Return		в воздать
// Reciprocal	обратный
// Shuffle		тас тасовать
const struct Word TWO_OPS_WORDS[] = {
	{"быть", IMOV},
	{"плюс", IADD},
	{"минс", ISUB},
	{"проб", ITEST},
	{"срав", ICMP},
	{"или", IOR},
	{"и", IAND},
	{"искл", IXOR},
	{"плюсс", IADC},
	{"минсп", ISBB},
	{"обмн", IXCHG},
	{"задр", ILEA},
	{"войти", IENTER},
	{"ввд", IINPUT},
	{"вывд", IOUTPUT},

	{"сбытьни", ICMOVNO},
	{"сбытьп", ICMOVB},
	{"сбытьнвр", ICMOVB},
	{"сбытьс", ICMOVB},
	{"сбытьнп", ICMOVNB},
	{"сбытьвр", ICMOVNB},
	{"сбытьнс", ICMOVNB},
	{"сбыть0", ICMOVE},
	{"сбытьр", ICMOVE},
	{"сбытьн0", ICMOVNE},
	{"сбытьнр", ICMOVNE},
	{"сбытьпр", ICMOVBE},
	{"сбытьнв", ICMOVBE},
	{"сбытьнпр", ICMOVA},
	{"сбытьв", ICMOVA},
	{"сбытьз", ICMOVS},
	{"сбытьнз", ICMOVNS},
	{"сбытьч", ICMOVP},
	{"сбытьчр", ICMOVP},
	{"сбытьнч", ICMOVNP},
	{"сбытьнч", ICMOVNP},
	{"сбытьм", ICMOVL},
	{"сбытьнбр", ICMOVL},
	{"сбытьнм", ICMOVNL},
	{"сбытьбр", ICMOVNL},
	{"сбытьмр", ICMOVLE},
	{"сбытьнб", ICMOVLE},
	{"сбытьнмр", ICMOVG},
	{"сбытьб", ICMOVG},
	{"сбытьи", ICMOVO},

	{"бнруо", IMOVUPS},
	{"бсо", IMOVSS},
	{"бнруд", IMOVUPD},
	{"бсд", IMOVSD_XMM},
	{"бвн", IMOVHLPS},
	{"бнв", IMOVLHPS},
	{"бнуо", IMOVLPS},
	{"бнуд", IMOVLPD},
	{"бвуо", IMOVHPS},
	{"бвуд", IMOVHPD},
	{"бдудв", IMOVDDUP},
	{"бонудв", IMOVSLDUP},
	{"бовудв", IMOVSHDUP},
	{"брспнуо", IUNPCKLPS},
	{"брспнуд", IUNPCKLPD},
	{"брспуо", IUNPCKHPS},
	{"брспуд", IUNPCKHPD},

	{"бууо", IMOVAPS},
	{"бууд", IMOVAPD},
	{"бнвуо", IMOVNTPS},
	{"бнвуд", IMOVNTPD},
	{"супчсо", ICOMISS},
	{"супчсд", ICOMISD},
	{"нусравсо", IUCOMISS},
	{"нусравсд", IUCOMISD},
	{"преуч2уо", ICVTPI2PS},
	{"пресч2со", ICVTSI2SS},
	{"преуч2уд", ICVTPI2PD},
	{"пресч2сд", ICVTSI2SD},
	{"преуо2уч", ICVTPS2PI},
	{"пресо2зч", ICVTSS2SI},
	{"преуд2уч", ICVTPD2PI},
	{"пресд2зч", ICVTSD2SI},
	{"преоуо2уч", ICVTTPS2PI},
	{"преосо2зч", ICVTTSS2SI},
	{"преоуд2уч", ICVTTPD2PI},
	{"преосд2зч", ICVTTSD2SI},

	{"урвнп_мм", IPALIGNR_MM},
	{"урвнп", IPALIGNR},
	{"спруо", IDPPS},
	{"спруд", IDPPD},

	{"бмзуо", IMOVMSKPS},
	{"бмзуд", IMOVMSKPD},
	{"крньуо", ISQRTPS},
	{"крньсо", ISQRTSS},
	{"крньуд", ISQRTPD},
	{"крньсд", ISQRTSD},
	{"окрньуо", IRSQRTPS},
	{"окрньсо", IRSQRTSS},
	{"оуо", IRCPPS},
	{"осо", IRCPSS},
	{"иуо", IANDPS},
	{"иуд", IANDPD},
	{"инуо", IANDNPS},
	{"инуд", IANDNPD},
	{"илиуо", IORPS},
	{"илиуд", IORPD},
	{"исклуо", IXORPS},
	{"исклуд", IXORPD},
	{"плюсуо", IADDPS},
	{"плюссо", IADDSS},
	{"плюсуд", IADDPD},
	{"плюссд", IADDSD},
	{"умнуо", IMULPS},
	{"умнсо", IMULSS},
	{"умнуд", IMULPD},
	{"умнсд", IMULSD},
	{"преуо2уд", ICVTPS2PD},
	{"преуд2уо", ICVTPD2PS},
	{"пресо2сд", ICVTSS2SD},
	{"пресд2со", ICVTSD2SS},
	{"пречч2уо", ICVTDQ2PS},
	{"преуо2чч", ICVTPS2DQ},
	{"преоуо2чч", ICVTTPS2DQ},
	{"минсуо", ISUBPS},
	{"минссо", ISUBSS},
	{"минсуд", ISUBPD},
	{"минссд", ISUBSD},
	{"минмуо", IMINPS},
	{"минмсо", IMINSS},
	{"минмуд", IMINPD},
	{"минмсд", IMINSD},
	{"делуо", IDIVPS},
	{"делсо", IDIVSS},
	{"делуд", IDIVPD},
	{"делсд", IDIVSD},
	{"максуо", IMAXPS},
	{"макссо", IMAXSS},
	{"максуд", IMAXPD},
	{"макссд", IMAXSD},
	{"урспнбд", IPUNPCKLBW},
	{"урспндч", IPUNPCKLWD},
	{"урспнчв", IPUNPCKLDQ},
	{"уупкзндб", IPACKSSWB},
	{"усраввб", IPCMPGTB},
	{"усраввд", IPCMPGTW},
	{"усраввч", IPCMPGTD},
	{"уупкбндб", IPACKUSWB},
	{"урспвбд", IPUNPCKHBW},
	{"урспвдч", IPUNPCKHWD},
	{"урспвчв", IPUNPCKHDQ},
	{"уупкзнчд", IPACKSSDW},
	{"урспнвдв", IPUNPCKLQDQ},
	{"урспввдв", IPUNPCKHQDQ},
	{"бытьчвр", IMOVDQA},
	{"бытьчвнр", IMOVDQU},
	{"усплд", IPSRLW},
	{"успад", IPSRAW},
	{"усллд", IPSLLW},
	{"усплч", IPSRLD},
	{"успач", IPSRAD},
	{"усллч", IPSLLD},
	{"усплв", IPSRLQ},
	{"усплдв", IPSRLDQ},
	{"усллв", IPSLLQ},
	{"усллдв", IPSLLDQ},
	{"усраврб", IPCMPEQB},
	{"усраврд", IPCMPEQW},
	{"усраврч", IPCMPEQD},
	{"гплюсуд", IHADDPD},
	{"гплюсуо", IHADDPS},
	{"гминсуд", IHSUBPD},
	{"гминсуо", IHSUBPS},
	{"бытьэ", IMOV_XMM},

	{"бнвч", IMOVNTI},
	{"плюсминсуд", IADDSUBPD},
	{"плюсминсуо", IADDSUBPS},
	{"уплюсв", IPADDQ},
	{"уумннд", IPMULLW},
	// {"бв2дв", IMOVQ2DQ},
	// {"бдв2в", IMOVDQ2Q},
	{"убмскб", IPMOVMSKB},
	{"уминсбзнб", IPSUBUSB},
	{"уминсбзнд", IPSUBUSW},
	{"уминмбзб", IPMINUB},
	{"уи", IPAND},
	{"уплюснрзб", IPADDUSB},
	{"уплюснрзд", IPADDUSW},
	{"умаксбзб", IPMAXUB},
	{"уин", IPANDN},
	{"усрдб", IPAVGB},
	{"усрдд", IPAVGW},
	{"уумнвбзд", IPMULHUW},
	{"уумнвд", IPMULHW},
	{"преуд2чв", ICVTPD2DQ},
	{"преоуд2чв", ICVTTPD2DQ},
	{"пречв2уд", ICVTDQ2PD},
	{"бнвчв", IMOVNTDQ},
	{"уминсзб", IPSUBSB},
	{"уминсзд", IPSUBSW},
	{"уминмзд", IPMINSW},
	{"уили", IPOR},
	{"уплюсзб", IPADDSB},
	{"уплюсзд", IPADDSW},
	{"умаксзд", IPMAXSW},
	{"уискл", IPXOR},
	{"здвнр", ILDDQU},
	{"уумннрчв", IPMULUDQ},
	{"ууплюсдч", IPMADDWD},
	{"усумарбд", IPSADBW},
	{"мскбдвнр", IMASKMOVDQU},
	{"уминсб", IPSUBB},
	{"уминсд", IPSUBW},
	{"уминсч", IPSUBD},
	{"уминсв", IPSUBQ},
	{"уплюсб", IPADDB},
	{"уплюсд", IPADDW},
	{"уплюсч", IPADDD},
};
const struct Word TRI_OPS_WORDS[] = {
	{"окруо", IROUNDPS},	  {"окруд", IROUNDPD},	   {"окрсо", IROUNDSS},
	{"окрсд", IROUNDSD},	  {"смшуо", IBLENDPS},	   {"смшуд", IBLENDPD},

	{"усмшд", IPBLENDW},	  {"уизвлб", IPEXTRB},	   {"уизвлд", IPEXTRW},
	{"уизвл", IPEXTRDQ},	  {"извлуо", IEXTRACTPS},  {"увствб", IPINSRB},
	{"вствуо", IINSERTPS},	  {"увтсв", IPINSRDQ},	   {"мусумарбд", IMPSADBW},
	{"усравям", IPCMPESTRM},  {"усравяи", IPCMPESTRI}, {"усравням", IPCMPISTRM},
	{"усравняи", IPCMPISTRI},

	{"утаснд", IPSHUFLW},	  {"утасвд", IPSHUFHW},	   {"утасч", IPSHUFD},

	{"сравуо", ICMPPS},		  {"сравсо", ICMPSS},	   {"сравуд", ICMPPD},
	{"сравсдэ", ICMPSD_XMM},  {"увстврд", IPINSRW},	   {"тасуо", ISHUFPS},
	{"тасуд", ISHUFPD},
};
// seg
const struct Reg SEG_REGS[] = {{R_CS, "кс"},	 {R_DS, "дс"}, {R_ES, "ес"},
							   {R_SS, "сс"},	 {R_FS, "фс"}, {R_GS, "гс"},
							   {R_LDTR, "лдтр"}, {R_TR, "тр"}};
// 8
const struct Reg B_REGS[] = {
	{R_AH, "аг"},	  {R_DH, "дг"},		{R_CH, "сг"},	  {R_BH, "бг"},
	{R_AL, "ал"},	  {R_DL, "дл"},		{R_CL, "сл"},	  {R_BL, "бл"},
	{R_R8B, "б8"},	  {R_R9B, "б9"},	{R_R10B, "б10"},  {R_R11B, "б11"},
	{R_R12B, "б12"},  {R_R13B, "дб3"},	{R_R14B, "б14"},  {R_R15B, "б15"},
	{R_R8B, "р8б"},	  {R_R9B, "р9б"},	{R_R10B, "р10б"}, {R_R11B, "р11б"},
	{R_R12B, "р12б"}, {R_R13B, "р13б"}, {R_R14B, "р14б"}, {R_R15B, "р15б"}};
// 16
const struct Reg W_REGS[] = {
	{R_AX, "ах"},	  {R_CX, "сх"},		 {R_DX, "дх"},	   {R_BX, "бх"},
	{R_SP, "сп"},	  {R_BP, "бп"},		 {R_SI, "си"},	   {R_DI, "ди"},
	{R_R8W, "д8"},	  {R_R9W, "д9"},	 {R_R10W, "д10"},  {R_R11W, "д11"},
	{R_R12W, "д12"},  {R_R13W, "д13"},	 {R_R14W, "д14"},  {R_R15W, "д15"},
	{R_R8W, "р8д"},	  {R_R9W, "р9д"},	 {R_R10W, "р10д"}, {R_R11W, "р11д"},
	{R_R12W, "р12д"}, {R_R13W, "р13д"},	 {R_R14W, "р14д"}, {R_R15W, "р15д"},
	{R_IP, "ип"},	  {R_FLAGS, "флаги"}};
// 32
const struct Reg E_REGS[] = {
	{R_EAX, "еах"},	  {R_EBX, "ебх"},	   {R_ECX, "есх"},	 {R_EDX, "едх"},
	{R_ESI, "еси"},	  {R_EDI, "еди"},	   {R_EBP, "ебп"},	 {R_ESP, "есп"},
	{R_R8D, "е8"},	  {R_R9D, "е9"},	   {R_R10D, "е10"},	 {R_R11D, "е11"},
	{R_R12D, "е12"},  {R_R13D, "е13"},	   {R_R14D, "е14"},	 {R_R15D, "е15"},
	{R_R8D, "р8ч"},	  {R_R9D, "р9ч"},	   {R_R10D, "р10ч"}, {R_R11D, "р11ч"},
	{R_R12D, "р12ч"}, {R_R13D, "р13ч"},	   {R_R14D, "р14ч"}, {R_R15D, "р15ч"},
	{R_EIP, "еип"},	  {R_EFLAGS, "ефлаги"}};
// 64
const struct Reg R_REGS[] = {
	{R_RAX, "рах"}, {R_RBX, "рбх"},		 {R_RCX, "рсх"}, {R_RDX, "рдх"},
	{R_RSI, "рси"}, {R_RDI, "рди"},		 {R_RBP, "рбп"}, {R_RSP, "рсп"},
	{R_R8, "р8"},	{R_R9, "р9"},		 {R_R10, "р10"}, {R_R11, "р11"},
	{R_R12, "р12"}, {R_R13, "р13"},		 {R_R14, "р14"}, {R_R15, "р15"},
	{R_RIP, "рип"}, {R_RFLAGS, "рфлаги"}};
// xmm
const struct Reg XMM_REGS[] = {
	{R_XMM0, "хмм0"},	{R_XMM1, "хмм1"},	{R_XMM2, "хмм2"},
	{R_XMM3, "хмм3"},	{R_XMM4, "хмм4"},	{R_XMM5, "хмм5"},
	{R_XMM6, "хмм6"},	{R_XMM7, "хмм7"},	{R_XMM8, "хмм8"},
	{R_XMM9, "хмм9"},	{R_XMM10, "хмм10"}, {R_XMM11, "хмм11"},
	{R_XMM12, "хмм12"}, {R_XMM13, "хмм13"}, {R_XMM14, "хмм14"},
	{R_XMM15, "хмм15"},

	{R_XMM0, "э0"},		{R_XMM1, "э1"},		{R_XMM2, "э2"},
	{R_XMM3, "э3"},		{R_XMM4, "э4"},		{R_XMM5, "э5"},
	{R_XMM6, "э6"},		{R_XMM7, "э7"},		{R_XMM8, "э8"},
	{R_XMM9, "э9"},		{R_XMM10, "э10"},	{R_XMM11, "э11"},
	{R_XMM12, "э12"},	{R_XMM13, "э13"},	{R_XMM14, "э14"},
	{R_XMM15, "э15"},
};
// mm
const struct Reg MM_REGS[] = {
	{R_MM0, "мм0"}, {R_MM1, "мм1"}, {R_MM2, "мм2"}, {R_MM3, "мм3"},
	{R_MM4, "мм4"}, {R_MM5, "мм5"}, {R_MM6, "мм6"}, {R_MM7, "мм7"},
};
// ymm - ю
// zmm - я

char *CANT_CHANGE_REG_SZ =
	"Нельзя менять размер регистра, это ни к чему не приведет так и так.";

uc cont_str(char *view, const char **strs, long strs_len) {
	for (int i = 0; i < strs_len; i++)
		if (sc(view, strs[i]))
			return 1;
	return 0;
}

enum ICode entry_i(struct Pser *p, struct PList *os) {
	plist_add(os, next_get(p, 0));
	next_get(p, 0);
	return IENTRY;
}
enum ICode seg_i(struct Pser *p, struct PList *os) {
	struct Token *t = next_get(p, 0);
	int *flags = malloc(sizeof(int));
	*flags = 0;
	if (t->code == INT)
		*flags = t->number;
	else
		while (t->code != SLASHN && t->code != EF) {
			if (sc(t->view, STR_SEG_X))
				*flags |= 0b1;
			else if (sc(t->view, STR_SEG_W))
				*flags |= 0b10;
			else if (sc(t->view, STR_SEG_R))
				*flags |= 0b100;
			t = next_get(p, 0);
		}
	next_get(p, 0); // skip \n
	plist_add(os, flags);
	return ISEGMENT;
}

struct Oper *expression(struct Pser *);

enum ICode n_ops_i(struct Pser *p, int n, struct PList *os, enum ICode code) {
	next_get(p, 0); // skip instruction
	for (int i = 0; i < n; i++)
		plist_add(os, expression(p));
	return code;
}

#define set_tc(tp, cp, t, c, sp, s)                                            \
	do {                                                                       \
		*(tp) = (t);                                                           \
		*(cp) = (c);                                                           \
		*(sp) = (s);                                                           \
	} while (0)
// void set_tc(struct Token **tp, enum OCode *cp, struct Token *t, enum
// OCode c) { *tp = t; *cp = c; }

const char *STRS_SIZES[] = {"байт",	 "дбайт", "чбайт", "вбайт",
							"эбайт", "юбайт", "ябайт"};
const char *STR_RESERVED = "запас";
int search_size(struct Pser *p, struct Oper **o, char *v) {
	for (uint32_t i = 0; i < lenofarr(STRS_SIZES); i++)
		if (sc(v, STRS_SIZES[i])) {
			*o = expression(p);
			if (is_reg(*o) || is_xmm(*o) || is_mm(*o))
				ee_token(p->f, (*o)->t, CANT_CHANGE_REG_SZ);
			(*o)->sz = 1 << i;
			(*o)->forsed_sz = 1;
			return 1;
		}
	return 0;
}

int is_size_word(char *v) {
	if (sc(v, STR_RESERVED))
		return RESERVED; // -1
	for (uint32_t i = 0; i < lenofarr(STRS_SIZES); i++)
		if (sc(v, STRS_SIZES[i]))
			return 1 << i;
	return 0;
}

int search_defn(struct Pser *p, struct Oper **o, char *v) {
	struct Defn *d;
	for (int i = 0; i < p->ds->size; i++) {
		d = plist_get(p->ds, i);
		if (sc(v, d->view)) {
			*o = d->value;
			return 1;
		}
	}
	return 0;
}

struct Defn *is_defn(struct Pser *p, char *v) {
	struct Defn *d;
	for (int i = 0; i < p->ds->size; i++) {
		d = plist_get(p->ds, i);
		if (sc(v, d->view))
			return d;
	}
	return 0;
}

char *ERR_WRONG_TOKEN = "Неверное выражение.";
char *ERR_WRONG_MINUS = "Минус можно использовать только перед числами.";
char *INVALID_STR_LEN =
	"Длина строки в выражении не может быть равна 0 или быть больше 2.";
char *TOO_MUCH_OS =
	"Слишком много или мало выражений в адресанте, максимум может быть "
	"4, минимум 1, в следующих порядках:\n"
	"\t[<регистр> <множитель: 1,2,4,8> регистр <смещение>]\n"
	"\t[смещение]\n";
char *POSSIBLE_WRONG_ORDER =
	"Возможно выражения в адресанте были в неверном порядке, "
	"всевозможные порядки:\n"
	"\t[<регистр> <множитель: 1,2,4,8> регистр <смещение>]\n"
	"\t[смещение]\n";
char *WRONG_ADDRES_OP = "Неверное выражение внутри адресанта, ими могут быть "
						"только регистры, метки или целые числа.";
char *WRONG_DISP = "Неверное смещение, ожидалось число 32 бит или метка.";
char *WRONG_SCALE =
	"Неверный множитель, ожидалось число(0, 2, 4, 8) или его отсутствие.";
char *WRONG_INDEX = "Неверный индекс, ожидался регистр.";
char *WRONG_RM = "Неверное выражение, ожидался регистр.";
char *WRONG_BASE = "Неверная основа, ожидался регистр.";
char *WRONG_MOFFS = "Неверное выражение, ожидалось число в качестве адреса.";
char *WRONG_ADDR_REG_SZ =
	"Неверный размер регистра в выражении операнда, адресными регистрами могут "
	"быть только 32-х или 64-х битные регистры.";
char *WRONG_MOFFS_MEM_SZ = "Неверный размер адреса, адресация в х64 может быть "
						   "только или 64 бит или 32 бит.";
// МИО - Множитель Индекс Основа
char *FORBIDDEN_RSP_INDEX =
	"Регистр рсп или есп запрещены в качестве индекса в МИО байте.";
char *DISSERENT_SIZE_REGS = "Регистры адресанта не могут быть разных размеров, "
							"только или все 64 бит или 32 бит.";

enum RegCode get_mem_reg(enum RegCode r) {
	enum RegCode n = R_NONE; // new
	if (r >= R_RAX && r <= R_RDI)
		n = r;
	else if (r >= R_R8 && r <= R_R15)
		n = r - R_R8 + R_RAX;
	else if (r >= R_EAX && r <= R_EDI)
		n = r - R_EAX + R_RAX;
	else if (r >= R_R8D && r <= R_R15D)
		n = r - R_R8D + R_RAX;
	if (n < R_RAX || n > R_RDI) {
		printf("\t\t\treg cvt err\n"); // 16 bytes str
		exit(1);
	}
	return n;
}

int search_reg(char *v, const int regs_len, const struct Reg regs[],
			   struct Oper *o, uc sz) {
	for (int i = 0; i < regs_len; i++)
		if (sc(v, regs[i].v)) {
			o->rm = regs[i].c;
			o->sz = sz;
			o->code = OREG;
			o->mod = MOD_REG;
			return 1;
		}
	return 0;
}

int search_some_reg(char *v, const int regs_len, const struct Reg regs[],
					struct Oper *o, enum OCode code, int sz) {
	for (int i = 0; i < regs_len; i++)
		if (sc(v, regs[i].v)) {
			o->rm = regs[i].c;
			o->code = code;
			o->sz = sz;
			return 1;
		}
	return 0;
}

void set_disp_to_op(struct Pser *p, struct Oper *o, struct Oper *d) {
	if (d->code == OINT) {
		int disp = d->t->number;
		if (disp < -128 || disp > 127)
			o->mod = MOD_MEM_D32;
		else
			o->mod = MOD_MEM_D8;
		o->disp = disp;
	} else if (d->code == OREL) {
		o->disp_is_rel_flag = 1;
		o->mod = MOD_MEM_D32;
		o->rel_view = d->t->view;
	} else
		ee_token(p->f, d->t, WRONG_DISP);
	free(d);
}
void set_scale_to_op(struct Pser *p, struct Oper *o, struct Oper *s) {
	if (!(s->code == OINT))
		ee_token(p->f, s->t, WRONG_SCALE);
	int scale = s->t->number;
	if (scale == 1)
		o->scale = SCALE_1;
	else if (scale == 2)
		o->scale = SCALE_2;
	else if (scale == 4)
		o->scale = SCALE_4;
	else if (scale == 8)
		o->scale = SCALE_8;
	else
		ee_token(p->f, s->t, WRONG_SCALE);
	free(s);
}
void set_index_to_op(struct Pser *p, struct Oper *o, struct Oper *i) {
	if (!(i->code == OREG))
		ee_token(p->f, i->t, WRONG_INDEX);
	if (is_rsp_addr(i))
		ee_token(p->f, i->t, FORBIDDEN_RSP_INDEX);
	if (is_r_new(i))
		o->rex |= REX_X;
	o->index = get_mem_reg(i->rm);
	free(i);
}
void set_base_to_op(struct Pser *p, struct Oper *o, struct Oper *b) {
	if (b->code != OREG)
		ee_token(p->f, b->t, WRONG_BASE);
	o->rm = R_RSP; // sib
	if (is_rbp_addr(b) || is_r13_addr(b))
		o->mod = MOD_MEM_D8;
	if (is_r_new(b))
		o->rex |= REX_B;
	o->base = get_mem_reg(b->rm);
	free(b);
}
void set_rm_to_op(struct Oper *o, struct Oper *rm) {
	if (is_rsp_addr(rm) || is_r12_addr(rm)) {
		// REMEMBER: only R_RSI as SIB flag, esp is not allowed
		// mod = 00, rm = 100, base = 100, index = 100
		o->rm = R_RSP;	  // sib
		o->index = R_RSP; // unused
		o->base = R_RSP;
	} else
		o->rm = get_mem_reg(rm->rm);

	if (is_rbp_addr(rm) || is_r13_addr(rm))
		o->mod = MOD_MEM_D8;
	if (is_r_new(rm))
		o->rex |= REX_B;
	free(rm);
}

void get_moffs(struct Pser *p, struct Oper **o) {
	*o = expression(p);
	if ((*o)->code != OINT)
		ee_token(p->f, (*o)->t, WRONG_MOFFS);
	if ((*o)->sz != DWORD && (*o)->sz != QWORD)
		ee_token(p->f, (*o)->t, WRONG_MOFFS_MEM_SZ);

	(*o)->code = OMOFFS;
	(*o)->mem_sz = (*o)->sz;
	(*o)->sz = DWORD;
}

struct Oper *expression(struct Pser *p) {
	struct Oper *o = malloc(sizeof(struct Oper)), *otmp, *otmp2;
	o->disp_is_rel_flag = 0;
	o->disp = 0;
	o->mem_sz = 0;
	o->mod = MOD_REG;
	o->scale = SCALE_1;
	o->index = R_NONE;
	o->base = R_NONE;
	o->rm = R_NONE;
	o->rex = 0;
	o->forsed_sz = 0;
	o->sz = DWORD;

	struct PList *sib = new_plist(4);
	enum OCode code, *cp = &code;

	struct Token *ot, *t0 = next_get(p, -1); //, *t1, *t2;
	char *v;

	switch (t0->code) {
	case INT:
		set_tc(&t0, cp, t0, OINT, &o->sz, DWORD);
		ot = t0;
		break;
	case REAL:
		set_tc(&t0, cp, t0, OFPN, &o->sz, QWORD);
		ot = t0;
		break;
	case STR:
		if (t0->str->size == 1) {
			t0->number = (uint64_t)t0->str->st[0];
			o->sz = BYTE;
		} else if (t0->str->size == 2) {
			t0->number = *(uint16_t *)(t0->str->st);
			o->sz = WORD;
		} else
			ee_token(p->f, t0, INVALID_STR_LEN);
		code = OINT;
		ot = t0;
		break;
	case MINUS:
		t0 = next_get(p, -1);
		if (t0->code == INT) {
			t0->number *= -1;
			code = OINT;
		} else if (t0->code == REAL) {
			t0->fpn *= -1;
			code = OFPN;
		} else
			ee_token(p->f, t0, ERR_WRONG_MINUS);
		o->sz = DWORD;
		ot = t0;
		break;
	case ID:
		v = t0->view;
		ot = t0;
		if (search_reg(v, lenofarr(E_REGS), E_REGS, o, DWORD))
			;
		else if (search_reg(v, lenofarr(R_REGS), R_REGS, o, QWORD))
			;
		else if (search_reg(v, lenofarr(W_REGS), W_REGS, o, WORD))
			;
		else if (search_reg(v, lenofarr(B_REGS), B_REGS, o, BYTE))
			;
		else if (search_some_reg(v, lenofarr(XMM_REGS), XMM_REGS, o, OXMM,
								 XWORD))
			;
		else if (search_some_reg(v, lenofarr(SEG_REGS), SEG_REGS, o, OSREG,
								 WORD))
			;
		else if (search_some_reg(v, lenofarr(MM_REGS), MM_REGS, o, OMM, QWORD))
			;
		else if (search_size(p, &o, v) || search_defn(p, &o, v)) {
			plist_free(sib);
			return o; // its special
		} else if (sc(v, STR_ADDR)) {
			get_moffs(p, &o);
			plist_free(sib);
			return o; // not so special indeed
		}
		if (o->rm != R_NONE) {
			code = o->code; // OREG or OSREG for now and OSIMD later
			break;
		}
		code = OREL;
		o->sz = DWORD;
		break;
	case PAR_L:
		do {
			otmp = expression(p);
			if (otmp->code != OINT && otmp->code != OREL && otmp->code != OREG)
				ee_token(p->f, otmp->t, WRONG_ADDRES_OP);
			if (is_r8(otmp) || is_r16(otmp))
				ee_token(p->f, otmp->t, WRONG_ADDR_REG_SZ);
			if (otmp->code == OREG) {
				if (o->mem_sz) {
					if ((is_f_reg64(otmp->rm) && o->mem_sz == DWORD) ||
						(is_f_reg32(otmp->rm) && o->mem_sz == QWORD))
						ee_token(p->f, otmp->t, DISSERENT_SIZE_REGS);
				} else {
					if (is_f_reg32(otmp->rm))
						o->mem_sz = DWORD;
					else
						o->mem_sz = QWORD;
				}
			}
			plist_add(sib, otmp);
			t0 = gettp(p, 0);
		} while (t0->code != PAR_R);
		next_get(p, 0); // skip )

		// [r s r d] = 4 os max
		if (sib->size > 4 || sib->size == 0)
			ee_token(p->f, t0, TOO_MUCH_OS);
		ot = t0;
		code = OMEM;
		o->mod = MOD_MEM;

		// REMEMBER: ALL MEM REGS ARE RAX-RDI
		// OTHER SHOULD BE CONVERTED VIA FLAGS

		otmp = plist_get(sib, 0);
		if (sib->size == 1) {
			if (otmp->code == OREL || otmp->code == OINT) {
				// disp
				set_disp_to_op(p, o, otmp); // changes mod
				// REMEMBER: mod = 00, rm = 101 == [RIP+disp32]
				o->mod = MOD_MEM;
				o->rm = R_RBP;
			} else if (otmp->code == OREG) {
				// reg(rm)
				set_rm_to_op(o, otmp);
			} else
				ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);

		} else if (sib->size == 2) {
			if (otmp->code == OINT) {
				// sc reg(index)
				set_scale_to_op(p, o, otmp);
				set_index_to_op(p, o, plist_get(sib, 1));
				// mod = 00, rm = 100, base = 101 ==
				o->rm = R_RSP; // sib
				// no base register and
				o->base = R_RBP;
				// REMEMBER: do disp 32 although mod is 00
				// a 32-bit displacement
			} else if (otmp->code == OREG) {
				otmp2 = plist_get(sib, 1);
				if (otmp2->code == OREG) {
					// reg(base) reg(index) | sib
					set_base_to_op(p, o, otmp);
					set_index_to_op(p, o, otmp2);
				} else if (otmp2->code == OREL || otmp2->code == OINT) {
					// reg(rm) disp         | not sib
					set_rm_to_op(o, otmp);
					set_disp_to_op(p, o, otmp2); // changes mod to non 00
				} else
					ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);
			} else
				ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);

		} else if (sib->size == 3) {
			if (otmp->code == OREG) {
				set_base_to_op(p, o, otmp); // sets sib

				otmp2 = plist_get(sib, 1);
				if (otmp2->code == OREG) {
					// reg(base) reg(index) disp | sib
					set_index_to_op(p, o, otmp2);
					otmp2 = plist_get(sib, 2);
					set_disp_to_op(p, o, otmp2); // changes mod to non 00
				} else if (otmp2->code == OINT) {
					// reg(base) sc reg(index)   | sib
					set_scale_to_op(p, o, otmp2);
					otmp2 = plist_get(sib, 2);
					set_index_to_op(p, o, otmp2);
				} else
					ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);
			} else if (otmp->code == OINT) {
				o->rm = R_RSP; // sib
				// sc reg(index) disp            | sib
				set_scale_to_op(p, o, otmp);
				otmp = plist_get(sib, 1);
				set_index_to_op(p, o, otmp);
				// mod = 00, rm = 100, base = 101 no base register and disp32
				o->base = R_RBP;
				otmp = plist_get(sib, 2);
				// REMEMBER: here too need to write disp32
				set_disp_to_op(p, o, otmp); // changes mod
				o->mod = MOD_MEM;
			} else
				ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);

		} else if (otmp->code == OREG) { // size is 4
			// reg(base) sc reg(index) disp | sib
			set_base_to_op(p, o, otmp); // sets sib
			set_scale_to_op(p, o, plist_get(sib, 1));
			set_index_to_op(p, o, plist_get(sib, 2));
			set_disp_to_op(p, o, plist_get(sib, 3)); // changes mod
		} else
			ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);
		break;
	default:
		ee_token(p->f, t0, ERR_WRONG_TOKEN);
	};

	o->code = code;
	o->t = ot;
	plist_free(sib);
	// print_oper(o);
	return o;
}

enum ICode label_i(struct Pser *p, struct PList *os) {
	plist_add(os, gettp(p, 0));
	next_get(p, 0); // skip label
	next_get(p, 0); // skip :
	return ILABEL;
}

char *INVALID_SIZE_NOT_FOUND =
	"Ожидался размер операнда: <байт> <дбайт> <чбайт> <вбайт>.";
char *INVALID_SIZE_OF_FPN = "Неверный размер для числа с плавающей точкой, "
							"ожидался размер <чбайт> или <вбайт>.";
char *AWAITED_SLASHN = "Ожидался перевод строки.";
char *INVALID_DEFN_USAGE = "Неверное использование определения.";
char *DEFN_NOT_FOUND = "Определение не было найдено.";
char *INVALID_RESERVED_USAGE_BEGIN =
	"Нельзя начинать объявление переменной со слова \"запас\".";
char *INVALID_RESERVED_USAGE =
	"Нельзя так использовать слово \"запас\"."
	"\nПравильный синтаксис для использования слова \"запас\":"
	"\n\tзапас значение раз.";
char *EXPEXTED_INT_FOR_RESERVED_TIMES =
	"Правильный синтаксис для использования слова \"запас\":"
	"\n\tзапас значение раз.";
char *INVALID_INT_FOR_RESERVED_TIMES =
	"Число означающее количество раз значений не может быть меньше 1.";

enum ICode let_i(struct Pser *p, struct PList *os) {
	struct BList *data = new_blist(8);
	struct Token *c = next_get(p, 0), *name; // skip let word
	enum ICode code = ILET;
	long value;
	struct Defn *d;
	struct Oper *o;

	while (c->code == SLASH || c->code == SLASHN)
		c = next_get(p, 0);

	int size = is_size_word(c->view), old_sz;
	if (size > 0) // -1 is RESERVED
		code = IDATA;
	else {
		if (size < 0)
			ee_token(p->f, c, INVALID_RESERVED_USAGE_BEGIN);

		name = c;
		do
			c = next_get(p, 0);
		while (c->code == SLASH || c->code == SLASHN);
		size = is_size_word(c->view);
		if (!size)
			ee_token(p->f, c, INVALID_SIZE_NOT_FOUND);
		if (size == RESERVED)
			ee_token(p->f, c, INVALID_RESERVED_USAGE_BEGIN);
	}

	loop {
		c = next_get(p, 0);
		if (c->code == SLASH) {
			c = next_get(p, 0); // skip slash get \n
			if (c->code != SLASHN)
				ee_token(p->f, c, AWAITED_SLASHN);
			continue;
		}
		if (c->code == SEP) {
			next_get(p, 0); // skip separator
			break;
		}
		if (c->code == ID) {
			old_sz = size;
			size = is_size_word(c->view);
			if (size) {
				if (size == RESERVED) {
					size = old_sz;
					next_get(p, 0); // skip запас word

					o = expression(p);
					if (o->code != OINT)
						ee_token(p->f, c, INVALID_RESERVED_USAGE);
					value = o->t->number;

					o = expression(p);
					if (o->code != OINT)
						ee_token(p->f, c, EXPEXTED_INT_FOR_RESERVED_TIMES);

					blist_add_set(data, size, &value, o->t->number);
					p->pos--; // here expression eats next token so
				}
				continue;
			}
			size = old_sz;
			d = is_defn(p, c->view);
			if (!d)
				break; // break if ID is not size or defn
			o = d->value;
			if (o->code == OINT)
				blat(data, (uc *)&o->t->number, size);
			else if (o->code == OFPN) {
				c->fpn = o->t->fpn;
				goto let_i_real;
			} else
				ee_token(p->f, c, INVALID_DEFN_USAGE);
		} else if (c->code == INT)
			blat(data, (uc *)&c->number, size);
		else if (c->code == STR)
			blat(data, (uc *)c->str->st, c->str->size);
		else if (c->code == REAL) {
		let_i_real:
			if (size == QWORD)
				blat(data, (uc *)&c->fpn, QWORD);
			else if (size == DWORD) {
				float value = c->fpn;
				blat(data, (uc *)&value, DWORD);
			} else
				ee_token(p->f, c, INVALID_SIZE_OF_FPN);
		} else {
			if (p->debug) {
				if (code == ILET)
					printf("пусть %s будет %ld байт\n", name->view, data->size);
				else
					printf("пусть %ld байт\n", data->size);
			}
			break;
		}
	}

	if (code == ILET)
		plist_add(os, name);
	plist_add(os, data);
	return code;
}

// TODO: is ir possible to do better str search
enum ICode define_pd(struct Pser *p) {
	struct Defn *d;
	char *view = next_get(p, 0)->view;
	next_get(p, 0); // skip name token
	struct Oper *value = expression(p);

	for (long i = 0; i < p->ds->size; i++) {
		d = plist_get(p->ds, i);

		if (sc(view, d->view)) {
			// expression can be freed here cuz it doesnt holds pointers that
			// it mallocs, it only borrows them
			free(d->value);
			d->value = value;
			return INONE;
		}
	}

	d = malloc(sizeof(struct Defn));
	d->view = view;
	d->value = value;
	plist_add(p->ds, d);
	return INONE;
}

const char *const EXPECTED_POSITIVE_INT_FOR_ALIGN =
	"Для функции \"равнять\" в качестве единственного аргумента ожидалось "
	"положительное целое число.";

enum ICode align_pd(struct Pser *p, struct PList *os) {
	next_get(p, 0); // skip равнять

	struct Oper *o = expression(p);
	if (o->code != OINT || o->t->number < 0)
		ee(p->f, o->t->p, EXPECTED_POSITIVE_INT_FOR_ALIGN);
	plist_add(os, o);
	if (gettp(p, 0)->code == SLASHN)
		return IALIGN;

	o = expression(p);
	if (o->code != OINT || o->t->number < 0)
		ee(p->f, o->t->p, EXPECTED_POSITIVE_INT_FOR_ALIGN);
	plist_add(os, o);
	return IALIGN;
}

const char *const EXPECTED_STR_AS_INCLUDE_PATH =
	"В выраженнии \"влечь\" в качестве параметра ожидалась строка "
	"с путем к файлу.";

enum ICode include_pd(struct Pser *p, struct PList *os) {
	// TODO: make relative folder addressation
	struct Token *path = next_get(p, 0);
	if (path->code != STR)
		ee(p->f, path->p, EXPECTED_STR_AS_INCLUDE_PATH);

	next_get(p, 0);
	plist_add(os, path);
	return IINCLUDE;
}

int ops_i(struct Pser *p, struct PList *os, char *view, enum ICode *c) {
	size_t i;
	for (i = 0; i < lenofarr(VAR_OPS_WORDS); i++)
		if (sc(view, VAR_OPS_WORDS[i].view)) {
			struct Token *cur = next_get(p, 0);
			while (cur->code != SLASHN && cur->code != SEP && cur->code != EF) {
				plist_add(os, expression(p));
				cur = gettp(p, 0);
			}
			// this is still can be a wrong instruction because its just
			// wont have the right opsCode in cmnd so dont care here about it
			*c = VAR_OPS_WORDS[i].inst;
			return 1;
		}
	for (i = 0; i < lenofarr(TWO_OPS_WORDS); i++)
		if (sc(view, TWO_OPS_WORDS[i].view)) {
			*c = n_ops_i(p, 2, os, TWO_OPS_WORDS[i].inst);
			return 1;
		}
	for (i = 0; i < lenofarr(ZERO_OPS_WORDS); i++)
		if (sc(view, ZERO_OPS_WORDS[i].view)) {
			next_get(p, 0);
			*c = ZERO_OPS_WORDS[i].inst;
			return 1;
		}
	for (i = 0; i < lenofarr(ONE_OPS_WORDS); i++)
		if (sc(view, ONE_OPS_WORDS[i].view)) {
			*c = n_ops_i(p, 1, os, ONE_OPS_WORDS[i].inst);
			return 1;
		}
	for (i = 0; i < lenofarr(TRI_OPS_WORDS); i++)
		if (sc(view, TRI_OPS_WORDS[i].view)) {
			*c = n_ops_i(p, 3, os, TRI_OPS_WORDS[i].inst);
			return 1;
		}
	return 0;
}

struct Inst *get_inst(struct Pser *p) {
	struct PList *os = new_plist(4);
	struct Token *cur = gettp(p, 0), *n;
	while (cur->code == SLASHN || cur->code == SEP)
		cur = next_get(p, 0);
	char *cv = cur->view;
	enum ICode code;
	n = gettp(p, 1);

	// fill *os in funcs
	if (cur->code == EF)
		code = IEOI;
	else if (cur->code == ID) {
		if (ops_i(p, os, cv, &code))
			;
		else if (n->code == COLO)
			code = label_i(p, os);
		else if (sc(cv, STR_LET))
			code = let_i(p, os);
		else if (sc(cv, STR_DEFINE))
			code = define_pd(p);
		else if (sc(cv, STR_ALIGN))
			code = align_pd(p, os);
		else if (sc(cv, STR_INCLUDE))
			code = include_pd(p, os);
		else if (sc(cv, STR_SEG))
			code = seg_i(p, os);
		else if (sc(cv, STR_ENTRY))
			code = entry_i(p, os);
		else
			ee_token(p->f, cur, "НЕИЗВЕСТНАЯ КОМАНДА");
	} else if (cur->code == INC)
		code = n_ops_i(p, 1, os, IINC);
	else if (cur->code == DEC)
		code = n_ops_i(p, 1, os, IDEC);
	else
		ee_token(p->f, cur, "НЕИЗВЕСТНАЯ КОМАНДА");

	return new_inst(p, code, os, cur);
}

void include_in_is(struct Pser *p, struct PList *is, struct Inst *i) {
	struct Token *path = plist_get(i->os, 0);
	blist_add(path->str, 0); // string 0 terminator

	struct Pser *tmp_p = new_pser((char *)path->str->st, p->debug);
	plist_free(tmp_p->ds);
	tmp_p->ds = p->ds;
	struct PList *inc = pse(tmp_p);

	for (uint32_t j = 0; j < inc->size; j++)
		plist_add(is, plist_get(inc, j));

	free(tmp_p);
	free(inc);
}

struct PList *pse(struct Pser *p) {
	struct PList *is = new_plist(p->ts->cap_pace); // why not

	struct Inst *i = get_inst(p);
	while (i->code != IEOI) {
		if (i->code == IINCLUDE)
			include_in_is(p, is, i);
		else if (i->code != INONE)
			plist_add(is, i);
		i = get_inst(p);
	}
	plist_add(is, i);

	return is;
}
