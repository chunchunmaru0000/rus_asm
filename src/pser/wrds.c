#include "pser.h"

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
// not                  н       не
// overflow     и       избыток
// below                п       под
// above                в       выше
// equ                  р       равно
// carry                с       снести
// zero                 0       ноль
// sign                 з       знак
// parity_even  ч       чет
// parity_odd   нч      не чет
// less                 м       меньше
// greater              б       больше

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
// Single               о один
// Double               д два
// Scalar               с скалярный
// Un aligned   нр не ровный
// Packed               у упакованный
// Duplicate    удв удвоить
// High                 в высок
// Low                  н низ
// Unpack               рсп распаковать
// Convert              пре преобразовать
// Compare              срав сравнить
// Unordered    ну не упорядоченный
// Integers             ч число
// Non temp             нв не временный
// Truncate             о округлять
// Memory               память
// Round                окр
// Blend                смш
// Align                рвн
// Extract              извл
// Insert               вств
// Dot Product  спр скалярное произведение
// SAD                  сумар
// Multiple             м
// Explicit             я
// Return               в воздать
// Reciprocal   обратный
// Shuffle              тас тасовать
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

const uint32_t E_REGS_LEN = lenofarr(E_REGS);
const uint32_t R_REGS_LEN = lenofarr(R_REGS);
const uint32_t W_REGS_LEN = lenofarr(W_REGS);
const uint32_t B_REGS_LEN = lenofarr(B_REGS);
const uint32_t XMM_REGS_LEN = lenofarr(XMM_REGS);
const uint32_t SEG_REGS_LEN = lenofarr(SEG_REGS);
const uint32_t MM_REGS_LEN = lenofarr(MM_REGS);

const uint32_t VAR_OPS_WORDS_LEN = lenofarr(VAR_OPS_WORDS);
const uint32_t TWO_OPS_WORDS_LEN = lenofarr(TWO_OPS_WORDS);
const uint32_t ZERO_OPS_WORDS_LEN = lenofarr(ZERO_OPS_WORDS);
const uint32_t ONE_OPS_WORDS_LEN = lenofarr(ONE_OPS_WORDS);
const uint32_t TRI_OPS_WORDS_LEN = lenofarr(TRI_OPS_WORDS);
