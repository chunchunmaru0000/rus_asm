enum OCode { // operand type codes
	OINT,	 // integer numbers negative number are handled in tzer
	OFPN,	 // floating point numbers that just different for the parser
	OREL,	 // labels and vars relative offsets
	OREG,	 // general purpose registers
	OMEM,	 // [] size is size of entire op
	OXMM,	 // single instruction multiple data registers
	OSREG,	 // segment registers cs, ds, fs, gs
	OMOFFS,
	// OCR // control registers?
	// ODR // debug registers?
	// OIOR // i/o registers? are there even these
	OFPU, // FPU registers and things
	OMM,  // mm registers
};

enum RegCode {
	R_NONE,

	R_ES,
	R_CS,
	R_SS,
	R_DS,
	R_FS,
	R_GS,
	R_LDTR,
	R_TR,

	R_AL,
	R_CL,
	R_DL,
	R_BL,
	R_AH,
	R_CH,
	R_DH,
	R_BH,
	R_R8B,
	R_R9B,
	R_R10B,
	R_R11B,
	R_R12B,
	R_R13B,
	R_R14B,
	R_R15B,

	R_IP,
	R_FLAGS,
	R_AX,
	R_CX,
	R_DX,
	R_BX,
	R_SP,
	R_BP,
	R_SI,
	R_DI,
	R_R8W,
	R_R9W,
	R_R10W,
	R_R11W,
	R_R12W,
	R_R13W,
	R_R14W,
	R_R15W,

	R_EIP,
	R_EFLAGS,
	R_EAX,
	R_ECX,
	R_EDX,
	R_EBX,
	R_ESP,
	R_EBP,
	R_ESI,
	R_EDI,
	R_R8D,
	R_R9D,
	R_R10D,
	R_R11D,
	R_R12D,
	R_R13D,
	R_R14D,
	R_R15D,

	R_RIP,
	R_RFLAGS,
	R_RAX,
	R_RCX,
	R_RDX,
	R_RBX,
	R_RSP,
	R_RBP,
	R_RSI,
	R_RDI,
	R_R8,
	R_R9,
	R_R10,
	R_R11,
	R_R12,
	R_R13,
	R_R14,
	R_R15,

	R_XMM0,
	R_XMM1,
	R_XMM2,
	R_XMM3,
	R_XMM4,
	R_XMM5,
	R_XMM6,
	R_XMM7,
	R_XMM8,
	R_XMM9,
	R_XMM10,
	R_XMM11,
	R_XMM12,
	R_XMM13,
	R_XMM14,
	R_XMM15,

	R_MM0,
	R_MM1,
	R_MM2,
	R_MM3,
	R_MM4,
	R_MM5,
	R_MM6,
	R_MM7,
};

enum ICode {
	// directives
	INONE,
	IENTRY,
	ISEGMENT,
	IEOI, // end of instructions
	ILABEL,
	ILET,
	IINCLUDE,
	IALIGN,
	IDATA,
	// two ops
	IADD,
	IOR,
	IADC,
	ISBB,
	IAND,
	ISUB,
	IXOR,
	ICMP,
	ITEST,
	IMOV, // IMOVSXD, its just mov here
	IMUL,
	IIMUL,
	IDIV,
	IIDIV,
	IXCHG,
	ILEA,
	IROL,
	IROR,
	IRCL,
	IRCR,
	ISHL,
	ISHR,
	ISAR,
	IINPUT,
	IOUTPUT,
	IMOVNTI,

	ICMOVO,
	ICMOVNO,
	ICMOVB,
	ICMOVNB,
	ICMOVE,
	ICMOVNE,
	ICMOVBE,
	ICMOVA,
	ICMOVS,
	ICMOVNS,
	ICMOVP,
	ICMOVNP,
	ICMOVL,
	ICMOVNL,
	ICMOVLE,
	ICMOVG,
	// one op
	ISETO,
	ISETNO,
	ISETB,
	ISETNB,
	ISETE,
	ISETNE,
	ISETBE,
	ISETA,
	ISETS,
	ISETNS,
	ISETP,
	ISETNP,
	ISETL,
	ISETNL,
	ISETLE,
	ISETG,

	IROL1,
	IROR1,
	IRCL1,
	IRCR1,
	ISHL1,
	ISHR1,
	ISAR1,
	ICALL,
	IPUSH,
	IPOP,
	IINT,
	IINC,
	IDEC,
	ICALLF,
	IJMPF,

	IJMP,
	IJO,
	IJNO,
	IJB,
	IJNB,
	IJE,
	IJNE,
	IJBE,
	IJA,
	IJS,
	IJNS,
	IJP,
	IJNP,
	IJL,
	IJNL,
	IJLE,
	IJG,

	ILOOPNZ,
	ILOOPZ,
	ILOOP,
	IJECXZ,
	IJRCXZ,
	INEG,
	INOT,
	// zero ops
	IUSEFS,
	IUSEGS,
	IREPNZ,
	IREP,
	ICLC,
	ISTC,
	ICLI,
	ISTI,
	ICLD,
	ISTD,
	IINT3,
	IINT1,
	IHLT,
	ICMC,
	ISYSCALL,
	INOP,
	IRET,
	IRETF,
	IRETFQ,
	ILEAVE,
	IENTER,
	ILOCK,
	IINSB,
	IINSW,
	IINSD,
	IOUTSB,
	IOUTSW,
	IOUTSD,
	IPAUSE,
	IWAIT,
	ICBW,
	ICWDE,
	ICDQE,
	ICWD,
	ICDQ,
	ICQO,
	IPUSHF,
	IPOPF,
	ISAHF,
	ILAHF,
	IMOVSB,
	IMOVSW,
	IMOVSD,
	IMOVSQ,
	ICMPSB,
	ICMPSW,
	ICMPSD,
	ICMPSQ,
	ISCASB,
	ISCASW,
	ISCASD,
	ISCASQ,
	ILODSB,
	ILODSW,
	ILODSD,
	ILODSQ,
	ISTOSB,
	ISTOSW,
	ISTOSD,
	ISTOSQ,
	// all not implemented zero ops after 0x0f
	// IVMCALL,
	// IVMLAUNCH,
	// IVMRESUME,
	// IVMXOFF,
	// IXGETBV,
	// IXSETBV,
	// ISWAPGS,
	// IRDTSCP,
	// ICLTS,
	// ISYSRET,
	// IINVD,
	// IWBINVD,
	// IUD2,
	// IWRMSR,
	// IRDTSC,
	// IRDMSR,
	// IRDPMC,
	// ISYSENTER,
	// ISYSEXIT,
	// IGETSEC,
	// IEMMS,
	// ICPUID,
	// IRSM,
	// ILFENCE,
	// IMFENCE,
	// ISFENCE,
	// xmm
	I_XMM_INSTRUCTIONS_BEGIN,
	// __
	I_XMM_NO_PREFIX_BEGIN,

	IMOVUPS,
	IMOVHLPS,
	IMOVLPS,
	IUNPCKLPS,
	IUNPCKHPS,
	IMOVLHPS,
	IMOVHPS,
	IMOVAPS,
	ICVTPI2PS,
	IMOVNTPS,
	ICVTTPS2PI,
	ICVTPS2PI,
	IUCOMISS,
	ICOMISS,
	IPALIGNR_MM,
	IMOVMSKPS,
	ISQRTPS,
	IRSQRTPS,
	IRCPPS,
	IANDPS,
	IANDNPS,
	IORPS,
	IXORPS,
	IADDPS,
	IMULPS,
	ICVTPS2PD,
	ICVTDQ2PS,
	ISUBPS,
	IMINPS,
	IDIVPS,
	IMAXPS,
	ISHUFPS,
	ICMPPS,

	I_XMM_NO_PREFIX_END,
	// 66
	I_XMM_66_PREFIX_BEGIN,

	IMOVUPD,
	IMOVLPD,
	IUNPCKLPD,
	IUNPCKHPD,
	IMOVHPD,
	IUCOMISD,
	ICOMISD,
	IMOVNTPD,
	IMOVAPD,
	ICVTPI2PD,
	ICVTTPD2PI,
	ICVTPD2PI,
	IROUNDPS,
	IROUNDPD,
	IROUNDSS,
	IROUNDSD,
	IBLENDPS,
	IBLENDPD,
	IPBLENDW,
	IPALIGNR,
	IPEXTRB,
	IPEXTRW,
	IPEXTRDQ,
	IEXTRACTPS,
	IPINSRB,
	IINSERTPS,
	IPINSRDQ,
	IDPPS,
	IDPPD,
	IMPSADBW,
	IPCMPESTRM,
	IPCMPESTRI,
	IPCMPISTRM,
	IPCMPISTRI,
	IPSHUFD,
	IMOVMSKPD,
	ISQRTPD,
	IANDPD,
	IANDNPD,
	IORPD,
	IXORPD,
	IADDPD,
	IMULPD,
	ICVTPD2PS,
	ICVTPS2DQ,
	ISUBPD,
	IMINPD,
	IDIVPD,
	IMAXPD,
	IPUNPCKLBW,
	IPUNPCKLWD,
	IPUNPCKLDQ,
	IPACKSSWB,
	IPCMPGTB,
	IPCMPGTW,
	IPCMPGTD,
	IPACKUSWB,
	IPUNPCKHBW,
	IPUNPCKHWD,
	IPUNPCKHDQ,
	IPACKSSDW,
	IPUNPCKLQDQ,
	IPUNPCKHQDQ,
	IMOVDQA,
	IPSRLW,
	IPSRAW,
	IPSLLW,
	IPSRLD,
	IPSRAD,
	IPSLLD,
	IPSRLQ,
	IPSRLDQ,
	IPSLLQ,
	IPSLLDQ,
	IPCMPEQB,
	IPCMPEQW,
	IPCMPEQD,
	IHADDPD,
	IHSUBPD,
	IMOVNTDQ,
	IPSUBSB,
	IPSUBSW,
	IPMINSW,
	IPOR,
	IPADDSB,
	IPADDSW,
	IPMAXSW,
	IPXOR,
	IPMULUDQ,
	IPMADDWD,
	IPSADBW,
	IMASKMOVDQU,
	IPSUBB,
	IPSUBW,
	IPSUBD,
	IPSUBQ,
	IPADDB,
	IPADDW,
	IPADDD,
	ICVTTPD2DQ,
	IADDSUBPD,
	IPADDQ,
	IPMULLW,
	IPMOVMSKB,
	IPSUBUSB,
	IPSUBUSW,
	IPMINUB,
	IPAND,
	IPADDUSB,
	IPADDUSW,
	IPMAXUB,
	IPANDN,
	IPAVGB,
	IPAVGW,
	IPMULHUW,
	IPMULHW,
	ISHUFPD,
	ICMPPD,
	IPINSRW,

	I_XMM_66_PREFIX_END,
	// F2
	I_XMM_F2_PREFIX_BEGIN,

	IMOVSD_XMM,
	IMOVDDUP,
	ICVTSI2SD,
	ICVTTSD2SI,
	ICVTSD2SI,
	IPSHUFLW,
	ISQRTSD,
	IADDSD,
	IMULSD,
	ICVTSD2SS,
	ISUBSD,
	IMINSD,
	IDIVSD,
	IMAXSD,
	IHADDPS,
	IHSUBPS,
	ILDDQU,
	ICVTPD2DQ,
	IADDSUBPS,
	ICMPSD_XMM,

	I_XMM_F2_PREFIX_END,
	// F3
	I_XMM_F3_PREFIX_BEGIN,

	IMOVSS,
	IMOVSLDUP,
	IMOVSHDUP,
	ICVTSI2SS,
	ICVTTSS2SI,
	ICVTSS2SI,
	IPSHUFHW,
	ISQRTSS,
	IRSQRTSS,
	IRCPSS,
	IADDSS,
	IMULSS,
	ICVTSS2SD,
	ICVTTPS2DQ,
	ISUBSS,
	IMINSS,
	IDIVSS,
	IMAXSS,
	IMOVDQU,
	ICVTDQ2PD,
	ICMPSS,

	I_XMM_F3_PREFIX_END,
	IMOV_XMM,
	// ############ 0f 50 - 0F 7F
	// IPUNPCKLBW_MM,
	// IPUNPCKLWD_MM,
	// IPUNPCKLDQ_MM,
	// IPACKSSWB_MM,
	// IPCMPGTB_MM,
	// IPCMPGTW_MM,
	// IPCMPGTD_MM,
	// IPACKUSWB_MM,
	// IPUNPCKHBW_MM,
	// IPUNPCKHWD_MM,
	// IPUNPCKHDQ_MM,
	// IPACKSSDW_MM,
	// IPSRLW_MM,
	// IPSRAW_MM,
	// IPSLLW_MM,
	// IPSRLD_MM,
	// IPSRAD_MM,
	// IPSLLD_MM,
	// IPSRLQ_MM,
	// IPSLLQ_MM,
	// IPCMPEQB_MM,
	// IPCMPEQW_MM,
	// IPCMPEQD_MM,
	// IPSHUFW, // also uses only MM
	// ############ 0F C2 - 0F FE
	// IPINSRW_MM,
	// IPEXTRW_MM,
	// IPEXTRW,
	// IPSRLW_MM,
	// IPSRLD_MM,
	// IPSRLQ_MM,
	// IPADDQ_MM,
	// IPMULLW_MM,
	// IPMOVMSKB_MM,
	// IPSUBUSB_MM,
	// IPSUBUSW_MM,
	//      IPMINUB_MM,
	//      IPAND_MM,
	//      IPADDUSB_MM,
	//      IPADDUSW_MM,
	//      IPMAXUB_MM,
	//      IPANDN_MM,
	//      IPAVGB_MM,
	//      IPSRAW_MM,
	//      IPSRAD_MM,
	//      IPAVGW_MM,
	//      IPMULHUW_MM,
	//      IPMULHW_MM,
	//      IMOVNTQ_MM,
	//      IPSUBSB_MM,
	//      IPSUBSW_MM,
	//      IPMINSW_MM,
	//      IPOR_MM,
	//      IPADDSB_MM,
	//      IPADDSW_MM,
	//      IPMAXSW_MM,
	//      IPXOR_MM,
	//      IPSLLW_MM,
	//      IPSLLD_MM,
	//      IPSLLQ_MM,
	//      IPMULUDQ_MM,
	//      IPMADDWD_MM,
	//      IPSADBW_MM,
	//      IMASKMOVQ_MM,
	//      IPSUBB_MM,
	//      IPSUBW_MM,
	//      IPSUBD_MM,
	//      IPSUBQ_MM,
	//      IPADDB_MM,
	//      IPADDW_MM,
	//      IPADDD_MM,
	// IMOVQ2DQ,
	// IMOVDQ2Q,
};
