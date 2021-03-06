/*
 *	term.c
 *
 *	terminal module
 */

#include "headers.h"
#include "depend.h"
#include "printf.h"
#include "kctype.h"
#include "typesize.h"
#include "string.h"
#include "termio.h"
#include "term.h"

#ifdef	USETERMINFO
#include <curses.h>
#include <term.h>
#endif

#if	MSDOS
# ifdef	PC98
# define	NOTUSEBIOS
# else
# define	USEVIDEOBIOS
# define	VIDEOBIOS	0x10
# define	BIOSSEG		0x40
# define	CURRPAGE	0x62
# define	SCRHEIGHT	0x84
# define	SCRWIDTH	0x4a
# endif
# ifdef	DJGPP
# define	__term_regs	__dpmi_regs
# define	intdos2(rp)	__dpmi_int(0x21, rp)
# define	getkeybuf(o)	_farpeekw(_dos_ds, KEYBUFWORKSEG * 0x10 + o)
# define	putkeybuf(o, n)	_farpokew(_dos_ds, KEYBUFWORKSEG * 0x10 + o, n)
# define	intbios(v, rp)	__dpmi_int(v, rp)
# define	getbiosbyte(o)	_farpeekb(_dos_ds, BIOSSEG * 0x10 + o)
# else	/* !DJGPP */
typedef union REGS	__term_regs;
# define	intdos2(rp)	int86(0x21, rp, rp)
# define	getkeybuf(o)	(*((u_short far *)MK_FP(KEYBUFWORKSEG, o)))
# define	putkeybuf(o, n)	(*((u_short far *)MK_FP(KEYBUFWORKSEG, o)) = n)
# define	intbios(v, rp)	int86(v, rp, rp)
# define	getbiosbyte(o)	(*((u_char far *)MK_FP(BIOSSEG, o)))
#  ifdef	LSI_C
static int _asm_sti __P_((char *));
static int _asm_cli __P_((char *));
#  define	enable()	_asm_sti("\n\tsti\n")
#  define	disable()	_asm_cli("\n\tcli\n")
#  endif
# endif	/* !DJGPP */
#else	/* !MSDOS */
typedef struct _kstree_t {
	u_char key;
	u_char num;
	struct _kstree_t *next;
} kstree_t;
#endif	/* !MSDOS */

#define	WINTERMNAME		"iris"
#define	CYGTERMNAME		"cygwin"
#ifdef	_POSIX_VDISABLE
#define	K_UNDEF			_POSIX_VDISABLE
#else
#define	K_UNDEF			0xff
#endif

#if	defined (USETERMINFO) && defined (SVR4)
typedef char			tputs_t;
#else
typedef int			tputs_t;
#endif

#ifdef	USETERMINFO
#define	tgetnum2(s)		(s)
#define	tgetflag2(s)		(s)
#define	TERM_pc			pad_char
#define	TERM_bc			NULL
#define	TERM_co			columns
#define	TERM_li			lines
#define	TERM_xn			eat_newline_glitch
#define	TERM_xs			ceol_standout_glitch
#define	TERM_gn			generic_type
#define	TERM_ti			enter_ca_mode
#define	TERM_te			exit_ca_mode
#define	TERM_mm			meta_on
#define	TERM_mo			meta_off
#define	TERM_cs			change_scroll_region
#define	TERM_ks			keypad_xmit
#define	TERM_ke			keypad_local
#define	TERM_ve			cursor_normal
#define	TERM_vs			cursor_visible
#define	TERM_vi			cursor_invisible
#define	TERM_sc			save_cursor
#define	TERM_rc			restore_cursor
#define	TERM_bl			bell
#define	TERM_vb			flash_screen
#define	TERM_cl			clear_screen
#define	TERM_me			exit_attribute_mode
#define	TERM_md			enter_bold_mode
#define	TERM_mr			enter_reverse_mode
#define	TERM_mh			enter_dim_mode
#define	TERM_mb			enter_blink_mode
#define	TERM_so			enter_standout_mode
#define	TERM_us			enter_underline_mode
#define	TERM_se			exit_standout_mode
#define	TERM_ue			exit_underline_mode
#define	TERM_ce			clr_eol
#define	TERM_al			insert_line
#define	TERM_dl			delete_line
#define	TERM_ic			insert_character
#define	TERM_IC			parm_ich
#define	TERM_dc			delete_character
#define	TERM_DC			parm_dch
#define	TERM_cm			cursor_address
#define	TERM_ho			cursor_home
#define	TERM_cr			carriage_return
#define	TERM_nl			NULL
#define	TERM_sf			scroll_forward
#define	TERM_sr			scroll_reverse
#define	TERM_up			cursor_up
#define	TERM_UP			parm_up_cursor
#define	TERM_do			cursor_down
#define	TERM_DO			parm_down_cursor
#define	TERM_nd			cursor_right
#define	TERM_RI			parm_right_cursor
#define	TERM_le			cursor_left
#define	TERM_LE			parm_left_cursor
#define	TERM_AF			set_a_foreground
#define	TERM_AB			set_a_background
#define	TERM_Sf			set_foreground
#define	TERM_Sb			set_background
#define	TERM_cb			clr_bol
#define	TERM_as			enter_alt_charset_mode
#define	TERM_ae			exit_alt_charset_mode
#define	TERM_ku			key_up
#define	TERM_kd			key_down
#define	TERM_kr			key_right
#define	TERM_kl			key_left
#define	TERM_kh			key_home
#define	TERM_kb			key_backspace
#define	TERM_l1			lab_f1
#define	TERM_l2			lab_f2
#define	TERM_l3			lab_f3
#define	TERM_l4			lab_f4
#define	TERM_l5			lab_f5
#define	TERM_l6			lab_f6
#define	TERM_l7			lab_f7
#define	TERM_l8			lab_f8
#define	TERM_l9			lab_f9
#define	TERM_la			lab_f10
#define	TERM_F1			key_f11
#define	TERM_F2			key_f12
#define	TERM_F3			key_f13
#define	TERM_F4			key_f14
#define	TERM_F5			key_f15
#define	TERM_F6			key_f16
#define	TERM_F7			key_f17
#define	TERM_F8			key_f18
#define	TERM_F9			key_f19
#define	TERM_FA			key_f20
#define	TERM_k1			key_f1
#define	TERM_k2			key_f2
#define	TERM_k3			key_f3
#define	TERM_k4			key_f4
#define	TERM_k5			key_f5
#define	TERM_k6			key_f6
#define	TERM_k7			key_f7
#define	TERM_k8			key_f8
#define	TERM_k9			key_f9
#define	TERM_k10		key_f10
#define	TERM_k0			key_f0
#define	TERM_kL			key_dl
#define	TERM_kA			key_il
#define	TERM_kD			key_dc
#define	TERM_kI			key_ic
#define	TERM_kC			key_clear
#define	TERM_kE			key_eol
#define	TERM_kP			key_ppage
#define	TERM_kN			key_npage
#define	TERM_at8		key_enter
#define	TERM_at1		key_beg
#define	TERM_at7		key_end
#else	/* !USETERMINFO */
#define	tgetnum2		tgetnum
#define	tgetflag2		tgetflag
#define	TERM_pc			"pc"
#define	TERM_bc			"bc"
#define	TERM_co			"co"
#define	TERM_li			"li"
#define	TERM_xn			"xn"
#define	TERM_xs			"xs"
#define	TERM_gn			"gn"
#define	TERM_ti			"ti"
#define	TERM_te			"te"
#define	TERM_mm			"mm"
#define	TERM_mo			"mo"
#define	TERM_cs			"cs"
#define	TERM_ks			"ks"
#define	TERM_ke			"ke"
#define	TERM_ve			"ve"
#define	TERM_vs			"vs"
#define	TERM_vi			"vi"
#define	TERM_sc			"sc"
#define	TERM_rc			"rc"
#define	TERM_bl			"bl"
#define	TERM_vb			"vb"
#define	TERM_cl			"cl"
#define	TERM_me			"me"
#define	TERM_md			"md"
#define	TERM_mr			"mr"
#define	TERM_mh			"mh"
#define	TERM_mb			"mb"
#define	TERM_so			"so"
#define	TERM_us			"us"
#define	TERM_se			"se"
#define	TERM_ue			"ue"
#define	TERM_ce			"ce"
#define	TERM_al			"al"
#define	TERM_dl			"dl"
#define	TERM_ic			"ic"
#define	TERM_IC			"IC"
#define	TERM_dc			"dc"
#define	TERM_DC			"DC"
#define	TERM_cm			"cm"
#define	TERM_ho			"ho"
#define	TERM_cr			"cr"
#define	TERM_nl			"nl"
#define	TERM_sf			"sf"
#define	TERM_sr			"sr"
#define	TERM_up			"up"
#define	TERM_UP			"UP"
#define	TERM_do			"do"
#define	TERM_DO			"DO"
#define	TERM_nd			"nd"
#define	TERM_RI			"RI"
#define	TERM_le			"le"
#define	TERM_LE			"LE"
#define	TERM_AF			"AF"
#define	TERM_AB			"AB"
#define	TERM_Sf			"Sf"
#define	TERM_Sb			"Sb"
#define	TERM_cb			"cb"
#define	TERM_as			"as"
#define	TERM_ae			"ae"
#define	TERM_ku			"ku"
#define	TERM_kd			"kd"
#define	TERM_kr			"kr"
#define	TERM_kl			"kl"
#define	TERM_kh			"kh"
#define	TERM_kb			"kb"
#define	TERM_l1			"l1"
#define	TERM_l2			"l2"
#define	TERM_l3			"l3"
#define	TERM_l4			"l4"
#define	TERM_l5			"l5"
#define	TERM_l6			"l6"
#define	TERM_l7			"l7"
#define	TERM_l8			"l8"
#define	TERM_l9			"l9"
#define	TERM_la			"la"
#define	TERM_F1			"F1"
#define	TERM_F2			"F2"
#define	TERM_F3			"F3"
#define	TERM_F4			"F4"
#define	TERM_F5			"F5"
#define	TERM_F6			"F6"
#define	TERM_F7			"F7"
#define	TERM_F8			"F8"
#define	TERM_F9			"F9"
#define	TERM_FA			"FA"
#define	TERM_k1			"k1"
#define	TERM_k2			"k2"
#define	TERM_k3			"k3"
#define	TERM_k4			"k4"
#define	TERM_k5			"k5"
#define	TERM_k6			"k6"
#define	TERM_k7			"k7"
#define	TERM_k8			"k8"
#define	TERM_k9			"k9"
#define	TERM_k10		"k;"
#define	TERM_k0			"k0"
#define	TERM_kL			"kL"
#define	TERM_kA			"kA"
#define	TERM_kD			"kD"
#define	TERM_kI			"kI"
#define	TERM_kC			"kC"
#define	TERM_kE			"kE"
#define	TERM_kP			"kP"
#define	TERM_kN			"kN"
#define	TERM_at8		"@8"
#define	TERM_at1		"@1"
#define	TERM_at7		"@7"
#endif	/* !USETERMINFO */

#define	TERMCAPSIZE		2048

#ifndef	FREAD
# ifdef	_FREAD
# define	FREAD		_FREAD
# else
# define	FREAD		(O_RDONLY + 1)
# endif
#endif

#if	!MSDOS && !defined (USETERMINFO)
extern int tgetent __P_((char *, CONST char *));
extern int tgetnum __P_((CONST char *));
extern int tgetflag __P_((CONST char *));
extern char *tgetstr __P_((CONST char *, char **));
extern int tputs __P_((CONST char *, int, int (*)__P_((tputs_t))));
#endif
#ifdef	DEBUG
extern VOID muntrace __P_ ((VOID_A));
extern char *_mtrace_file;
#endif

#if	!MSDOS
static int NEAR chkctrl __P_((int));
static int NEAR getctrl __P_((CONST char *, CONST char *));
# ifdef	USESGTTY
static VOID NEAR ttymode __P_((int, int, int, int));
# else
static VOID NEAR ttymode __P_((int, int, int, int, int, int, int, int));
# endif
#endif	/* !MSDOS */
static VOID NEAR terror __P_((CONST char *));
#if	!MSDOS
static char *NEAR tstrdup __P_((CONST char *));
#endif
static VOID NEAR defaultterm __P_((VOID_A));
#if	!MSDOS || defined (PC98) || defined (NOTUSEBIOS)
static int NEAR Xungetch __P_((int));
#endif
static int NEAR maxlocate __P_((int *, int *));
#if	!MSDOS || !defined (USEVIDEOBIOS)
static int NEAR pollterm __P_((int));
#endif
static char *NEAR tparamstr __P_((CONST char *, int, int));
#if	!MSDOS
static char *NEAR tgetstr2 __P_((char **, CONST char *));
static char *NEAR tgetstr3 __P_((char **, CONST char *, CONST char *));
static char *NEAR tgetkeyseq __P_((int, CONST char *));
static kstree_t *NEAR newkeyseqtree __P_((kstree_t *, int));
static VOID NEAR freekeyseqtree __P_((kstree_t *, int));
static int cmpkeyseq __P_((CONST VOID_P, CONST VOID_P));
static VOID NEAR sortkeyseq __P_((VOID_A));
#endif
#if	MSDOS
# ifdef	USEVIDEOBIOS
static VOID NEAR bioslocate __P_((int, int));
static VOID NEAR biosscroll __P_((int, int, int, int, int));
static VOID NEAR biosputch __P_((int, int));
static VOID NEAR bioscurstype __P_((int));
static VOID NEAR chgattr __P_((int));
static int NEAR evalcsi __P_((CONST char *));
# endif
# if	!defined (DJGPP) || defined (NOTUSEBIOS) || defined (PC98)
static VOID NEAR dosgettime __P_((u_char []));
# endif
#else	/* !MSDOS */
static int putch2 __P_((tputs_t));
static kstree_t *NEAR searchkeyseq __P_((int, kstree_t *, int));
#endif	/* !MSDOS */

#if	!MSDOS && !defined (USETERMINFO)
# ifdef	NOTERMVAR
# define	T_EXTERN
# else
# define	T_EXTERN	extern
# endif
T_EXTERN short ospeed;
T_EXTERN char PC;
T_EXTERN char *BC;
T_EXTERN char *UP;
#endif
int n_column = -1;
int n_lastcolumn = -1;
int n_line = -1;
int stable_standout = 0;
#if	MSDOS
CONST char *termstr[MAXTERMSTR];
#else
char *termstr[MAXTERMSTR];
#endif
u_char cc_intr = K_CTRL('C');
u_char cc_quit = K_CTRL('\\');
#if	MSDOS
u_char cc_eof = K_CTRL('Z');
#else
u_char cc_eof = K_CTRL('D');
#endif
u_char cc_eol = K_UNDEF;
u_char cc_erase = K_CTRL('H');
int (*keywaitfunc)__P_((VOID_A)) = NULL;
#if	!MSDOS
int usegetcursor = 0;
int suspended = 0;
char *duptty[2] = {NULL, NULL};
#endif
int ttyio = -1;
int isttyiomode = 0;
XFILE *ttyout = NULL;
int dumbterm = 0;

#if	MSDOS
# ifdef	PC98
#define	KEYBUFWORKSEG		0x00
#define	KEYBUFWORKSIZ		0x20
#define	KEYBUFWORKMIN		0x502
#define	KEYBUFWORKMAX		0x522
#define	KEYBUFWORKTOP		0x524
#define	KEYBUFWORKEND		0x526
#define	KEYBUFWORKCNT		0x528
static CONST u_char specialkey[] =
	"\032:=<;89>\25667bcdefghijk\202\203\204\205\206\207\210\211\212\213?";
static CONST u_char metakey[] =
	"\035-+\037\022 !\"\027#$%/.\030\031\020\023\036\024\026,\021*\025)";
# else	/* !PC98 */
#define	KEYBUFWORKSEG		0x40
#define	KEYBUFWORKSIZ		0x20
#define	KEYBUFWORKMIN		getkeybuf(0x80)
#define	KEYBUFWORKMAX		(KEYBUFWORKMIN + KEYBUFWORKSIZ)
#define	KEYBUFWORKTOP		(KEYBUFWORKMIN - 4)
#define	KEYBUFWORKEND		(KEYBUFWORKMIN - 2)
static CONST u_char specialkey[] = "\003HPMKRSGOIQ;<=>?@ABCDTUVWXYZ[\\]\206";
static CONST u_char metakey[] =
	"\0360. \022!\"#\027$%&21\030\031\020\023\037\024\026/\021-\025,";
# endif	/* !PC98 */
# ifdef	NOTUSEBIOS
static u_short keybuftop = 0;
# endif
# ifdef	USEVIDEOBIOS
static u_short videoattr = 0x07;
# endif
static CONST int specialkeycode[] = {
	0,
	K_UP, K_DOWN, K_RIGHT, K_LEFT,
	K_IC, K_DC, K_HOME, K_END, K_PPAGE, K_NPAGE,
	K_F(1), K_F(2), K_F(3), K_F(4), K_F(5),
	K_F(6), K_F(7), K_F(8), K_F(9), K_F(10),
	K_F(11), K_F(12), K_F(13), K_F(14), K_F(15),
	K_F(16), K_F(17), K_F(18), K_F(19), K_F(20), K_HELP
};
#define	SPECIALKEYSIZ		arraysize(specialkeycode)
#else	/* !MSDOS */
static CONST char *dumblist[] = {"dumb", "un", "unknown"};
#define	DUMBLISTSIZE		arraysize(dumblist)
static keyseq_t keyseq[K_MAX - K_MIN + 1];
static kstree_t *keyseqtree = NULL;
static CONST char *defkeyseq[K_MAX - K_MIN + 1] = {
	NULL,			/* K_NOKEY */
	"\033OB",		/* K_DOWN */
	"\033OA",		/* K_UP */
	"\033OD",		/* K_LEFT */
	"\033OC",		/* K_RIGHT */
	"\033[1~",		/* K_HOME */
	"\b",			/* K_BS */
	NULL,			/* K_F0 */
	"\033[11~",		/* K_F(1) */
	"\033[12~",		/* K_F(2) */
	"\033[13~",		/* K_F(3) */
	"\033[14~",		/* K_F(4) */
	"\033[15~",		/* K_F(5) */
	"\033[17~",		/* K_F(6) */
	"\033[18~",		/* K_F(7) */
	"\033[19~",		/* K_F(8) */
	"\033[20~",		/* K_F(9) */
	"\033[21~",		/* K_F(10) */
	"\033[23~",		/* K_F(11) */
	"\033[24~",		/* K_F(12) */
	"\033[25~",		/* K_F(13) */
	"\033[26~",		/* K_F(14) */
	"\033[28~",		/* K_F(15) */
	"\033[29~",		/* K_F(16) */
	"\033[31~",		/* K_F(17) */
	"\033[32~",		/* K_F(18) */
	"\033[33~",		/* K_F(19) */
	"\033[34~",		/* K_F(20) */
	"\033OP",		/* K_F(21) */
	"\033OQ",		/* K_F(22) */
	"\033OR",		/* K_F(23) */
	"\033OS",		/* K_F(24) */
	"\033OT",		/* K_F(25) */
	"\033OU",		/* K_F(26) */
	"\033OV",		/* K_F(27) */
	"\033OW",		/* K_F(28) */
	"\033OX",		/* K_F(29) */
	"\033OY",		/* K_F(30) */

	NULL,			/* 0447 */
	NULL,			/* 0450 */
	NULL,			/* 0451 */
	NULL,			/* 0452 */
	NULL,			/* 0453 */
	NULL,			/* 0454 */
	NULL,			/* 0455 */
	NULL,			/* 0456 */
	NULL,			/* 0457 */
	NULL,			/* 0460 */
	NULL,			/* 0461 */

	"\033Oj",		/* K_F('*') */
	"\033Ok",		/* K_F('+') */
	"\033Ol",		/* K_F(',') */
	"\033Om",		/* K_F('-') */
	"\033On",		/* K_F('.') */
	"\033Oo",		/* K_F('/') */
	"\033Op",		/* K_F('0') */
	"\033Oq",		/* K_F('1') */
	"\033Or",		/* K_F('2') */
	"\033Os",		/* K_F('3') */
	"\033Ot",		/* K_F('4') */
	"\033Ou",		/* K_F('5') */
	"\033Ov",		/* K_F('6') */
	"\033Ow",		/* K_F('7') */
	"\033Ox",		/* K_F('8') */
	"\033Oy",		/* K_F('9') */

	NULL,			/* 0502 */
	NULL,			/* 0503 */
	NULL,			/* 0504 */

	"\033OX",		/* K_F('=') */

	NULL,			/* 0506 */

	"\033OM",		/* K_F('?') */

	"",			/* K_DL */
	"",			/* K_IL */
	"\177",			/* K_DC */
	"\033[2~",		/* K_IC */
	NULL,			/* K_EIC */
	"",			/* K_CLR */
	NULL,			/* K_EOS */
	NULL,			/* K_EOL */
	NULL,			/* K_ESF */
	NULL,			/* K_ESR */
	"\033[6~",		/* K_NPAGE */
	"\033[5~",		/* K_PPAGE */
	NULL,			/* K_STAB */
	NULL,			/* K_CTAB */
	NULL,			/* K_CATAB */
	"\033[9~",		/* K_ENTER */
	NULL,			/* K_SRST */
	NULL,			/* K_RST */
	NULL,			/* K_PRINT */
	NULL,			/* K_LL */
	NULL,			/* K_A1 */
	NULL,			/* K_A3 */
	NULL,			/* K_B2 */
	NULL,			/* K_C1 */
	NULL,			/* K_C3 */
	NULL,			/* K_BTAB */
	NULL,			/* K_BEG */
	NULL,			/* K_CANC */
	NULL,			/* K_CLOSE */
	NULL,			/* K_COMM */
	NULL,			/* K_COPY */
	NULL,			/* K_CREAT */
	"\033[4~",		/* K_END */
	NULL,			/* K_EXIT */
	NULL,			/* K_FIND */
	NULL,			/* K_HELP */
};
#endif	/* !MSDOS */
static u_char ungetbuf[16];
static int ungetnum = 0;
static int termflags = 0;
#define	F_INITTTY		001
#define	F_TERMENT		002
#define	F_INITTERM		004
#define	F_TTYCHANGED		010
#define	F_WORKING		020
#define	F_RESETTTY		(F_INITTTY | F_TTYCHANGED)
#ifdef	CYGWIN
static int cygterm = 0;
#endif
static CONST char *deftermstr[MAXTERMSTR] = {
#ifdef	PC98
	"\033[>1h",		/* T_INIT */
	"\033[>1l",		/* T_END */
	"\033)3",		/* T_METAMODE */
	"\033)0",		/* T_NOMETAMODE */
#else
	"",			/* T_INIT */
	"",			/* T_END */
	"",			/* T_METAMODE */
	"",			/* T_NOMETAMODE */
#endif
	"",			/* T_SCROLL */
#if	MSDOS
	"",			/* T_KEYPAD */
	"",			/* T_NOKEYPAD */
	"\033[>5l",		/* T_NORMALCURSOR */
	"\033[>5l",		/* T_HIGHCURSOR */
	"\033[>5h",		/* T_NOCURSOR */
	"\033[s",		/* T_SETCURSOR */
	"\033[u",		/* T_RESETCURSOR */
#else	/* !MSDOS */
# ifdef	BOW
	/* hack for bowpad */
	"",			/* T_KEYPAD */
	"",			/* T_NOKEYPAD */
	"",			/* T_NORMALCURSOR */
	"",			/* T_HIGHCURSOR */
	"",			/* T_NOCURSOR */
# else
	"\033[?1h\033=",	/* T_KEYPAD */
	"\033[?1l\033>",	/* T_NOKEYPAD */
	"\033[?25h",		/* T_NORMALCURSOR */
	"\033[?25h",		/* T_HIGHCURSOR */
	"\033[?25l",		/* T_NOCURSOR */
# endif
	"\0337",		/* T_SETCURSOR */
	"\0338",		/* T_RESETCURSOR */
#endif	/* !MSDOS */
	"\007",			/* T_BELL */
	"\007",			/* T_VBELL */
	"\033[;H\033[2J",	/* T_CLEAR */
	"\033[m",		/* T_NORMAL */
	"\033[1m",		/* T_BOLD */
	"\033[7m",		/* T_REVERSE */
	"\033[2m",		/* T_DIM */
	"\033[5m",		/* T_BLINK */
	"\033[7m",		/* T_STANDOUT */
	"\033[4m",		/* T_UNDERLINE */
	"\033[m",		/* END_STANDOUT */
	"\033[m",		/* END_UNDERLINE */
	"\033[K",		/* L_CLEAR */
	"\033[1K",		/* L_CLEARBOL */
	"\033[L",		/* L_INSERT */
	"\033[M",		/* L_DELETE */
	"",			/* C_INSERT */
	"",			/* C_DELETE */
#if	MSDOS
	"\033[%d;%dH",		/* C_LOCATE */
#else	/* !MSDOS */
# ifdef	USETERMINFO
	"\033[%i%p1%d;%p2%dH",	/* C_LOCATE */
# else
	"\033[%i%d;%dH",	/* C_LOCATE */
# endif
#endif	/* !MSDOS */
	"\033[H",		/* C_HOME */
	"\r",			/* C_RETURN */
	"\n",			/* C_NEWLINE */
	"\n",			/* C_SCROLLFORW */
	"",			/* C_SCROLLREV */
	"\033[A",		/* C_UP */
	"\n",			/* C_DOWN */
	"\033[C",		/* C_RIGHT */
	"\b",			/* C_LEFT */
#ifdef	USETERMINFO
	"\033[%p1%dA",		/* C_NUP */
	"\033[%p1%dB",		/* C_NDOWN */
	"\033[%p1%dC",		/* C_NRIGHT */
	"\033[%p1%dD",		/* C_NLEFT */
	"\033[3%p1%dm",		/* T_FGCOLOR */
	"\033[4%p1%dm",		/* T_BGCOLOR */
#else
	"\033[%dA",		/* C_NUP */
	"\033[%dB",		/* C_NDOWN */
	"\033[%dC",		/* C_NRIGHT */
	"\033[%dD",		/* C_NLEFT */
	"\033[3%dm",		/* T_FGCOLOR */
	"\033[4%dm",		/* T_BGCOLOR */
#endif
};


#if	MSDOS
VOID inittty(reset)
int reset;
{
# ifndef	DJGPP
	static u_char dupbrk;
	union REGS reg;
# endif
	static int dupin = -1;
	static int dupout = -1;
	int fd;

	if (termflags & F_WORKING) return;
	termflags |= F_WORKING;

	if (!reset) {
		if (opentty(&ttyio, &ttyout) < 0) terror("opentty()");
# ifdef	NOTUSEBIOS
		if (!keybuftop) keybuftop = getkeybuf(KEYBUFWORKTOP);
# endif
# ifndef	DJGPP
		reg.x.ax = 0x3300;
		int86(0x21, &reg, &reg);
		dupbrk = reg.h.dl;
# endif
		if (!isatty(STDIN_FILENO)) {
			if (dupin < 0)
				dupin = newdup(safe_dup(STDIN_FILENO));
			if (dupin < 0 || safe_dup2(ttyio, STDIN_FILENO) < 0)
				terror("dup2()");
		}
		if (!isatty(STDOUT_FILENO)) {
			if (dupout < 0)
				dupout = newdup(safe_dup(STDOUT_FILENO));
			if (dupout < 0 || safe_dup2(ttyio, STDOUT_FILENO) < 0)
				terror("dup2()");
		}
		termflags |= F_INITTTY;
	}
	else if (termflags & F_INITTTY) {
# ifndef	DJGPP
		reg.x.ax = 0x3301;
		reg.h.dl = dupbrk;
		int86(0x21, &reg, &reg);
# endif
		if (dupin >= 0) {
			fd = safe_dup2(dupin, STDIN_FILENO);
			safeclose(dupin);
			dupin = -1;
			if (fd < 0) {
				termflags &= ~F_INITTTY;
				terror("dup2()");
			}
		}
		if (dupout >= 0) {
			fd = safe_dup2(dupout, STDOUT_FILENO);
			safeclose(dupout);
			dupout = -1;
			if (fd < 0) {
				termflags &= ~F_INITTTY;
				terror("dup2()");
			}
		}
	}
	termflags &= ~F_WORKING;
}

VOID Xcooked(VOID_A)
{
# ifndef	DJGPP
	union REGS reg;

	reg.x.ax = 0x3301;
	reg.h.dl = 1;
	int86(0x21, &reg, &reg);
# endif
}

VOID Xcbreak(VOID_A)
{
# ifndef	DJGPP
	union REGS reg;

	reg.x.ax = 0x3301;
	reg.h.dl = 0;
	int86(0x21, &reg, &reg);
# endif
}

VOID Xraw(VOID_A)
{
# ifndef	DJGPP
	union REGS reg;

	reg.x.ax = 0x3301;
	reg.h.dl = 0;
	int86(0x21, &reg, &reg);
# endif
}

VOID Xecho(VOID_A)
{
}

VOID Xnoecho(VOID_A)
{
}

VOID Xnl(VOID_A)
{
}

VOID Xnonl(VOID_A)
{
}

VOID tabs(VOID_A)
{
}

VOID notabs(VOID_A)
{
}

VOID keyflush(VOID_A)
{
	__term_regs reg;

	ungetnum = 0;
	disable();
	reg.x.ax = 0x0c00;
	intdos2(&reg);
# ifdef	NOTUSEBIOS
	keybuftop = getkeybuf(KEYBUFWORKTOP);
# endif
	enable();
}

#else	/* !MSDOS */

static int NEAR chkctrl(key)
int key;
{
	int i;

	if (!keyseqtree) return(key);
	for (i = 0; i < keyseqtree -> num; i++)
		if (key == keyseq[keyseqtree -> next[i].key].str[0])
			return(K_UNDEF);

	return(key);
}

/*ARGSUSED*/
static int NEAR getctrl(buf, ccbuf)
CONST char *buf, *ccbuf;
{
# ifdef	USESGTTY
	struct tchars *ccp, cc;
# endif
	static termioctl_t *ttyp, tty;

	if (ttyio < 0) return(-1);

	if (buf) ttyp = (termioctl_t *)buf;
	else if (tioctl(ttyio, REQGETP, &tty) < 0) return(-1);
	else ttyp = &tty;

# ifdef	USESGTTY
	if (ccbuf) ccp = (struct tchars *)ccbuf;
	else if (Xioctl(ttyio, TIOCGETC, &cc) < 0) return(-1);
	else ccp = &cc;
	cc_intr = chkctrl(ccp -> t_intrc);
	cc_quit = chkctrl(ccp -> t_quitc);
	cc_eof = chkctrl(ccp -> t_eofc);
	cc_eol = chkctrl(ccp -> t_brkc);
	cc_erase = chkctrl(ttyp -> sg_erase);
# else	/* !USESGTTY */
#  ifdef	VINTR
	cc_intr = chkctrl(ttyp -> c_cc[VINTR]);
#  endif
#  ifdef	VQUIT
	cc_quit = chkctrl(ttyp -> c_cc[VQUIT]);
#  endif
#  ifdef	VEOF
	cc_eof = chkctrl(ttyp -> c_cc[VEOF]);
#  endif
#  ifdef	VEOL
	cc_eol = chkctrl(ttyp -> c_cc[VEOL]);
#  endif
	cc_erase = chkctrl(ttyp -> c_cc[VERASE]);
# endif	/* !USESGTTY */

	return(0);
}

VOID inittty(reset)
int reset;
{
# ifdef	USESGTTY
	static int dupttyflag;
# endif
	static termioctl_t dupttyio;
	termioctl_t tty;
	int n;

	if (termflags & F_WORKING) return;
	termflags |= F_WORKING;

	if (!reset) {
		if (opentty(&ttyio, &ttyout) < 0) terror("opentty()");
	}
	else if (ttyio < 0 || (termflags & F_RESETTTY) != F_RESETTTY) {
		termflags &= ~F_WORKING;
		return;
	}

	n = 0;
	if (tioctl(ttyio, REQGETP, &tty) < 0) n = -2;
	else if (!reset) {
		memcpy((char *)&dupttyio, (char *)&tty, sizeof(termioctl_t));
# ifdef	USESGTTY
		if (Xioctl(ttyio, TIOCLGET, &dupttyflag) < 0) n = -1;
		else
# endif
		n = getctrl((CONST char *)&dupttyio, NULL);
# ifndef	USETERMINFO
		ospeed = getspeed(dupttyio);
# endif
		termflags |= F_INITTTY;
	}
	else {
		if (tioctl(ttyio, REQSETP, &dupttyio) < 0) n = -2;
# ifdef	USESGTTY
		else if (Xioctl(ttyio, TIOCLSET, &dupttyflag) < 0) n = -2;
# endif
	}
	if (n < 0) {
		if (n < -1) {
			safeclose(ttyio);
			ttyio = -1;
		}
		termflags &= ~F_INITTTY;
		terror("ioctl()");
	}
	termflags &= ~F_WORKING;
}

# ifdef	USESGTTY
static VOID NEAR ttymode(set, reset, lset, lreset)
int set, reset, lset, lreset;
# else
static VOID NEAR ttymode(set, reset, iset, ireset, oset, oreset, vmin, vtime)
int set, reset, iset, ireset, oset, oreset, vmin, vtime;
# endif
{
# ifdef	USESGTTY
	int lflag;
# endif
	termioctl_t tty;

	if (!(termflags & F_INITTTY) || (termflags & F_WORKING)) return;
	termflags |= F_WORKING;

	if (tioctl(ttyio, REQGETP, &tty) < 0) terror("ioctl()");
# ifdef	USESGTTY
	if (Xioctl(ttyio, TIOCLGET, &lflag) < 0) terror("ioctl()");
	if (set) tty.sg_flags |= set;
	if (reset) tty.sg_flags &= ~reset;
	if (lset) lflag |= lset;
	if (lreset) lflag &= ~lreset;
	if (Xioctl(ttyio, TIOCLSET, &lflag) < 0) terror("ioctl()");
# else	/* !USESGTTY */
	if (set) tty.c_lflag |= set;
	if (reset) tty.c_lflag &= ~reset;
	if (iset) tty.c_iflag |= iset;
	if (ireset) tty.c_iflag &= ~ireset;
	if (oset) tty.c_oflag |= oset;
	if (oreset) tty.c_oflag &= ~oreset;
	if (vmin) {
		tty.c_cc[VMIN] = vmin;
		tty.c_cc[VTIME] = vtime;
	}
# endif	/* !USESGTTY */
	if (tioctl(ttyio, REQSETP, &tty) < 0) terror("ioctl()");
	termflags |= F_TTYCHANGED;
	termflags &= ~F_WORKING;
}

VOID Xcooked(VOID_A)
{
# ifdef	USESGTTY
	ttymode(0, CBREAK | RAW, LPASS8, LLITOUT | LPENDIN);
# else
	ttymode(TIO_LCOOKED, PENDIN, TIO_ICOOKED, ~TIO_INOCOOKED,
		OPOST, 0, VAL_VMIN, VAL_VTIME);
# endif
}

VOID Xcbreak(VOID_A)
{
# ifdef	USESGTTY
	ttymode(CBREAK, 0, LLITOUT, 0);
# else
	ttymode(TIO_LCBREAK, ICANON, TIO_ICOOKED, IGNBRK, OPOST, 0, 1, 0);
# endif
}

VOID Xraw(VOID_A)
{
# ifdef	USESGTTY
	ttymode(RAW, 0, LLITOUT, 0);
# else
	ttymode(0, TIO_LCOOKED, IGNBRK, TIO_ICOOKED, 0, OPOST, 1, 0);
# endif
}

VOID Xecho(VOID_A)
{
# ifdef	USESGTTY
	ttymode(ECHO, 0, LCRTBS | LCRTERA | LCRTKIL | LCTLECH, 0);
# else
	ttymode(TIO_LECHO, ECHONL, 0, 0, 0, 0, 0, 0);
# endif
}

VOID Xnoecho(VOID_A)
{
# ifdef	USESGTTY
	ttymode(0, ECHO, 0, LCRTBS | LCRTERA);
# else
	ttymode(0, ~TIO_LNOECHO, 0, 0, 0, 0, 0, 0);
# endif
}

VOID Xnl(VOID_A)
{
# ifdef	USESGTTY
	ttymode(CRMOD, 0, 0, 0);
# else
	ttymode(0, 0, ICRNL, 0, ONLCR, ~TIO_ONONL, 0, 0);
# endif
}

VOID Xnonl(VOID_A)
{
# ifdef	USESGTTY
	ttymode(0, CRMOD, 0, 0);
# else
	ttymode(0, 0, 0, ICRNL, 0, ONLCR, 0, 0);
# endif
}

VOID tabs(VOID_A)
{
# ifdef	USESGTTY
	ttymode(0, XTABS, 0, 0);
# else
	ttymode(0, 0, 0, 0, 0, TAB3, 0, 0);
# endif
}

VOID notabs(VOID_A)
{
# ifdef	USESGTTY
	ttymode(XTABS, 0, 0, 0);
# else
	ttymode(0, 0, 0, 0, TAB3, 0, 0, 0);
# endif
}

VOID keyflush(VOID_A)
{
	int arg;

	ungetnum = 0;
	if (ttyio < 0) return;
	arg = FLSHIN;
	VOID_C ttyflush(ttyio, arg);
}

int savettyio(reset)
int reset;
{
	char *tty;
	int n;

	if (reset) tty = NULL;
	else {
		savetermio(ttyio, &tty, NULL);
		if (!tty) return(-1);
# ifdef	USESGTTY
		VOID_C getctrl(tty, &(tty[sizeof(termioctl_t) + sizeof(int)]));
# else
		VOID_C getctrl(tty, NULL);
# endif
	}

	n = (isttyiomode) ? 0 : 1;
	if (duptty[n]) free(duptty[n]);
	duptty[n] = tty;

	return(n);
}
#endif	/* !MSDOS */

VOID ttyiomode(isnl)
int isnl;
{
	if (ttyio < 0) /*EMPTY*/;
#if	!MSDOS
	else if (duptty[0]) loadtermio(ttyio, duptty[0], NULL);
#endif
	else {
#if	MSDOS
		Xraw();
#else	/* !MSDOS */
# ifdef	USESGTTY
		Xraw();
		Xnoecho();
		Xnonl();
		notabs();
		if (dumbterm > 1) ttymode(ECHO | CRMOD, 0, 0, 0);
# else	/* !USESGTTY */
		if (isnl) ttymode(0, TIO_LCOOKED | ~TIO_LNOECHO,
			IGNBRK, TIO_ICOOKED | ICRNL, TIO_ONL | TAB3, 0, 1, 0);
		else ttymode(0, TIO_LCOOKED | ~TIO_LNOECHO,
			IGNBRK, TIO_ICOOKED | ICRNL, TAB3, TIO_ONL, 1, 0);
		if (dumbterm > 1)
			ttymode(ECHO, 0, ICRNL, 0, TIO_ONL, 0, 1, 0);
# endif	/* !USESGTTY */
#endif	/* !MSDOS */
	}
	if ((termflags & F_TERMENT) && !dumbterm) {
		putterm(T_KEYPAD);
		tflush();
	}
	isttyiomode = isnl + 1;
}

VOID stdiomode(VOID_A)
{
#if	!MSDOS && !defined (USESGTTY)
	int isnl;
#endif

	if (ttyio < 0) /*EMPTY*/;
#if	!MSDOS
	else if (duptty[1]) loadtermio(ttyio, duptty[1], NULL);
#endif
	else {
#if	MSDOS
		Xcooked();
#else	/* !MSDOS */
# ifdef	USESGTTY
		Xcooked();
		Xecho();
		Xnl();
		tabs();
		if (dumbterm > 2) ttymode(0, ECHO | CRMOD, 0, 0);
# else	/* !USESGTTY */
		isnl = (isttyiomode) ? isttyiomode - 1 : 0;
		if (isnl) ttymode(TIO_LCOOKED | TIO_LECHO, PENDIN | ECHONL,
			TIO_ICOOKED | ICRNL, ~TIO_INOCOOKED,
			0, ~TIO_ONONL | TAB3, VAL_VMIN, VAL_VTIME);
		else ttymode(TIO_LCOOKED | TIO_LECHO, PENDIN | ECHONL,
			TIO_ICOOKED | ICRNL, ~TIO_INOCOOKED,
			TIO_ONL, ~TIO_ONONL | TAB3, VAL_VMIN, VAL_VTIME);
		if (dumbterm > 2)
			ttymode(0, ECHO,
				0, ICRNL, 0, ONLCR, VAL_VMIN, VAL_VTIME);
# endif	/* !USESGTTY */
#endif	/* !MSDOS */
	}
	if ((termflags & F_TERMENT) && !dumbterm) {
		putterm(T_NOKEYPAD);
		tflush();
	}
	isttyiomode = 0;
}

int termmode(init)
int init;
{
	static int mode = 0;
	int oldmode;

	oldmode = mode;
	if (init >= 0 && mode != init) {
		if ((termflags & F_TERMENT) && !dumbterm) {
			putterms((init) ? T_INIT : T_END);
			tflush();
		}
		mode = init;
	}

	return(oldmode);
}

VOID exit2(n)
int n;
{
	if ((termflags & F_TERMENT) && !dumbterm) putterm(T_NORMAL);
	endterm();
	inittty(1);
	keyflush();
#ifdef	DEBUG
# if	!MSDOS
	freeterment();
# endif
	VOID_C closetty(&ttyio, &ttyout);
	muntrace();
#endif	/* DEBUG */
	exit(n);
}

static VOID NEAR terror(mes)
CONST char *mes;
{
	int duperrno;

	duperrno = errno;
	if (termflags & F_INITTTY) {
		if ((termflags & F_TERMENT) && !dumbterm) putterm(T_NORMAL);
		endterm();
		Xcooked();
		Xecho();
		Xnl();
		tabs();
	}
	if ((termflags & F_TERMENT) && dumbterm <= 2)
		VOID_C Xfputc('\007', Xstderr);
	VOID_C fputnl(Xstderr);
	errputs(mes, 0);

	if (duperrno) VOID_C Xfprintf(Xstderr, ": %s", Xstrerror(duperrno));
	VOID_C fputnl(Xstderr);
	inittty(1);
	exit(2);
}

#if	!MSDOS
static char *NEAR tstrdup(s)
CONST char *s;
{
	char *cp;

	if (!s) s = "";
	if (!(cp = (char *)malloc(strlen(s) + 1))) terror("malloc()");
	Xstrcpy(cp, s);

	return(cp);
}
#endif	/* !MSDOS */

static VOID NEAR defaultterm(VOID_A)
{
	int i;

	if (dumbterm > 1) {
#if	MSDOS
		for (i = 0; i < MAXTERMSTR; i++) termstr[i] = "";
#else	/* !MSDOS */
		for (i = 0; i < MAXTERMSTR; i++) termstr[i] = tstrdup(NULL);
# ifndef	USETERMINFO
		PC = '\0';
		BC = tstrdup(NULL);
		UP = tstrdup(NULL);
# endif
		for (i = 0; i <= K_MAX - K_MIN; i++) {
			if (!(defkeyseq[i])) keyseq[i].str = NULL;
			else keyseq[i].str = tstrdup(NULL);
		}
#endif	/* !MSDOS */
	}
	else {
#if	MSDOS
		for (i = 0; i < MAXTERMSTR; i++) termstr[i] = deftermstr[i];
#else	/* !MSDOS */
		for (i = 0; i < MAXTERMSTR; i++)
			termstr[i] = tstrdup(deftermstr[i]);
# ifndef	USETERMINFO
		PC = '\0';
		BC = tstrdup("\010");
		UP = tstrdup("\033[A");
# endif
		for (i = 0; i <= K_MAX - K_MIN; i++) {
			if (!(defkeyseq[i])) keyseq[i].str = NULL;
			else keyseq[i].str = tstrdup(defkeyseq[i]);
		}
#endif	/* !MSDOS */
	}

#if	!MSDOS
	for (i = 0; i <= K_MAX - K_MIN; i++) {
		keyseq[i].code = K_MIN + i;
		keyseq[i].flags = 0;
	}
	for (i = 1; i <= 10; i++) {
		keyseq[K_F(i) - K_MIN].flags |= KF_HASALTER;
		keyseq[K_F(i + 20) - K_MIN].code = K_F(i) | K_ALTERNATE;
		keyseq[K_F(i + 20) - K_MIN].flags |= KF_HASALTER;
	}
#endif
}

#if	!MSDOS || defined (PC98) || defined (NOTUSEBIOS)
static int NEAR Xungetch(c)
int c;
{
	if (c == EOF || ungetnum >= arraysize(ungetbuf)) return(EOF);
	ungetbuf[ungetnum++] = (u_char)c;

	return(c);
}
#endif	/* !MSDOS || PC98 || NOTUSEBIOS */

#if	MSDOS && defined (USEVIDEOBIOS)
static int NEAR maxlocate(yp, xp)
int *yp, *xp;
{
	*xp = getbiosbyte(SCRWIDTH);
	*yp = getbiosbyte(SCRHEIGHT) + 1;

	return(0);
}

int getxy(xp, yp)
int *xp, *yp;
{
	__term_regs reg;

	reg.x.ax = 0x0300;
	reg.h.bh = getbiosbyte(CURRPAGE);
	intbios(VIDEOBIOS, &reg);
	*xp = reg.h.dl + 1;
	*yp = reg.h.dh + 1;

	return(0);
}

#else	/* !MSDOS || !USEVIDEOBIOS */

static int NEAR maxlocate(yp, xp)
int *yp, *xp;
{
	int i, x, y;

# if	!MSDOS
	if (!usegetcursor) return(-1);
# endif
	if (getxy(&x, &y) < 0) x = y = 0;
# if	MSDOS
	VOID_C tputparam(C_LOCATE, 1, 999, 1);
	VOID_C tputparam(C_NDOWN, 999, 999, n_line);
# else	/* !MSDOS */
#  if	defined (CYGWIN) && (CYGWIN > 1007009)
	if (cygterm) {
		locate(998, 0);
		movecursor(-1, C_DOWN, 999);
	}
	else
#  endif
	locate(998, 998);
# endif	/* !MSDOS */
	i = getxy(xp, yp);
	if (x > 0 && y > 0) locate(--x, --y);

	return(i);
}

/*ARGSUSED*/
static int NEAR pollterm(first)
int first;
{
#if	!MSDOS
	long usec;
#endif

#if	MSDOS
	if (!(bdos(0x0b, 0, 0) & 0xff)) return(EOF);
#else
	usec = WAITTERMINAL * 1000L;
	if (first) usec *= 10L;

	if (!kbhit2(usec)) return(EOF);
#endif

	return(Xgetch());
}

int getxy(xp, yp)
int *xp, *yp;
{
	char *format, buf[sizeof(SIZEFMT) + (MAXLONGWIDTH - 2) * 2];
	int i, j, tmp, count, *val[2];

	format = SIZEFMT;
	keyflush();
# if	!MSDOS
	if (!usegetcursor) return(-1);
# endif
	tputs2(GETSIZE, 1);
	tflush();

	buf[0] = '\0';
	do {
		if ((tmp = pollterm(1)) == EOF) break;
		buf[0] = tmp;
	} while (buf[0] != format[0]);

	i = 0;
	if (buf[0] == format[0]) for (i++; i < strsize(buf); i++) {
		if ((tmp = pollterm(0)) == EOF) break;
		buf[i] = tmp;
		if (buf[i] == format[strsize(SIZEFMT) - 1]) break;
	}
	keyflush();
# ifdef	CYGWIN
	if (cygterm) /*EMPTY*/;
	else
# endif
	for (;;) if (pollterm(0) == EOF) break;
	if (!i || buf[i++] != format[strsize(SIZEFMT) - 1]) return(-1);
	buf[i] = '\0';

	count = 0;
	val[0] = yp;
	val[1] = xp;

	for (i = j = 0; format[i] && buf[j]; i++) {
		if (format[i] == '%' && format[i + 1] == 'd') {
			if ((tmp = getnum(buf, &j)) < 0) break;
			i++;
			if (count++ < 2) *(val[count - 1]) = tmp;
		}
		else if (format[i] != buf[j++]) break;
	}
	if (count != 2) return(-1);

	return(0);
}
#endif	/* !MSDOS || !USEVIDEOBIOS */

#if	MSDOS
static char *NEAR tparamstr(s, arg1, arg2)
CONST char *s;
int arg1, arg2;
{
	char *cp;

	if (Xasprintf(&cp, s, arg1, arg2) < 0) return(NULL);

	return(cp);
}

/*ARGSUSED*/
VOID getterment(s)
CONST char *s;
{
	if ((termflags & F_TERMENT) || (termflags & F_WORKING)) return;
	termflags |= F_WORKING;

	defaultterm();
	if (n_column < 0) n_column = 80;
	n_lastcolumn = n_column - 1;
	if (n_line < 0) n_line = 25;
	termflags |= F_TERMENT;
	termflags &= ~F_WORKING;
}

#else	/* !MSDOS */

static char *NEAR tparamstr(s, arg1, arg2)
CONST char *s;
int arg1, arg2;
{
# ifndef	USETERMINFO
	printbuf_t pbuf;
	char *tmp;
	int i, j, n, pop, err, args[2];
# endif

# ifdef	USETERMINFO
#  ifdef	DEBUG
	if (!s) return(NULL);
	_mtrace_file = "tparm(start)";
	s = tparm((char *)s, arg1, arg2, 0, 0, 0, 0, 0, 0, 0);
	if (_mtrace_file) _mtrace_file = NULL;
	else {
		_mtrace_file = "tparm(end)";
		malloc(0);	/* dummy alloc */
	}
	if (!s) return(NULL);
#  else
	if (!s || !(s = tparm((char *)s, arg1, arg2, 0, 0, 0, 0, 0, 0, 0)))
		return(NULL);
#  endif

	return(tstrdup(s));
# else	/* !USETERMINFO */
	if (!s || !(pbuf.buf = (char *)malloc(1))) return(NULL);
	pbuf.ptr = pbuf.size = 0;
	pbuf.flags = VF_NEW;
	args[0] = arg1;
	args[1] = arg2;
	pop = 0;

	for (i = n = 0; s[i]; i++) {
		tmp = NULL;
		err = 0;
		if (s[i] != '%' || s[++i] == '%') {
			if (setchar(s[i], &pbuf) < 0) return(NULL);
		}
		else if (n >= 2) err++;
		else switch (s[i]) {
			case 'd':
				if (Xasprintf(&tmp, "%d", args[n++]) < 0)
					err++;
				break;
			case '2':
				if (Xasprintf(&tmp, "%02d", args[n++]) < 0)
					err++;
				break;
			case '3':
				if (Xasprintf(&tmp, "%03d", args[n++]) < 0)
					err++;
				break;
			case '.':
				if (Xasprintf(&tmp, "%c", args[n++]) < 0)
					err++;
				break;
			case '+':
				if (!s[++i]
				|| Xasprintf(&tmp, "%c", args[n++] + s[i]) < 0)
					err++;
				break;
			case '>':
				if (!s[++i] || !s[i + 1]) err++;
				else if (args[n] > s[i++]) args[n] += s[i];
				break;
			case 'p':
				i++;
				if (s[i] == '1') {
					pop = 1;
					break;
				}
				else if (s[i] == '2') {
					if (pop) break;
					pop = 2;
				}
				else break;
/*FALLTHRU*/
			case 'r':
				j = args[0];
				args[0] = args[1];
				args[1] = j;
				break;
			case 'i':
				args[0]++;
				args[1]++;
				break;
			case 'n':
				args[0] ^= 0140;
				args[1] ^= 0140;
				break;
			case 'B':
				args[n] = ((args[n] / 10) << 4)
					| (args[n] % 10);
				break;
			case 'D':
				args[n] -= 2 * (args[n] % 16);
				break;
			default:
				err++;
				break;
		}

		if (err) {
			free(pbuf.buf);
			return(NULL);
		}
		if (tmp) {
			for (j = 0; tmp[j]; j++)
				if (setchar(tmp[j], &pbuf) < 0) return(NULL);
			free(tmp);
		}
	}

	pbuf.buf[pbuf.ptr] = '\0';

	return(pbuf.buf);
# endif	/* !USETERMINFO */
}

static char *NEAR tgetstr2(term, s)
char **term;
CONST char *s;
{
# ifndef	USETERMINFO
	char strbuf[TERMCAPSIZE];
	char *p;
# endif

# ifndef	USETERMINFO
	p = strbuf;
#  ifdef	DEBUG
	_mtrace_file = "tgetstr(start)";
	s = tgetstr(s, &p);
	if (_mtrace_file) _mtrace_file = NULL;
	else {
		_mtrace_file = "tgetstr(end)";
		malloc(0);	/* dummy alloc */
	}
#  else
	s = tgetstr(s, &p);
#  endif
# endif	/* !USETERMINFO */
	if (s) {
		if (*term) free(*term);
		*term = tstrdup(s);
	}

	return(*term);
}

static char *NEAR tgetstr3(term, str1, str2)
char **term;
CONST char *str1, *str2;
{
# ifndef	USETERMINFO
	char *p, strbuf[TERMCAPSIZE];
# endif
	char *str;

# ifdef	USETERMINFO
	str = (str1) ? tstrdup(str1) : tparamstr(str2, 1, 1);
# else	/* !USETERMINFO */
	p = strbuf;
#  ifdef	DEBUG
	_mtrace_file = "tgetstr(start)";
	str = tgetstr(str1, &p);
	if (_mtrace_file) _mtrace_file = NULL;
	else {
		_mtrace_file = "tgetstr(end)";
		malloc(0);	/* dummy alloc */
	}
	if (str) str = tstrdup(str);
	else {
		_mtrace_file = "tgetstr(start)";
		str = tgetstr(str2, &p);
		if (_mtrace_file) _mtrace_file = NULL;
		else {
			_mtrace_file = "tgetstr(end)";
			malloc(0);	/* dummy alloc */
		}
		str = tparamstr(str, 1, 1);
	}
#  else
	str = tgetstr(str1, &p);
	if (str) str = tstrdup(str);
	else {
		str = tgetstr(str2, &p);
		str = tparamstr(str, 1, 1);
	}
#  endif
# endif	/* !USETERMINFO */
	if (str) {
		if (*term) free(*term);
		*term = str;
	}

	return(*term);
}

static char *NEAR tgetkeyseq(n, s)
int n;
CONST char *s;
{
	char *cp;
	int i, j;

	n -= K_MIN;
	cp = NULL;
	if (!tgetstr2(&cp, s)) return(NULL);
	if ((keyseq[n].code & K_ALTERNATE) && *cp != '\033') return(NULL);
	for (i = 0; i <= K_MAX - K_MIN; i++) {
		if (!(keyseq[i].str)) continue;
		for (j = 0; cp[j]; j++)
			if ((cp[j] & 0x7f) != (keyseq[i].str[j] & 0x7f))
				break;
		if (cp[j]) continue;
		free(keyseq[i].str);
		keyseq[i].str = NULL;
		keyseq[i].flags &= ~KF_DEFINED;
	}

	if (!(keyseq[n].str) || !(keyseq[n].flags & KF_HASALTER)) i = -1;
	else for (i = K_MAX - K_MIN; i >= 0; i--) {
		if (i == n) continue;
		if (alternate(keyseq[n].code) == alternate(keyseq[i].code))
			break;
	}
	if (i < 0 || (keyseq[i].flags & KF_DEFINED)) /*EMPTY*/;
	else if (!(keyseq[n].code & K_ALTERNATE) || !(keyseq[i].str)) {
		if (keyseq[i].str) free(keyseq[i].str);
		keyseq[i].str = keyseq[n].str;
		keyseq[i].flags = keyseq[n].flags;
		keyseq[n].str = NULL;
	}

	if (keyseq[n].str) free(keyseq[n].str);
	keyseq[n].str = cp;
	keyseq[n].flags |= KF_DEFINED;

	return(cp);
}

static kstree_t *NEAR newkeyseqtree(parent, num)
kstree_t *parent;
int num;
{
	kstree_t *new;
	int n;

	if (!parent || !(parent -> next))
		new = (kstree_t *)malloc(num * sizeof(kstree_t));
	else new = (kstree_t *)realloc(parent -> next, num * sizeof(kstree_t));
	if (!new) terror("realloc()");

	if (!parent) n = 0;
	else {
		n = parent -> num;
		parent -> num = (u_char)num;
		parent -> next = new;
	}

	while (n < num) {
		new[n].key = (u_char)-1;
		new[n].num = (u_char)0;
		new[n++].next = (kstree_t *)NULL;
	}

	return(new);
}

static VOID NEAR freekeyseqtree(list, n)
kstree_t *list;
int n;
{
	int i;

	if (!list) return;
	for (i = (int)(list[n].num) - 1; i >= 0; i--)
		freekeyseqtree(list[n].next, i);
	if (!n) free(list);
}

static int cmpkeyseq(vp1, vp2)
CONST VOID_P vp1;
CONST VOID_P vp2;
{
	keyseq_t *kp1, *kp2;
	int n;

	kp1 = (keyseq_t *)vp1;
	kp2 = (keyseq_t *)vp2;
	if (!(kp1 -> str)) return(-1);
	if (!(kp2 -> str)) return(1);
	n = (kp1 -> len < kp2 -> len) ? kp1 -> len : kp2 -> len;
	if ((n = memcmp(kp1 -> str, kp2 -> str, n))) return(n);

	return((int)(kp1 -> len) - (int)(kp2 -> len));
}

static VOID NEAR sortkeyseq(VOID_A)
{
	kstree_t *p;
	int i, j, k;

	qsort(keyseq, K_MAX - K_MIN + 1, sizeof(keyseq_t), cmpkeyseq);
	if (keyseqtree) freekeyseqtree(keyseqtree, 0);

	keyseqtree = newkeyseqtree(NULL, 1);

	for (i = 0; i <= K_MAX - K_MIN; i++) {
		p = keyseqtree;
		for (j = 0; j < keyseq[i].len; j++) {
			for (k = 0; k < p -> num; k++) {
				if (keyseq[i].str[j]
				== keyseq[p -> next[k].key].str[j])
					break;
			}
			if (k >= p -> num) {
				newkeyseqtree(p, k + 1);
				p -> next[k].key = (u_char)i;
			}
			p = &(p -> next[k]);
		}
	}
}

VOID getterment(s)
CONST char *s;
{
# ifndef	USETERMINFO
	char buf[TERMCAPSIZE];
# endif
# ifdef	IRIX
	int winterm;		/* for STUPID winterm entry */
# endif
	CONST char *term;
	char *cp;
	int i, j, dumb, dupdumbterm;

	if ((termflags & F_TERMENT) || (termflags & F_WORKING)) return;
	termflags |= F_WORKING;

	dupdumbterm = dumbterm;
	dumbterm = dumb = 0;
	term = (s) ? s : (char *)getenv(ENVTERM);
	if (!term || !*term) {
		dumbterm = 1;
		dumb = DUMBLISTSIZE;
	}
	else {
		for (i = 0; i < DUMBLISTSIZE; i++)
			if (!strcmp(term, dumblist[i])) break;
		if (i < DUMBLISTSIZE) dumbterm = 1;
# ifdef	IRIX
		winterm = !strncmp(term, WINTERMNAME, strsize(WINTERMNAME));
# endif
# ifdef	CYGWIN
		cygterm = !strcmp(term, CYGTERMNAME);
# endif
	}

	for (;;) {
		if (dumb) term = dumblist[--dumb];
# ifdef	USETERMINFO
#  ifdef	DEBUG
		_mtrace_file = "setupterm(start)";
		setupterm((char *)term, Xfileno(ttyout), &i);
		if (_mtrace_file) _mtrace_file = NULL;
		else {
			_mtrace_file = "setupterm(end)";
			malloc(0);	/* dummy alloc */
		}
#  else
		setupterm((char *)term, Xfileno(ttyout), &i);
#  endif
		if (i == 1) break;
# else	/* !USETERMINFO */
#  ifdef	DEBUG
		_mtrace_file = "tgetent(start)";
		i = tgetent(buf, term);
		if (_mtrace_file) _mtrace_file = NULL;
		else {
			_mtrace_file = "tgetent(end)";
			malloc(0);	/* dummy malloc */
		}
#  else
		i = tgetent(buf, term);
#  endif
		if (i > 0) break;
# endif	/* !USETERMINFO */
		else if (dumb) continue;	/* retry for libncurses */
		else if (s) {
			dumbterm = 2;
			break;
		}
		else {
			errno = 0;
# ifdef	USETERMINFO
			terror("No TERMINFO is prepared");
# else
			terror("No TERMCAP is prepared");
# endif
		}
	}

	if (dupdumbterm >= 2 || !strcmp(term, "emacs")
	|| ((cp = getenv(ENVEMACS)) && !strcmp(cp, "t")))
		dumbterm = 3;

	defaultterm();
	termflags |= F_TERMENT;
	if (dumbterm > 1) {
		for (i = 0; i <= K_MAX - K_MIN; i++)
			keyseq[i].len = (keyseq[i].str)
				? strlen(keyseq[i].str) : 0;
		sortkeyseq();
		termflags &= ~F_WORKING;
		return;
	}

# ifndef	USETERMINFO
	cp = NULL;
	if (tgetstr2(&cp, TERM_pc)) {
		PC = *cp;
		free(cp);
	}
	tgetstr2(&BC, TERM_bc);
	tgetstr2(&UP, TERM_up);
# endif

	cp = NULL;
	if (tgetstr2(&cp, TERM_ku) || tgetstr2(&cp, TERM_kd)
	|| tgetstr2(&cp, TERM_kr) || tgetstr2(&cp, TERM_kl)) {
		free(cp);
		*(termstr[T_KEYPAD]) = *(termstr[T_NOKEYPAD]) = '\0';
		for (i = K_DOWN; i <= K_RIGHT; i++) {
			if (keyseq[i - K_MIN].str) {
				free(keyseq[i - K_MIN].str);
				keyseq[i - K_MIN].str = NULL;
			}
		}
	}

	if (n_column < 0 && (n_column = tgetnum2(TERM_co)) < 0) n_column = 80;
	n_lastcolumn = n_column;
	if (!tgetflag2(TERM_xn)) n_lastcolumn--;
	if (n_line < 0 && (n_line = tgetnum2(TERM_li)) < 0) n_column = 24;
	stable_standout = tgetflag2(TERM_xs);
	if (dumbterm < 2 && tgetflag2(TERM_gn)) dumbterm = 2;
	tgetstr2(&(termstr[T_INIT]), TERM_ti);
	tgetstr2(&(termstr[T_END]), TERM_te);
	tgetstr2(&(termstr[T_METAMODE]), TERM_mm);
	tgetstr2(&(termstr[T_NOMETAMODE]), TERM_mo);
	tgetstr2(&(termstr[T_SCROLL]), TERM_cs);
	tgetstr2(&(termstr[T_KEYPAD]), TERM_ks);
	tgetstr2(&(termstr[T_NOKEYPAD]), TERM_ke);
# ifdef	IRIX
	if (winterm) /*EMPTY*/;
	else
# endif
	{
		tgetstr2(&(termstr[T_NORMALCURSOR]), TERM_ve);
		tgetstr2(&(termstr[T_HIGHCURSOR]), TERM_vs);
		tgetstr2(&(termstr[T_NOCURSOR]), TERM_vi);
	}
	tgetstr2(&(termstr[T_SETCURSOR]), TERM_sc);
	tgetstr2(&(termstr[T_RESETCURSOR]), TERM_rc);
	tgetstr2(&(termstr[T_BELL]), TERM_bl);
	tgetstr2(&(termstr[T_VBELL]), TERM_vb);
	tgetstr2(&(termstr[T_CLEAR]), TERM_cl);
	tgetstr2(&(termstr[T_NORMAL]), TERM_me);
	tgetstr2(&(termstr[T_BOLD]), TERM_md);
	tgetstr2(&(termstr[T_REVERSE]), TERM_mr);
	tgetstr2(&(termstr[T_DIM]), TERM_mh);
	tgetstr2(&(termstr[T_BLINK]), TERM_mb);
	tgetstr2(&(termstr[T_STANDOUT]), TERM_so);
	tgetstr2(&(termstr[T_UNDERLINE]), TERM_us);
	tgetstr2(&(termstr[END_STANDOUT]), TERM_se);
	tgetstr2(&(termstr[END_UNDERLINE]), TERM_ue);
	tgetstr2(&(termstr[L_CLEAR]), TERM_ce);
	tgetstr2(&(termstr[L_CLEARBOL]), TERM_cb);
	tgetstr2(&(termstr[L_INSERT]), TERM_al);
	tgetstr2(&(termstr[L_DELETE]), TERM_dl);
	tgetstr3(&(termstr[C_INSERT]), TERM_ic, TERM_IC);
	tgetstr3(&(termstr[C_DELETE]), TERM_dc, TERM_DC);
	tgetstr2(&(termstr[C_LOCATE]), TERM_cm);
	tgetstr2(&(termstr[C_HOME]), TERM_ho);
	tgetstr2(&(termstr[C_RETURN]), TERM_cr);
	tgetstr2(&(termstr[C_NEWLINE]), TERM_nl);
	tgetstr2(&(termstr[C_SCROLLFORW]), TERM_sf);
	tgetstr2(&(termstr[C_SCROLLREV]), TERM_sr);
	tgetstr3(&(termstr[C_UP]), TERM_up, TERM_UP);
	tgetstr3(&(termstr[C_DOWN]), TERM_do, TERM_DO);
	tgetstr3(&(termstr[C_RIGHT]), TERM_nd, TERM_RI);
	tgetstr3(&(termstr[C_LEFT]), TERM_le, TERM_LE);
	tgetstr2(&(termstr[C_NUP]), TERM_UP);
	tgetstr2(&(termstr[C_NDOWN]), TERM_DO);
	tgetstr2(&(termstr[C_NRIGHT]), TERM_RI);
	tgetstr2(&(termstr[C_NLEFT]), TERM_LE);

# if	!defined (HPUX) || !defined (USETERMINFO) \
|| (defined (set_a_foreground) && defined (set_foreground))
	/* Hack for HP-UX 10.20 */
	cp = NULL;
	if (tgetstr2(&cp, TERM_AF) || tgetstr2(&cp, TERM_Sf)) {
		if (termstr[T_FGCOLOR]) free(termstr[T_FGCOLOR]);
		termstr[T_FGCOLOR] = cp;
	}
# endif

# if	!defined (HPUX) || !defined (USETERMINFO) \
|| (defined (set_a_background) && defined (set_background))
	/* Hack for HP-UX 10.20 */
	cp = NULL;
	if (tgetstr2(&cp, TERM_AB) || tgetstr2(&cp, TERM_Sb)) {
		if (termstr[T_BGCOLOR]) free(termstr[T_BGCOLOR]);
		termstr[T_BGCOLOR] = cp;
	}
# endif

	tgetkeyseq(K_UP, TERM_ku);
	tgetkeyseq(K_DOWN, TERM_kd);
	tgetkeyseq(K_RIGHT, TERM_kr);
	tgetkeyseq(K_LEFT, TERM_kl);
	tgetkeyseq(K_HOME, TERM_kh);
	tgetkeyseq(K_BS, TERM_kb);
	tgetkeyseq(K_F(1), TERM_k1);
	tgetkeyseq(K_F(2), TERM_k2);
	tgetkeyseq(K_F(3), TERM_k3);
	tgetkeyseq(K_F(4), TERM_k4);
	tgetkeyseq(K_F(5), TERM_k5);
	tgetkeyseq(K_F(6), TERM_k6);
	tgetkeyseq(K_F(7), TERM_k7);
	tgetkeyseq(K_F(8), TERM_k8);
	tgetkeyseq(K_F(9), TERM_k9);
	tgetkeyseq(K_F(10), TERM_k10);
	tgetkeyseq(K_F(11), TERM_F1);
	tgetkeyseq(K_F(12), TERM_F2);
	tgetkeyseq(K_F(13), TERM_F3);
	tgetkeyseq(K_F(14), TERM_F4);
	tgetkeyseq(K_F(15), TERM_F5);
	tgetkeyseq(K_F(16), TERM_F6);
	tgetkeyseq(K_F(17), TERM_F7);
	tgetkeyseq(K_F(18), TERM_F8);
	tgetkeyseq(K_F(19), TERM_F9);
	tgetkeyseq(K_F(20), TERM_FA);
	tgetkeyseq(K_F(21), TERM_l1);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(22), TERM_l2);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(23), TERM_l3);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(24), TERM_l4);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(25), TERM_l5);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(26), TERM_l6);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(27), TERM_l7);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(28), TERM_l8);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(29), TERM_l9);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_F(30), TERM_la);	/* Hack for NEWS-OS bug */
	tgetkeyseq(K_DL, TERM_kL);
	tgetkeyseq(K_IL, TERM_kA);
	tgetkeyseq(K_DC, TERM_kD);
	tgetkeyseq(K_IC, TERM_kI);
	tgetkeyseq(K_CLR, TERM_kC);
	tgetkeyseq(K_EOL, TERM_kE);
	tgetkeyseq(K_PPAGE, TERM_kP);
	tgetkeyseq(K_NPAGE, TERM_kN);

# if	!defined (HPUX) || !defined (USETERMINFO) \
|| (defined (key_enter) && defined (key_beg) && defined (key_end))
	/* Hack for HP-UX 10.20 */
#  ifdef	IRIX
	if (winterm) /*EMPTY*/;
	else
#  endif
	tgetkeyseq(K_ENTER, TERM_at8);
	tgetkeyseq(K_BEG, TERM_at1);
	tgetkeyseq(K_END, TERM_at7);
# endif	/* !HPUX || !USETERMINFO || (key_enter && key_beg && key_end) */

	for (i = 0; i <= K_MAX - K_MIN; i++) {
		if (!(keyseq[i].str)) keyseq[i].len = 0;
		else {
			for (j = 0; keyseq[i].str[j]; j++)
				keyseq[i].str[j] &= 0x7f;
			keyseq[i].len = j;
			if (j == 1 && keyseq[i].str[0] == cc_erase)
				cc_erase = K_UNDEF;
		}
	}
	sortkeyseq();
	termflags &= ~F_WORKING;
}

VOID freeterment(VOID_A)
{
	int i;

	if (!(termflags & F_TERMENT)) return;

# ifndef	USETERMINFO
	if (BC) free(BC);
	if (UP) free(UP);
# endif
	for (i = 0; i < MAXTERMSTR; i++) if (termstr[i]) {
		free(termstr[i]);
		termstr[i] = NULL;
	}
	for (i = 0; i <= K_MAX - K_MIN; i++) {
		if (keyseq[i].str) free(keyseq[i].str);
		keyseq[i].str = NULL;
		keyseq[i].len = keyseq[i].flags = 0;
	}
	if (keyseqtree) {
		freekeyseqtree(keyseqtree, 0);
		keyseqtree = NULL;
	}
	termflags &= ~F_TERMENT;
}

VOID regetterment(s, tty)
CONST char *s;
int tty;
{
	int wastty;

	if ((wastty = isttyiomode) && tty) stdiomode();
	freeterment();
	getterment(s);
	if (wastty && tty) ttyiomode(wastty - 1);
}

VOID setdefterment(VOID_A)
{
	freeterment();
	defaultterm();
}

VOID setdefkeyseq(VOID_A)
{
	int i;

	for (i = 0; i <= K_MAX - K_MIN; i++) {
		if (keyseq[i].str) free(keyseq[i].str);
		if (!(defkeyseq[i])) {
			keyseq[i].str = NULL;
			keyseq[i].len = 0;
		}
		else {
			keyseq[i].str = tstrdup(defkeyseq[i]);
			keyseq[i].len = strlen(keyseq[i].str);
		}
		keyseq[i].code = K_MIN + i;
		keyseq[i].flags = 0;
	}
	for (i = 1; i <= 10; i++) {
		keyseq[K_F(i) - K_MIN].flags |= KF_HASALTER;
		keyseq[K_F(i + 20) - K_MIN].code = K_F(i) | K_ALTERNATE;
		keyseq[K_F(i + 20) - K_MIN].flags |= KF_HASALTER;
	}
	sortkeyseq();
}

int getdefkeyseq(kp)
keyseq_t *kp;
{
	CONST char *cp;

	if (kp -> code < K_MIN || kp -> code > K_MAX) /*EMPTY*/;
	else if (!(cp = defkeyseq[kp -> code - K_MIN])) /*EMPTY*/;
	else {
		kp -> str = (char *)cp;
		kp -> len = strlen(cp);
		return(0);
	}
	kp -> len = 0;

	return(-1);
}

VOID setkeyseq(n, str, len)
int n;
char *str;
int len;
{
	int i;

	for (i = 0; i <= K_MAX - K_MIN; i++) if (keyseq[i].code == n) {
		if (keyseq[i].str) free(keyseq[i].str);
		keyseq[i].str = str;
		keyseq[i].len = len;
		keyseq[i].flags |= KF_DEFINED;
		break;
	}
	if (i > K_MAX - K_MIN) {
		free(str);
		return;
	}

	if (str) {
		for (i = 0; i <= K_MAX - K_MIN; i++) {
			if (alternate(keyseq[i].code) == n
			|| !(keyseq[i].str) || keyseq[i].len != len)
				continue;
			if (!memcmp(str, keyseq[i].str, len)) {
				free(keyseq[i].str);
				keyseq[i].str = NULL;
				keyseq[i].len = 0;
				keyseq[i].flags &= ~KF_DEFINED;
			}
		}
		if (str[0] == cc_erase && !(str[1])) cc_erase = K_UNDEF;
	}
	sortkeyseq();
}

int getkeyseq(kp)
keyseq_t *kp;
{
	int i;

	for (i = 0; i <= K_MAX - K_MIN; i++) {
		if (keyseq[i].code == kp -> code) {
			memcpy((char *)kp, (char *)&(keyseq[i]),
				sizeof(keyseq_t));
			return(0);
		}
	}
	kp -> len = 0;

	return(-1);
}

keyseq_t *copykeyseq(list)
keyseq_t *list;
{
	int i;

	if (list) {
		for (i = 0; i <= K_MAX - K_MIN; i++) {
			keyseq[i].code = list[i].code;
			keyseq[i].len = list[i].len;
			keyseq[i].flags = list[i].flags;
			if (keyseq[i].str) free(keyseq[i].str);
			if (!(list[i].len)) keyseq[i].str = NULL;
			else {
				keyseq[i].str =
					(char *)malloc(list[i].len + 1);
				if (!keyseq[i].str) terror("malloc()");
				memcpy(keyseq[i].str, list[i].str,
					list[i].len);
			}
		}
		sortkeyseq();
	}
	else {
		list = (keyseq_t *)malloc((K_MAX - K_MIN + 1)
			* sizeof(keyseq_t));
		if (!list) terror("malloc()");
		for (i = 0; i <= K_MAX - K_MIN; i++) {
			list[i].code = keyseq[i].code;
			list[i].len = keyseq[i].len;
			list[i].flags = keyseq[i].flags;
			if (!(keyseq[i].len)) list[i].str = NULL;
			else {
				list[i].str =
					(char *)malloc(keyseq[i].len + 1);
				if (!list[i].str) terror("malloc()");
				memcpy(list[i].str, keyseq[i].str,
					keyseq[i].len);
			}
		}
	}

	return(list);
}

VOID freekeyseq(list)
keyseq_t *list;
{
	int i;

	if (!list) return;
	for (i = 0; i <= K_MAX - K_MIN; i++)
		if (list[i].str) free(list[i].str);
	free(list);
}
#endif	/* !MSDOS */

int tputparam(n, arg1, arg2, cnt)
int n, arg1, arg2, cnt;
{
	char *cp;

	if (n < 0 || n >= MAXTERMSTR) return(-1);
	if (!(termstr[n])) return(-1);
	if (!(cp = tparamstr(termstr[n], arg1, arg2))) return(-1);

	n = 0;
	if (!*cp) n = -1;
	else tputs2(cp, cnt);
	free(cp);

	return(n);
}

VOID initterm(VOID_A)
{
	if (!(termflags & F_TERMENT)) getterment(NULL);
	VOID_C termmode(1);
	if ((termflags & F_TERMENT) && !dumbterm) {
		putterm(T_KEYPAD);
		tflush();
	}
	termflags |= F_INITTERM;
}

VOID endterm(VOID_A)
{
	if (!(termflags & F_INITTERM)) return;
	VOID_C termmode(0);
	if ((termflags & F_TERMENT) && !dumbterm) {
		putterm(T_NOKEYPAD);
		tflush();
	}
	termflags &= ~F_INITTERM;
}

VOID putterm(n)
int n;
{
	if (n < 0 || n >= MAXTERMSTR) return;
	if (!(termstr[n])) return;

	tputs2(termstr[n], 1);
}

#if	MSDOS && defined (USEVIDEOBIOS)
static VOID NEAR bioslocate(x, y)
int x, y;
{
	__term_regs reg;

	reg.x.ax = 0x0200;
	reg.h.bh = getbiosbyte(CURRPAGE);
	reg.h.dh = y - 1;
	reg.h.dl = x - 1;
	intbios(VIDEOBIOS, &reg);
}

static VOID NEAR biosscroll(d, sx, sy, ex, ey)
int d, sx, sy, ex, ey;
{
	__term_regs reg;

	if (sx > ex || sy > ey) return;
	if (d >= 0) {
		reg.h.ah = 0x06;
		reg.h.al = d;
	}
	else {
		reg.h.ah = 0x07;
		reg.h.al = -d;
	}
	reg.h.bh = videoattr;
	reg.h.ch = sy - 1;
	reg.h.cl = sx - 1;
	reg.h.dh = ey - 1;
	reg.h.dl = ex - 1;
	intbios(VIDEOBIOS, &reg);
}

static VOID NEAR biosputch(c, n)
int c, n;
{
	__term_regs reg;

	reg.h.ah = 0x09;
	reg.h.al = (c & 0xff);
	reg.h.bh = getbiosbyte(CURRPAGE);
	reg.h.bl = videoattr;
	reg.x.cx = n;
	intbios(VIDEOBIOS, &reg);
}

static VOID NEAR bioscurstype(n)
int n;
{
	__term_regs reg;

	reg.x.ax = 0x0300;
	reg.h.bh = getbiosbyte(CURRPAGE);
	intbios(VIDEOBIOS, &reg);
	reg.x.ax = 0x0100;
	reg.x.cx &= 0x1f1f;
	reg.x.cx |= (n & 0x6000);
	intbios(VIDEOBIOS, &reg);
}

int Xputch(c)
int c;
{
	static int needscroll = 0;
	int n, x, y, w, h;

	if (c == '\007') {
		bdos(0x06, '\007', 0);
		return(c);
	}

	getxy(&x, &y);
	w = getbiosbyte(SCRWIDTH);
	h = getbiosbyte(SCRHEIGHT) + 1;

	if (c == '\b') {
		if (x > 1) x--;
		needscroll = 0;
	}
	else if (c == '\t') {
		if (x >= w && y >= h && needscroll) {
			x = 1;
			biosscroll(1, 1, 1, w, h);
			bioslocate(x, y);
		}
		needscroll = 0;

		n = 8 - ((x - 1) % 8);
		if (x + n <= w) x += n;
		else {
			n = w - x + 1;
			x = 1;
			if (y < h) y++;
			else {
				x = w;
				needscroll = 1;
			}
		}
		biosputch(' ', n);
	}
	else if (c == '\r') {
		x = 1;
		needscroll = 0;
	}
	else if (c == '\n') {
		if (y < h) y++;
		else biosscroll(1, 1, 1, w, h);
		needscroll = 0;
	}
	else {
		if (x >= w && y >= h && needscroll) {
			x = 1;
			biosscroll(1, 1, 1, w, h);
			bioslocate(x, y);
		}
		needscroll = 0;

		if (x < w) x++;
		else {
			x = 1;
			if (y < h) y++;
			else {
				x = w;
				needscroll = 1;
			}
		}
		biosputch(c, 1);
	}
	bioslocate(x, y);

	return(c);
}

#else	/* !MSDOS || !USEVIDEOBIOS */

int Xputch(c)
int c;
{
	c = Xfputc(c, ttyout);
	if (c == EOF) return(-1);

	return(c);
}
#endif	/* !MSDOS || !USEVIDEOBIOS */

#if	MSDOS
# ifdef	USEVIDEOBIOS
static VOID NEAR chgattr(n)
int n;
{
	switch (n) {
		case 0:
			videoattr = 0x07;
			break;
		case 1:
			videoattr |= 0x08;
			break;
		case 4:
			videoattr &= 0xf0;
			videoattr |= 0x01;
			break;
		case 5:
			videoattr |= 0x80;
			break;
		case 7:
			videoattr = 0x70;
			break;
		case 8:
			videoattr = 0x08;
			break;
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
			videoattr &= 0xf0;
			n -= 30;
			if (n & 1) videoattr |= 0x04;
			if (n & 2) videoattr |= 0x02;
			if (n & 4) videoattr |= 0x01;
			break;
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
			videoattr &= 0x0f;
			n -= 40;
			if (n & 1) videoattr |= 0x40;
			if (n & 2) videoattr |= 0x20;
			if (n & 4) videoattr |= 0x10;
			break;
		default:
			break;
	}
}

static int NEAR evalcsi(s)
CONST char *s;
{
	static int savex, savey;
	int i, x, y, w, h, n1, n2;

	i = 0;
	if (s[i] == '>') {
		i++;
		if ((n1 = getnum(s, &i)) < 0) i = -1;
		else if (s[i] == 'h') {
			if (n1 == 5) bioscurstype(0x2000);
			else i = -1;
		}
		else if (s[i] == 'l') {
			if (n1 == 5) bioscurstype(0x0000);
			else i = -1;
		}
		else i = -1;
	}
	else {
		getxy(&x, &y);
		w = getbiosbyte(SCRWIDTH);
		h = getbiosbyte(SCRHEIGHT) + 1;

		n1 = getnum(s, &i);
		if (s[i] != ';') n2 = -1;
		else {
			i++;
			n2 = getnum(s, &i);
		}

		switch (s[i]) {
			case 'A':
				if (n1 <= 0) n1 = 1;
				if ((y -= n1) <= 0) y = 1;
				bioslocate(x, y);
				break;
			case 'B':
				if (n1 <= 0) n1 = 1;
				if ((y += n1) > h) y = h;
				bioslocate(x, y);
				break;
			case 'C':
				if (n1 <= 0) n1 = 1;
				if ((x += n1) > w) x = w;
				bioslocate(x, y);
				break;
			case 'D':
				if (n1 <= 0) n1 = 1;
				if ((x -= n1) <= 0) x = 1;
				bioslocate(x, y);
				break;
			case 'H':
			case 'f':
				x = n2;
				y = n1;
				if (x <= 0) x = 1;
				else if (x > w) x = w;
				if (y <= 0) y = 1;
				else if (y > h) y = h;
				bioslocate(x, y);
				break;
			case 'm':
				if (n1 < 0) chgattr(0);
				else {
					chgattr(n1);
					if (n2 >= 0) chgattr(n2);
				}
				break;
			case 's':
				savex = x;
				savey = y;
				break;
			case 'u':
				x = savex;
				y = savey;
				if (x <= 0) x = 1;
				else if (x > w) x = w;
				if (y <= 0) y = 1;
				else if (y > h) y = h;
				bioslocate(savex, savey);
				break;
			case 'L':
				if (n1 <= 0) n1 = 1;
				biosscroll(-n1, 1, y, w, h);
				bioslocate(1, y);
				break;
			case 'M':
				if (n1 <= 0) n1 = 1;
				biosscroll(n1, 1, y, w, h);
				bioslocate(1, y);
				break;
			case 'J':
				switch (n1) {
					case -1:
					case 0:
						biosscroll(0, x, y, w, y);
						biosscroll(0, 1, y + 1, w, h);
						break;
					case 1:
						biosscroll(0, 1, y, x, y);
						biosscroll(0, 1, 1, w, y - 1);
						break;
					case 2:
						biosscroll(0, 1, 1, w, h);
						bioslocate(1, 1);
						break;
					default:
						break;
				}
				break;
			case 'K':
				switch (n1) {
					case -1:
					case 0:
						biosscroll(0, x, y, w, y);
						break;
					case 1:
						biosscroll(0, 1, y, x, y);
						break;
					case 2:
						biosscroll(0, 1, y, w, y);
						break;
					default:
						break;
				}
				break;
			default:
				i = -1;
				break;
		}
	}

	return(i);
}

VOID Xcputs(s)
CONST char *s;
{
	if (!s) return;

	for (; *s; s++) VOID_C Xputch(*s);
}

/*ARGSUSED*/
VOID tputs2(s, n)
CONST char *s;
int n;
{
	if (!s) return;

	for (; *s; s++) {
		if (*s != '\033' || s[1] != '[') /*EMPTY*/;
		else if ((n = evalcsi(&(s[2]))) >= 0) {
			s += n + 2;
			continue;
		}

		VOID_C Xputch(*s);
	}
}

# else	/* !USEVIDEOBIOS */

VOID Xcputs(s)
CONST char *s;
{
	int x, y;

	if (!s) return;

	for (; *s; s++) {
		if (*s != '\t') {
			VOID_C Xputch(*s);
			continue;
		}

		keyflush();
		if (getxy(&x, &y) < 0) x = 1;
		do {
			VOID_C Xputch(' ');
		} while (x++ % 8);
	}
}

/*ARGSUSED*/
VOID tputs2(s, n)
CONST char *s;
int n;
{
	if (!s) return;

	VOID_C Xfputs(s, ttyout);
}
# endif	/* !USEVIDEOBIOS */

/*ARGSUSED*/
int kbhit2(usec)
long usec;
{
# if	defined (NOTUSEBIOS) || defined (NOSELECT)
	union REGS reg;
# else	/* !NOTUSEBIOS && !NOSELECT */
#  ifndef	PC98
	struct timeval tv;
	int n;
#  endif
# endif	/* !NOTUSEBIOS && !NOSELECT */

	if (ungetnum > 0) return(1);
# ifdef	NOTUSEBIOS
	reg.x.ax = 0x4406;
	reg.x.bx = ttyio;
	putterm(T_METAMODE);
	int86(0x21, &reg, &reg);
	putterm(T_NOMETAMODE);

	return((reg.x.flags & 1) ? 0 : reg.h.al);
# else	/* !NOTUSEBIOS */
#  ifdef	PC98
	reg.h.ah = 0x01;
	int86(0x18, &reg, &reg);

	return(reg.h.bh != 0);
#  else	/* !PC98 */
#   ifdef	NOSELECT
	reg.h.ah = 0x01;
	int86(0x16, &reg, &reg);

	return((reg.x.flags & 0x40) ? 0 : 1);
#   else	/* !NOSELECT */
	tv.tv_sec = usec / 1000000L;
	tv.tv_usec = usec % 1000000L;
	if ((n = sureselect(1, &ttyio, NULL, &tv, SEL_TTYIO)) < 0)
		terror("select()");

	return(n);
#   endif	/* !NOSELECT */
#  endif	/* !PC98 */
# endif	/* !NOTUSEBIOS */
}

int Xgetch(VOID_A)
{
# ifdef	NOTUSEBIOS
	u_short key, top;
# endif
# if	defined (PC98) && !defined (NOTUSEBIOS)
	union REGS reg;
# endif
# if	defined (PC98) || defined (NOTUSEBIOS)
	int ch;
# endif

	if (ttyio < 0) return(EOF);
	if (ungetnum > 0) return((int)ungetbuf[--ungetnum]);

# ifndef	NOTUSEBIOS
#  ifdef	PC98
	reg.h.ah = 0x00;
	int86(0x18, &reg, &reg);

	if (!(ch = reg.h.al)) VOID_C Xungetch(reg.h.ah);

	return(ch);
#  else	/* !PC98 */
	return(bdos(0x07, 0x00, 0) & 0xff);
#  endif	/* !PC98 */
# else	/* NOTUSEBIOS */
	disable();
	top = keybuftop;
	for (;;) {
		if (top == getkeybuf(KEYBUFWORKEND)) {
			key = 0xffff;
			break;
		}
		key = getkeybuf(top);
#  ifdef	PC98
		if ((ch = (key & 0xff))) {
			if (ch < 0x80 || (ch > 0x9f && ch < 0xe0)) break;
			key &= 0xff00;
			if (Xstrchr(metakey, key >> 8)) break;
		}
#  else
		if ((ch = (key & 0xff)) && ch != 0xe0 && ch != 0xf0) break;
		key &= 0xff00;
#  endif
		if (Xstrchr(specialkey, key >> 8)) break;
		if ((top += 2) >= KEYBUFWORKMAX) top = KEYBUFWORKMIN;
	}
	putterm(T_METAMODE);
	ch = (bdos(0x07, 0x00, 0) & 0xff);
	putterm(T_NOMETAMODE);
	keybuftop = getkeybuf(KEYBUFWORKTOP);
	if (!(key & 0xff)) {
		while (kbhit2(1000000L / SENSEPERSEC)) {
			if (keybuftop != getkeybuf(KEYBUFWORKTOP)) break;
			putterm(T_METAMODE);
			bdos(0x07, 0x00, 0);
			putterm(T_NOMETAMODE);
		}
		ch = '\0';
		VOID_C Xungetch(key >> 8);
	}
	enable();

	return(ch);
# endif	/* NOTUSEBIOS */
}

# if	!defined (DJGPP) || defined (NOTUSEBIOS) || defined (PC98)
static VOID NEAR dosgettime(tbuf)
u_char tbuf[];
{
	union REGS reg;

	reg.x.ax = 0x2c00;
	intdos(&reg, &reg);
	tbuf[0] = reg.h.ch;
	tbuf[1] = reg.h.cl;
	tbuf[2] = reg.h.dh;
}
# endif

/*ARGSUSED*/
int getkey2(sig, code, timeout)
int sig, code, timeout;
{
# if	!defined (DJGPP) || defined (NOTUSEBIOS) || defined (PC98)
	static u_char tbuf1[3] = {0xff, 0xff, 0xff};
	u_char tbuf2[3];
# endif
# if	defined (DJGPP) && (DJGPP >= 2)
	static int count = SENSEPERSEC;
# endif
	int i, ch;

	timeout *= SENSEPERSEC;
# if	defined (DJGPP) && !defined (NOTUSEBIOS) && !defined (PC98)
	do {
		i = kbhit2(1000000L / SENSEPERSEC);
#  ifndef	NOSELECT
		if (sig && !(--count)) {
			count = SENSEPERSEC;
			raise(sig);
		}
#  endif
		if (timeout && !--timeout) return(K_TIMEOUT);
		if (keywaitfunc && (ch = (*keywaitfunc)()) < 0) return(ch);
	} while (!i);

# else	/* !DJGPP || NOTUSEBIOS || PC98 */
	if (tbuf1[0] == 0xff) dosgettime(tbuf1);
	while (!kbhit2(1000000L / SENSEPERSEC)) {
		dosgettime(tbuf2);
		if (memcmp((char *)tbuf1, (char *)tbuf2, sizeof(tbuf1))) {
#  if	!defined (DJGPP) || (DJGPP >= 2)
			if (sig) raise(sig);
#  endif
			memcpy((char *)tbuf1, (char *)tbuf2, sizeof(tbuf1));
		}
		if (keywaitfunc && (ch = (*keywaitfunc)()) < 0) return(ch);
	}
# endif	/* !DJGPP || NOTUSEBIOS || PC98 */
	if ((ch = Xgetch()) == EOF) return(K_NOKEY);

	if (ch) switch (ch) {
		case '\010':
			ch = K_BS;
			break;
		case '\177':
			ch = K_DC;
			break;
		default:
			break;
	}
# if	defined (PC98) || defined (NOTUSEBIOS) || !defined (NOSELECT)
	else if (!kbhit2(WAITKEYPAD * 1000L)) /*EMPTY*/;
# endif
	else if ((ch = Xgetch()) == EOF) ch = K_NOKEY;
	else {
		for (i = 0; i < SPECIALKEYSIZ; i++)
			if (ch == specialkey[i]) return(specialkeycode[i]);
		for (i = 0; i <= 'z' - 'a'; i++)
			if (ch == metakey[i]) return(mkmetakey('a' + i));
		ch = K_NOKEY;
	}

	return(ch);
}

int ungetkey2(c, desc)
int c, desc;
{
	if (c == EOF || ttyio < 0) return(EOF);
	if (ungetnum >= arraysize(ungetbuf)) return(EOF);
	if (!desc) ungetbuf[ungetnum] = (u_char)c;
	else {
		memmove((char *)&(ungetbuf[1]),
			(char *)&(ungetbuf[0]), ungetnum * sizeof(u_char));
		ungetbuf[0] = (u_char)c;
	}
	ungetnum++;

	return(c);
}

/*ARGSUSED*/
int setscroll(s, e)
int s, e;
{
	return(0);
}

VOID locate(x, y)
int x, y;
{
	tprintf(termstr[C_LOCATE], 1, ++y, ++x);
}

VOID tflush(VOID_A)
{
#ifndef	USEVIDEOBIOS
	VOID_C Xfflush(ttyout);
#endif
}

char *getwsize(xmax, ymax)
int xmax, ymax;
{
	int x, y;

	keyflush();
	if (maxlocate(&y, &x) < 0) x = y = -1;

	if (x > 0) {
		n_lastcolumn = (n_lastcolumn < n_column) ? x - 1 : x;
		n_column = x;
	}
	if (y > 0) n_line = y;

	if (n_column <= 0 || (xmax > 0 && n_column < xmax))
		return("Column size too small");
	if (n_line <= 0 || (ymax > 0 && n_line < ymax))
		return("Line size too small");

	return(NULL);
}

/*ARGSUSED*/
VOID setwsize(fd, xmax, ymax)
int fd, xmax, ymax;
{
}

#else	/* !MSDOS */

VOID Xcputs(s)
CONST char *s;
{
	if (!s) return;

	VOID_C Xfputs(s, ttyout);
}

static int putch2(c)
tputs_t c;
{
	return(Xfputc(c & 0x7f, ttyout));
}

VOID tputs2(s, n)
CONST char *s;
int n;
{
	tputs(s, n, putch2);
}

VOID putterms(n)
int n;
{
	if (n < 0 || n >= MAXTERMSTR) return;
	if (!(termstr[n])) return;

	tputs2(termstr[n], n_line);
}

VOID checksuspend(VOID_A)
{
	if (suspended) ttyiomode((isttyiomode) ? isttyiomode - 1 : 0);
	suspended = 0;
}

int kbhit2(usec)
long usec;
{
# ifndef	NOSELECT
	struct timeval tv;
	int n;
# endif

	if (ttyio < 0) return(0);
	if (ungetnum > 0) return(1);
# ifdef	NOSELECT
	return((usec) ? 1 : 0);
# else
	tv.tv_sec = usec / 1000000L;
	tv.tv_usec = usec % 1000000L;
	if ((n = sureselect(1, &ttyio, NULL, &tv, SEL_TTYIO)) < 0)
		terror("select()");

	return(n);
# endif
}

int Xgetch(VOID_A)
{
	u_char ch;
	int n;

	if (ttyio < 0) return(EOF);
	if (ungetnum > 0) return((int)ungetbuf[--ungetnum]);
	for (;;) {
		checksuspend();
		n = read(ttyio, &ch, sizeof(u_char));
		if (n >= 0 || errno != EINTR) break;
	}
	if (n < (int)sizeof(u_char)) return(EOF);

	return((int)ch);
}

static kstree_t *NEAR searchkeyseq(key, p, ptr)
int key;
kstree_t *p;
int ptr;
{
	int i;

	if (p && p -> next) for (i = 0; i < p -> num; i++)
		if (key == keyseq[p -> next[i].key].str[ptr])
			return(&(p -> next[i]));

	return(NULL);
}

/*ARGSUSED*/
int getkey2(sig, code, timeout)
int sig, code, timeout;
{
	static int count = SENSEPERSEC;
	kstree_t *p;
	keyseq_t *seq;
	int n, ch, key, wasmeta;

	timeout *= SENSEPERSEC;
	do {
		key = kbhit2(1000000L / SENSEPERSEC);
		if (sig && !(--count)) {
			count = SENSEPERSEC;
			kill(getpid(), sig);
		}
		if (timeout && !--timeout) return(K_TIMEOUT);
		if (keywaitfunc && (ch = (*keywaitfunc)()) < 0) return(ch);
	} while (!key);

	if ((key = ch = Xgetch()) == EOF) return(K_NOKEY);
# if	defined (DEP_KCONV) || defined (CODEEUC)
	else if (key != C_EKANA) /*EMPTY*/;
#  ifdef	DEP_KCONV
#   ifdef	CODEEUC
	else if (code != EUC && code != NOCNV) /*EMPTY*/;
#   else
	else if (code != EUC) /*EMPTY*/;
#   endif
#  endif	/* DEP_KCONV */
	else {
		ch = (kbhit2(WAITKANJI * 1000L)) ? Xgetch() : EOF;
		if (ch == EOF) return(key);
		if (Xiskana(ch)) return(mkekana(ch));
		VOID_C Xungetch(ch);
		return(key);
	}
# endif	/* DEP_KCONV || CODEEUC */

	if (cc_erase != K_UNDEF && key == cc_erase) return(K_BS);
	n = wasmeta = 0;

	if (key == K_ESC) {
		ch = (kbhit2(WAITKEYPAD * 1000L)) ? Xgetch() : EOF;
		if (ch == EOF) return(key);
		if (Xisalpha(ch)) wasmeta = mkmetakey(ch);
		if (!(p = searchkeyseq(key, keyseqtree, n))) {
			if (wasmeta) return(wasmeta);
			VOID_C Xungetch(ch);
			return(key);
		}
		seq = &(keyseq[p -> key]);
		if (++n >= seq -> len) return(seq -> code);
	}
	else {
		if (!(p = searchkeyseq(key, keyseqtree, n))) return(key);
		seq = &(keyseq[p -> key]);
		if (++n >= seq -> len) return(seq -> code);
		ch = (kbhit2(WAITKEYPAD * 1000L)) ? Xgetch() : EOF;
		if (ch == EOF) return(key);
	}

	while ((p = searchkeyseq(ch, p, n))) {
		seq = &(keyseq[p -> key]);
		if (++n >= seq -> len) return(seq -> code);
		ch = (kbhit2(WAITKEYPAD * 1000L)) ? Xgetch() : EOF;
		if (ch == EOF) break;
	}

	VOID_C Xungetch(ch);
	while (--n > 0) VOID_C Xungetch(seq -> str[n]);
	if (wasmeta) {
		if (ungetnum) ungetnum--;
		return(wasmeta);
	}

	return(key);
}

int getkey3(sig, code, timeout)
int sig, code, timeout;
{
	int ch;

	if ((ch = getkey2(sig, code, timeout)) < 0) return(ch);
	if (ch >= K_F('*') && ch < K_DL) {
		if (ch == K_F('?')) ch = K_CR;
		else ch -= K_F0;
	}

	return(alternate(ch));
}

int ungetkey2(c, desc)
int c, desc;
{
# ifdef	TIOCSTI
	u_char ch;
# endif

	if (c == EOF || ttyio < 0) return(EOF);
	if (!desc) return(Xungetch(c));
# ifdef	TIOCSTI
	ch = c;
	VOID_C Xioctl(ttyio, TIOCSTI, &ch);
# else
	if (ungetnum >= arraysize(ungetbuf)) return(EOF);
	memmove((char *)&(ungetbuf[1]), (char *)&(ungetbuf[0]),
		ungetnum * sizeof(u_char));
	ungetbuf[0] = (u_char)c;
	ungetnum++;
# endif

	return(c);
}

int setscroll(s, e)
int s, e;
{
	return tputparam(T_SCROLL, s, e, 1);
}

VOID locate(x, y)
int x, y;
{
	VOID_C tputparam(C_LOCATE, y, x, 1);
}

VOID tflush(VOID_A)
{
	VOID_C Xfflush(ttyout);
}

char *getwsize(xmax, ymax)
int xmax, ymax;
{
# ifndef	NOTERMWSIZE
	termwsize_t ws;
# endif
	int x, y, tx, ty;

	x = y = -1;
# ifndef	NOTERMWSIZE
	memset((char *)&ws, 0, sizeof(ws));
	if (Xioctl(ttyio, REQGETWS, &ws) >= 0) {
#  ifdef	TIOCGWINSZ
		if (ws.ws_col > 0) x = ws.ws_col;
		if (ws.ws_row > 0) y = ws.ws_row;
#  else	/* !TIOCGWINSZ */
#   ifdef	WIOCGETD
		if (ws.uw_width > 0 && ws.uw_hs > 0)
			x = ws.uw_width / ws.uw_hs;
		if (ws.uw_height > 0 && ws.uw_vs > 0)
			y = ws.uw_height / ws.uw_vs;
#   else	/* !WIOCGETD */
#    ifdef	TIOCGSIZE
		if (ws.ts_cols > 0) x = ws.ts_cols;
		if (ws.ts_lines > 0) y = ws.ts_lines;
#    endif	/* !TIOCGSIZE */
#   endif	/* !WIOCGETD */
#  endif	/* !TIOCGWINSZ */
	}
# endif	/* !NOTERMWSIZE */

	if (dumbterm) /*EMPTY*/;
	else if (usegetcursor || x < 0 || y < 0) {
		if (usegetcursor) VOID_C setscroll(-1, -1);
		else VOID_C setscroll(0, 998);
		if (maxlocate(&ty, &tx) >= 0 && (tx > x || ty > y)) {
			if (tx > x) x = tx;
			if (ty > y) y = ty;
			setwsize(ttyio, x, y);
		}
	}

	if (x > 0) {
		n_lastcolumn = (n_lastcolumn < n_column) ? x - 1 : x;
		n_column = x;
	}
	if (y > 0) n_line = y;

	if (n_column <= 0 || (xmax > 0 && n_column < xmax))
		return("Column size too small");
	if (n_line <= 0 || (ymax > 0 && n_line < ymax))
		return("Line size too small");

	if (xmax > 0 && ymax > 0) VOID_C setscroll(0, n_line - 1);

	return(NULL);
}

VOID setwsize(fd, xmax, ymax)
int fd, xmax, ymax;
{
# ifndef	NOTERMWSIZE
	termwsize_t ws;

	memset((char *)&ws, 0, sizeof(ws));
	VOID_C Xioctl(fd, REQGETWS, &ws);

#  ifdef	TIOCGWINSZ
	if (ws.ws_col <= 0 || ws.ws_xpixel <= 0) ws.ws_xpixel = 0;
	else ws.ws_xpixel += (xmax - ws.ws_col) * (ws.ws_xpixel / ws.ws_col);
	if (ws.ws_row <= 0 || ws.ws_ypixel <= 0) ws.ws_ypixel = 0;
	else ws.ws_ypixel += (ymax - ws.ws_row) * (ws.ws_ypixel / ws.ws_row);
	ws.ws_col = xmax;
	ws.ws_row = ymax;
#  else	/* !TIOCGWINSZ */
#   ifdef	WIOCGETD
	if (ws.uw_hs <= 0 || ws.uw_vs <= 0) return;
	ws.uw_width = xmax * ws.uw_hs;
	ws.uw_height = ymax * ws.uw_vs;
#   else	/* !WIOCGETD */
#    ifdef	TIOCGSIZE
	ws.ts_cols = xmax;
	ws.ts_lines = ymax;
#    endif	/* !TIOCGSIZE */
#   endif	/* !WIOCGETD */
#  endif	/* !TIOCGWINSZ */

	VOID_C Xioctl(fd, REQSETWS, &ws);
# endif	/* !NOTERMWSIZE */
}
#endif	/* !MSDOS */

int cvasprintf(sp, fmt, args)
char **sp;
CONST char *fmt;
va_list args;
{
	int n;

	n = Xvasprintf(sp, fmt, args);
	if (n < 0) terror("malloc()");

	return(n);
}

#ifdef	USESTDARGH
/*VARARGS1*/
int Xcprintf(CONST char *fmt, ...)
#else
/*VARARGS1*/
int Xcprintf(fmt, va_alist)
CONST char *fmt;
va_dcl
#endif
{
	va_list args;
	char *buf;
	int n;

	VA_START(args, fmt);
	n = cvasprintf(&buf, fmt, args);
	va_end(args);

	Xcputs(buf);
	free(buf);

	return(n);
}

#ifdef	USESTDARGH
/*VARARGS2*/
VOID tprintf(CONST char *fmt, int cnt, ...)
#else
/*VARARGS2*/
VOID tprintf(fmt, cnt, va_alist)
CONST char *fmt;
int cnt;
va_dcl
#endif
{
	va_list args;
	char *buf;

	VA_START(args, cnt);
	VOID_C cvasprintf(&buf, fmt, args);
	va_end(args);

	tputs2(buf, cnt);
	free(buf);
}

VOID cputnl(VOID_A)
{
	Xcputs("\r\n");
	tflush();
}

int kanjiputs(s)
CONST char *s;
{
	return(Xcprintf("%k", s));
}

VOID attrputs(s, isstandout)
CONST char *s;
int isstandout;
{
	if (!s || !*s) return;

	if (isstandout) putterm(T_STANDOUT);
	Xcputs(s);
	if (isstandout) putterm(END_STANDOUT);
}

#ifdef	USESTDARGH
/*VARARGS2*/
int attrprintf(CONST char *fmt, int isstandout, ...)
#else
/*VARARGS2*/
int attrprintf(fmt, isstandout, va_alist)
CONST char *fmt;
int isstandout;
va_dcl
#endif
{
	va_list args;
	char *buf;
	int n;

	VA_START(args, isstandout);
	n = cvasprintf(&buf, fmt, args);
	va_end(args);

	attrputs(buf, isstandout);
	free(buf);

	return(n);
}

int attrkanjiputs(s, isstandout)
CONST char *s;
int isstandout;
{
	return(attrprintf("%k", isstandout, s));
}

VOID chgcolor(color, reverse)
int color, reverse;
{
	int fg, bg;

	if (reverse) {
		fg = (color == ANSI_BLACK) ? ANSI_WHITE : ANSI_BLACK;
		bg = color;
	}
	else {
		fg = color;
		bg = -1;
	}

	if (tputparam(T_FGCOLOR, fg, 0, 1) < 0)
		tprintf("\033[%dm", 1, fg + ANSI_NORMAL);

	if (bg < 0) /*EMPTY*/;
	else if (tputparam(T_BGCOLOR, bg, 0, 1) < 0)
		tprintf("\033[%dm", 1, bg + ANSI_REVERSE);
}

VOID movecursor(n1, n2, c)
int n1, n2, c;
{
	if (tputparam(n1, c, c, n_line) < 0) while (c--) putterms(n2);
}
