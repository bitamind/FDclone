/*
 *	frontend.c
 *
 *	frontend of terminal emulation
 */

#include "fd.h"
#include "wait.h"
#include "parse.h"
#include "func.h"
#include "funcno.h"
#include "kanji.h"
#include "system.h"
#include "termemu.h"

#ifdef	DEP_IME
#include "roman.h"
#endif

#ifdef	DEP_IME
#define	MAXPTYMENU		5
#else
#define	MAXPTYMENU		4
#endif

#ifdef	DEP_PTY

#if	defined (DEP_ORIGSHELL) && !defined (NOALIAS)
extern int addalias __P_((char *, char *));
extern int deletealias __P_((CONST char *));
#endif

extern CONST functable funclist[];
extern int internal_status;
extern int fdmode;
extern int fdflags;
extern int wheader;
extern int lcmdline;
extern int maxcmdline;
extern p_id_t emupid;
extern int emufd;
extern char *ptytmpfile;
#ifdef	DEP_IME
extern int ime_cont;
#endif

static int waitpty __P_((VOID_A));
static VOID NEAR _XXcputs __P_((CONST char *, int));
static VOID NEAR _Xattrputs __P_((CONST char *, int, int));
static int NEAR ptygetkey __P_((VOID_A));
static int NEAR recvvar __P_((int, char ***));
#ifdef	DEP_ORIGSHELL
static int NEAR recvheredoc __P_((int, heredoc_t **));
static int NEAR recvrlist __P_((int, redirectlist **));
static int NEAR recvcommand __P_((int, command_t **, syntaxtree *));
static int NEAR recvstree __P_((int, syntaxtree **, syntaxtree *));
#endif
static VOID NEAR recvchild __P_((int));
static VOID NEAR sendkey __P_((int));

static int (*lastfunc)__P_((VOID_A)) = NULL;
static u_long lockflags = 0;


int waitstatus(pid, options, statusp)
p_id_t pid;
int options, *statusp;
{
	wait_pid_t w;
	p_id_t tmp;

	tmp = Xwait4(pid, &w, options, NULL);
	if (tmp < (p_id_t)0 || tmp != pid) return(-1);
	if (statusp) *statusp = (WIFSIGNALED(w))
		? WTERMSIG(w) + 128 : (WEXITSTATUS(w) & 0377);

	return(0);
}

static int waitpty(VOID_A)
{
	char result[MAXWINDOWS];
	int i, oldwin, fds[MAXWINDOWS];

	if (lastfunc && (*lastfunc)() < 0) return(-1);

	oldwin = win;
	for (i = 0; i < MAXWINDOWS; i++)
		fds[i] = (ptylist[i].pid) ? ptylist[i].pipe : -1;
	if (selectpty(MAXWINDOWS, fds, result, 0L) > 0) {
		for (i = 0; i < MAXWINDOWS; i++)
			if (result[i] && ptylist[i].pid) recvchild(i);
	}

	if (win != oldwin || !emupid) {
		recvchild(oldwin);
		return(-2);
	}
	if (!(ptylist[win].pid)) return(0);
	if (ptylist[win].status >= 0) return(-1);
	return(checkpty(win));
}

VOID Xttyiomode(isnl)
int isnl;
{
	int dupdumbterm;

	if (!emupid) {
		syncptyout(-1, -1);
		ttyiomode(isnl);
	}
	else {
		dupdumbterm = dumbterm;
		dumbterm = 1;
		ttyiomode(isnl);
		dumbterm = dupdumbterm;
		if (!dumbterm) Xputterm(T_KEYPAD);
	}
}

VOID Xstdiomode(VOID_A)
{
	int dupdumbterm;

	if (!emupid) {
		syncptyout(-1, -1);
		stdiomode();
	}
	else {
		dupdumbterm = dumbterm;
		dumbterm = 1;
		stdiomode();
		dumbterm = dupdumbterm;
		if (!dumbterm) Xputterm(T_NOKEYPAD);
	}
}

int Xtermmode(init)
int init;
{
	int mode, dupdumbterm;

	dupdumbterm = dumbterm;
#ifdef	DEP_ORIGSHELL
	if (isshptymode()) dumbterm = 1;
#endif
	if (isptymode()) dumbterm = 1;
	mode = termmode(init);
	dumbterm = dupdumbterm;

	return(mode);
}


int XXputch(c)
int c;
{
	if (!emupid) return(Xputch(c));
	else {
		sendword(emufd, TE_XPUTCH);
		sendword(emufd, c);

		return(c);
	}
}

static VOID NEAR _XXcputs(s, n)
CONST char *s;
int n;
{
	int len;

	if (!emupid) Xcputs(s);
	else {
		if (!s) return;
		sendword(emufd, TE_XCPUTS);
		len = strlen(s);
		sendword(emufd, len);
		sendword(emufd, n);
		sendbuf(emufd, s, len);
	}
}

VOID XXcputs(s)
CONST char *s;
{
	_XXcputs(s, -1);
}

VOID Xputterm(n)
int n;
{
	if (!emupid) putterm(n);
	else {
		sendword(emufd, TE_PUTTERM);
		sendword(emufd, n);
	}
}

VOID Xputterms(n)
int n;
{
	if (!emupid) putterms(n);
	else {
		sendword(emufd, TE_PUTTERMS);
		sendword(emufd, n);
	}
}

int Xsetscroll(min, max)
int min, max;
{
	if (!emupid) return(setscroll(min, max));
	else {
		sendword(emufd, TE_SETSCROLL);
		sendword(emufd, min);
		sendword(emufd, max);

		return(0);
	}
}

VOID Xlocate(x, y)
int x, y;
{
	if (!emupid) locate(x, y);
	else {
		sendword(emufd, TE_LOCATE);
		sendword(emufd, x);
		sendword(emufd, y);
	}
}

VOID Xtflush(VOID_A)
{
	if (!emupid) tflush();
}

#ifdef	USESTDARGH
/*VARARGS1*/
int XXcprintf(CONST char *fmt, ...)
#else
/*VARARGS1*/
int XXcprintf(fmt, va_alist)
CONST char *fmt;
va_dcl
#endif
{
	va_list args;
	char *buf;
	int n;

	VA_START(args, fmt);
	n = Xvasprintf(&buf, fmt, args);
	va_end(args);
	if (n < 0) error("malloc()");

	_XXcputs(buf, n);
	Xfree(buf);

	return(n);
}

VOID Xcputnl(VOID_A)
{
	if (!emupid) cputnl();
	else {
		sendword(emufd, TE_CPUTNL);
		sendword(emufd, 0);
	}
}

int Xkanjiputs(s)
CONST char *s;
{
	return(XXcprintf("%k", s));
}

static VOID NEAR _Xattrputs(s, isstandout, n)
CONST char *s;
int isstandout, n;
{
	if (!s || !*s) return;

	if (isstandout) Xputterm(T_STANDOUT);
	_XXcputs(s, n);
	if (isstandout) Xputterm(END_STANDOUT);
}

VOID Xattrputs(s, isstandout)
CONST char *s;
int isstandout;
{
	_Xattrputs(s, isstandout, -1);
}

#ifdef	USESTDARGH
/*VARARGS2*/
int Xattrprintf(CONST char *fmt, int isstandout, ...)
#else
/*VARARGS2*/
int Xattrprintf(fmt, isstandout, va_alist)
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

	_Xattrputs(buf, isstandout, n);
	free(buf);

	return(n);
}

int Xattrkanjiputs(s, isstandout)
CONST char *s;
int isstandout;
{
	return(Xattrprintf("%k", isstandout, s));
}

VOID Xchgcolor(color, reverse)
int color, reverse;
{
	if (!emupid) chgcolor(color, reverse);
	else {
		sendword(emufd, TE_CHGCOLOR);
		sendword(emufd, color);
		sendword(emufd, reverse);
	}
}

VOID Xmovecursor(n1, n2, c)
int n1, n2, c;
{
	if (!emupid) movecursor(n1, n2, c);
	else {
		sendword(emufd, TE_MOVECURSOR);
		sendword(emufd, n1);
		sendword(emufd, n2);
		sendword(emufd, c);
	}
}

VOID changewin(n, pid)
int n;
p_id_t pid;
{
	int i;

	if (!emupid) return;

	sendword(emufd, TE_CHANGEWIN);
	sendword(emufd, n);
	sendbuf(emufd, &pid, sizeof(pid));
	if (pid) return;

	for (i = 0; i < MAXWINDOWS; i++) if (ptylist[i].pid) return;
	VOID_C waitstatus(emupid, 0, NULL);
	emupid = (p_id_t)0;
	killallpty();
}

VOID changewsize(h, n)
int h, n;
{
	int i;

	if (!emupid) return;

	sendword(emufd, TE_CHANGEWSIZE);
	sendword(emufd, h);
	sendword(emufd, n);
	for (i = 0; i < n; i++) sendword(emufd, winvar[i].v_fileperrow);
}

VOID insertwin(n, max)
int n, max;
{
	ptyinfo_t tmp;

	memcpy((char *)&tmp, (char *)&(ptylist[max - 1]), sizeof(ptyinfo_t));
	memmove((char *)&(ptylist[n + 1]), (char *)&(ptylist[n]),
		(max - 1 - n) * sizeof(ptyinfo_t));
	memcpy((char *)&(ptylist[n]), (char *)&tmp, sizeof(ptyinfo_t));

	if (!emupid) return;

	sendword(emufd, TE_INSERTWIN);
	sendword(emufd, n);
	sendword(emufd, max);
}

VOID deletewin(n, max)
int n, max;
{
	ptyinfo_t tmp;

	memcpy((char *)&tmp, (char *)&(ptylist[n]), sizeof(ptyinfo_t));
	memmove((char *)&(ptylist[n]), (char *)&(ptylist[n + 1]),
		(max - n) * sizeof(ptyinfo_t));
	memcpy((char *)&(ptylist[max]), (char *)&tmp, sizeof(ptyinfo_t));
	killpty(max, NULL);

	if (!emupid) return;

	sendword(emufd, TE_DELETEWIN);
	sendword(emufd, n);
	sendword(emufd, max);
}

#ifdef	DEP_KCONV
VOID changekcode(VOID_A)
{
	if (!emupid) return;

	sendword(emufd, TE_CHANGEKCODE);
	sendword(emufd, inputkcode);
	sendword(emufd, outputkcode);
}

VOID changeinkcode(VOID_A)
{
	ptylist[win].incode = (u_char)ptyinkcode;
	if (!emupid) return;

	sendword(emufd, TE_CHANGEINKCODE);
	sendword(emufd, win);
	sendword(emufd, ptyinkcode);
}

VOID changeoutkcode(VOID_A)
{
	ptylist[win].outcode = (u_char)ptyoutkcode;
	if (!emupid) return;

	sendword(emufd, TE_CHANGEOUTKCODE);
	sendword(emufd, win);
	sendword(emufd, ptyoutkcode);
}
#endif	/* DEP_KCONV */

static int NEAR ptygetkey(VOID_A)
{
#ifdef	DEP_IME
	static char buf[MAXKANJIBUF + 1] = "";
	static int next = 0;
	u_int w;
#endif
	CONST char *str[MAXPTYMENU];
	char *cp, *new;
	int n, c, ch, val[MAXPTYMENU];

#ifdef	DEP_IME
	if (next > 0) {
		if (next >= sizeof(buf) || !(c = (u_char)(buf[next++])))
			next = 0;
		else return(c);
	}
	if (ime_cont) {
		c = ime_inputkanji(sigalrm(1), buf);
		if (c < 0 || !ime_cont) movepos(filepos, 0);
		if (c != K_ESC) {
			if (c < 0) {
				next = 0;
				ime_inputkanji(0, NULL);
			}
			else if (!c && (c = (u_char)(buf[0]))) next = 1;
			return(c);
		}
	}
#endif	/* DEP_IME */

	for (;;) {
		n = -1;
		c = getkey2(sigalrm(1), inputkcode, 0);
		while (lockflags & (1 << win)) {
			kbhit2(1000000L / SENSEPERSEC);
			waitpty();
		}
#ifdef	DEP_ORIGSHELL
		if (isshptymode()) break;
#endif
		if (c < 0 || ptymenukey < 0 || alternate(c) != ptymenukey)
			break;

		str[0] = new = asprintf2(PTYAI_K, getkeysym(ptymenukey, 0));
		str[1] = PTYIC_K;
		str[2] = PTYBR_K;
#ifdef	DEP_IME
		str[3] = PTYKJ_K;
#endif
		str[MAXPTYMENU - 1] = PTYNW_K;
		val[0] = 0;
		val[1] = 1;
		val[2] = 2;
#ifdef	DEP_IME
		val[3] = 3;
#endif
		val[MAXPTYMENU - 1] = 4;

		n = 0;
		changewin(MAXWINDOWS, (p_id_t)-1);
		ch = selectstr(&n, (windows > 1) ? MAXPTYMENU : MAXPTYMENU - 1,
			0, str, val);
		movepos(filepos, 0);
		changewin(win, (p_id_t)-1);
		Xfree(new);

		if (ch != K_CR) continue;
		else if (!n) break;
		else if (n == 2) {
			killpty(win, NULL);
			rewritefile(1);
			c = -1;
			break;
		}
#ifdef	DEP_IME
		else if (n == 3) {
			c = ime_inputkanji(sigalrm(1), buf);
			if (c < 0 || !ime_cont) movepos(filepos, 0);
			if (c != K_ESC) {
				if (c < 0) {
					next = 0;
					ime_inputkanji(0, NULL);
				}
				else if (!c && (c = (u_char)(buf[0])))
					next = 1;
				break;
			}
		}
#endif	/* DEP_IME */
#ifndef	_NOSPLITWIN
		else if (n == 4) {
			VOID_C nextwin();
			c = -2;
			break;
		}
#endif
		else {
			changewin(MAXWINDOWS, (p_id_t)-1);
			lcmdline = -1;
			maxcmdline = 1;
			cp = inputstr(PTYKC_K, 1, -1, NULL, -1);
			movepos(filepos, 0);
			changewin(win, (p_id_t)-1);
			if (!cp) continue;
#ifdef	DEP_IME
			w = ime_getkeycode(cp);
			Xfree(cp);
			if (!w) /*EMPTY*/;
			else if (!(w & ~01777)) {
				c = w;
				break;
			}
			else if ((c = ime_inkanjiconv(buf, w)) < 0) /*EMPTY*/;
			else if (c || !(c = (u_char)(buf[0]))) break;
			else {
				next = 1;
				break;
			}
#else	/* !DEP_IME */
			c = getkeycode(cp, 0);
			Xfree(cp);
			if (c >= 0) break;
#endif	/* !DEP_IME */

			changewin(MAXWINDOWS, (p_id_t)-1);
			warning(0, VALNG_K);
			movepos(filepos, 0);
			changewin(win, (p_id_t)-1);
		}
	}

#ifdef	DEP_IME
	if (c < 0) {
		next = 0;
		ime_inputkanji(0, NULL);
	}
#endif

	return(c);
}

static int NEAR recvvar(fd, varp)
int fd;
char ***varp;
{
	char *cp, **var;
	int i, c;

	if (recvbuf(fd, &var, sizeof(var)) < 0) return(-1);
	if (var) {
		if (recvbuf(fd, &c, sizeof(c)) < 0) return(-1);
		var = (char **)Xmalloc((c + 1) * sizeof(char *));
		for (i = 0; i < c; i++) {
			if (recvstring(fd, &cp) < 0) {
				var[i] = NULL;
				freevar(var);
				return(-1);
			}
			var[i] = cp;
		}
		var[i] = NULL;
	}
	*varp = var;

	return(0);
}

#ifdef	DEP_ORIGSHELL
static int NEAR recvheredoc(fd, hdpp)
int fd;
heredoc_t **hdpp;
{
	heredoc_t *hdp;

	if (recvbuf(fd, &hdp, sizeof(hdp)) < 0) return(-1);

	if (hdp) {
		hdp = (heredoc_t *)Xmalloc(sizeof(heredoc_t));
		if (recvbuf(fd, hdp, sizeof(*hdp)) < 0) {
			Xfree(hdp);
			return(-1);
		}
		hdp -> eof = hdp -> filename = hdp -> buf = NULL;
		if (recvstring(fd, &(hdp -> eof)) < 0
		|| recvstring(fd, &(hdp -> filename)) < 0
		|| recvstring(fd, &(hdp -> buf)) < 0) {
			freeheredoc(hdp, 0);
			return(-1);
		}
	}
	*hdpp = hdp;

	return(0);
}

static int NEAR recvrlist(fd, rpp)
int fd;
redirectlist **rpp;
{
	redirectlist *rp;
	int n;

	if (recvbuf(fd, &rp, sizeof(rp)) < 0) return(-1);

	if (rp) {
		rp = (redirectlist *)Xmalloc(sizeof(redirectlist));
		if (recvbuf(fd, rp, sizeof(*rp)) < 0) {
			Xfree(rp);
			return(-1);
		}
		rp -> filename = NULL;
		rp -> next = NULL;
		if (rp -> type & MD_HEREDOC)
			n = recvheredoc(fd, (heredoc_t **)&(rp -> filename));
		else n = recvstring(fd, &(rp -> filename));
		if (n < 0 || recvrlist(fd, &(rp -> next)) < 0) {
			freerlist(rp, 0);
			return(-1);
		}
	}
	*rpp = rp;

	return(0);
}

static int NEAR recvcommand(fd, commp, trp)
int fd;
command_t **commp;
syntaxtree *trp;
{
	command_t *comm;
	char **argv;
	int n;

	if (recvbuf(fd, &comm, sizeof(comm)) < 0) return(-1);

	if (comm) {
		if (trp -> flags & ST_NODE)
			return(recvstree(fd, (syntaxtree **)commp, trp));

		comm = (command_t *)Xmalloc(sizeof(command_t));
		if (recvbuf(fd, comm, sizeof(*comm)) < 0) {
			Xfree(comm);
			return(-1);
		}
		argv = comm -> argv;
		comm -> argv = NULL;
		comm -> redp = NULL;
		if (!argv) n = 0;
		else if (!isstatement(comm)) n = recvvar(fd, &(comm -> argv));
		else n = recvstree(fd, (syntaxtree **)&(comm -> argv), trp);
		if (n < 0 || recvrlist(fd, &(comm -> redp)) < 0) {
			freecomm(comm, 0);
			return(-1);
		}
	}
	*commp = comm;

	return(0);
}

static int NEAR recvstree(fd, trpp, parent)
int fd;
syntaxtree **trpp, *parent;
{
	syntaxtree *trp;

	if (recvbuf(fd, &trp, sizeof(trp)) < 0) return(-1);

	if (trp) {
		trp = newstree(parent);
		if (recvbuf(fd, trp, sizeof(*trp)) < 0) {
			Xfree(trp);
			return(-1);
		}
		trp -> comm = NULL;
		trp -> next = NULL;
		if (recvcommand(fd, &(trp -> comm), trp) < 0
		|| recvstree(fd, &(trp -> next), trp) < 0) {
			freestree(trp);
			return(-1);
		}
	}
	*trpp = trp;

	return(0);
}
#endif	/* DEP_ORIGSHELL */

static VOID NEAR recvchild(w)
int w;
{
#ifdef	DEP_ORIGSHELL
	syntaxtree *trp;
#endif
#ifndef	_NOARCHIVE
	lsparse_t launch;
	archive_t arch;
#endif
#ifdef	DEP_DOSEMU
	devinfo dev;
#endif
	bindtable bind;
	keyseq_t key;
	char *cp, *func1, *func2, ***varp, **var;
	u_char uc;
	int n, fd, val, wastty;

	fd = ptylist[w].pipe;
	if (recvbuf(fd, &uc, sizeof(uc)) < 0) return;

	switch (uc) {
		case TE_SETVAR:
			if (recvbuf(fd, &varp, sizeof(varp)) < 0
			|| recvvar(fd, &var) < 0)
				break;
			freevar(*varp);
			*varp = var;
			break;
		case TE_PUSHVAR:
			if (recvbuf(fd, &varp, sizeof(varp)) < 0
			|| recvstring(fd, &cp) < 0)
				break;
			n = countvar(*varp);
			(*varp) = (char **)Xrealloc(*varp,
				(n + 1 + 1) * sizeof(char *));
			memmove((char *)&((*varp)[1]), (char *)&((*varp)[0]),
				n * sizeof(char *));
			(*varp)[0] = cp;
			(*varp)[n + 1] = NULL;
			break;
		case TE_POPVAR:
			if (recvbuf(fd, &varp, sizeof(varp)) < 0
			|| (n = countvar(*varp)) <= 0)
				break;
			Xfree((*varp)[0]);
			memmove((char *)&((*varp)[0]), (char *)&((*varp)[1]),
				n * sizeof(char *));
			break;
		case TE_CHDIR:
			if (recvstring(fd, &cp) < 0 || !cp) break;
			VOID_C chdir2(cp);
			Xfree(cp);
			break;
#ifdef	DEP_ORIGSHELL
		case TE_PUTEXPORTVAR:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvstring(fd, &cp) < 0 || !cp)
				break;
			if (putexportvar(cp, n) < 0) Xfree(cp);
			break;
		case TE_PUTSHELLVAR:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvstring(fd, &cp) < 0 || !cp)
				break;
			if (putshellvar(cp, n) < 0) Xfree(cp);
			break;
		case TE_UNSET:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvstring(fd, &cp) < 0 || !cp)
				break;
			VOID_C unset(cp, n);
			Xfree(cp);
			break;
		case TE_SETEXPORT:
			if (recvstring(fd, &cp) < 0 || !cp) break;
			VOID_C setexport(cp);
			Xfree(cp);
			break;
		case TE_SETRONLY:
			if (recvstring(fd, &cp) < 0 || !cp) break;
			VOID_C setronly(cp);
			Xfree(cp);
			break;
		case TE_SETSHFLAG:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvbuf(fd, &val, sizeof(val)) < 0)
				break;
			setshflag(n, val);
			errorexit = tmperrorexit;
			break;
		case TE_ADDFUNCTION:
			if (recvstring(fd, &cp) < 0) break;
			if (recvstree(fd, &trp, NULL) < 0 || !trp) {
				Xfree(cp);
				break;
			}
			if (cp) setshfunc(cp, trp);
			break;
		case TE_DELETEFUNCTION:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvstring(fd, &cp) < 0 || !cp)
				break;
			VOID_C unsetshfunc(cp, n);
			Xfree(cp);
			break;
#else	/* !DEP_ORIGSHELL */
		case TE_PUTSHELLVAR:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvstring(fd, &func1) < 0)
				break;
			if (recvstring(fd, &cp) < 0 || !cp) {
				Xfree(func1);
				break;
			}
			setenv2(cp, func1, n);
			Xfree(cp);
			Xfree(func1);
			break;
		case TE_ADDFUNCTION:
			if (recvvar(fd, &var) < 0) break;
			if (recvstring(fd, &cp) < 0 || !cp) {
				freevar(var);
				break;
			}
			VOID_C addfunction(cp, var);
			break;
		case TE_DELETEFUNCTION:
			if (recvstring(fd, &cp) < 0 || !cp) break;
			VOID_C deletefunction(cp);
			Xfree(cp);
			break;
#endif	/* !DEP_ORIGSHELL */
#if	!defined (DEP_ORIGSHELL) || !defined (NOALIAS)
		case TE_ADDALIAS:
			if (recvstring(fd, &func1) < 0) break;
			if (recvstring(fd, &cp) < 0 || !cp) {
				Xfree(func1);
				break;
			}
			VOID_C addalias(cp, func1);
			break;
		case TE_DELETEALIAS:
			if (recvstring(fd, &cp) < 0 || !cp) break;
			VOID_C deletealias(cp);
			Xfree(cp);
			break;
#endif	/* !DEP_ORIGSHELL || !NOALIAS */
		case TE_SETHISTORY:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvstring(fd, &cp) < 0 || !cp)
				break;
			VOID_C entryhist(cp, n);
			Xfree(cp);
			break;
		case TE_ADDKEYBIND:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvbuf(fd, &bind, sizeof(bind)) < 0
			|| recvstring(fd, &func1) < 0)
				break;
			if (recvstring(fd, &func2) < 0) {
				Xfree(func1);
				break;
			}
			if (recvstring(fd, &cp) < 0) {
				Xfree(func1);
				Xfree(func2);
				break;
			}
			VOID_C addkeybind(n, &bind, func1, func2, cp);
			break;
		case TE_DELETEKEYBIND:
			if (recvbuf(fd, &n, sizeof(n)) < 0) break;
			deletekeybind(n);
			break;
		case TE_SETKEYSEQ:
			if (recvbuf(fd, &(key.code), sizeof(key.code)) < 0
			|| recvbuf(fd, &(key.len), sizeof(key.len)) < 0
			|| recvstring(fd, &(key.str)) < 0)
				break;
			setkeyseq(key.code, key.str, key.len);
			break;
#ifndef	_NOARCHIVE
		case TE_ADDLAUNCH:
			launch.ext = launch.comm = NULL;
# ifndef	OLDPARSE
			launch.format = launch.lignore = launch.lerror = NULL;
# endif
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvbuf(fd, &launch, sizeof(launch)) < 0
			|| recvstring(fd, &(launch.ext)) < 0
			|| recvstring(fd, &(launch.comm)) < 0
# ifndef	OLDPARSE
			|| recvvar(fd, &(launch.format)) < 0
			|| recvvar(fd, &(launch.lignore)) < 0
			|| recvvar(fd, &(launch.lerror)) < 0
# endif
			) {
				freelaunch(&launch);
				break;
			}
			addlaunch(n, &launch);
			break;
		case TE_DELETELAUNCH:
			if (recvbuf(fd, &n, sizeof(n)) < 0) break;
			deletelaunch(n);
			break;
		case TE_ADDARCH:
			arch.ext = arch.p_comm = arch.u_comm = NULL;
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvbuf(fd, &(arch.flags), sizeof(arch.flags)) < 0
			|| recvstring(fd, &(arch.ext)) < 0
			|| recvstring(fd, &(arch.p_comm)) < 0
			|| recvstring(fd, &(arch.u_comm)) < 0) {
				freearch(&arch);
				break;
			}
			addarch(n, &arch);
			break;
		case TE_DELETEARCH:
			if (recvbuf(fd, &n, sizeof(n)) < 0) break;
			deletearch(n);
			break;
#endif	/* !_NOARCHIVE */
#ifdef	DEP_DOSEMU
		case TE_INSERTDRV:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvbuf(fd, &dev, sizeof(dev)) < 0
			|| recvstring(fd, &(dev.name)) < 0)
				break;
			VOID_C insertdrv(n, &dev);
			break;
		case TE_DELETEDRV:
			if (recvbuf(fd, &n, sizeof(n)) < 0) break;
			VOID_C deletedrv(n);
			break;
#endif	/* DEP_DOSEMU */
		case TE_LOCKFRONT:
			lockflags |= (1 << w);
			if (!emupid) break;
			sendword(emufd, TE_LOCKBACK);
			sendword(emufd, w);
			break;
		case TE_UNLOCKFRONT:
			lockflags &= ~(1 << w);
			if (!emupid) break;
			sendword(emufd, TE_UNLOCKBACK);
			sendword(emufd, w);
			break;
		case TE_SAVETTYIO:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvbuf(fd, &val, sizeof(val)) < 0)
				break;
			if (!val) cp = NULL;
			else {
				cp = Xmalloc(val);
				if (recvbuf(fd, cp, val) < 0) {
					Xfree(cp);
					break;
				}
			}
			if (n >= 0) {
				Xfree(duptty[n]);
				duptty[n] = cp;
			}
			else Xfree(cp);
			break;
#ifdef	DEP_IME
		case TE_ADDROMAN:
			if (recvstring(fd, &cp) < 0 || !cp) break;
			if (recvstring(fd, &func1) < 0) {
				Xfree(cp);
				break;
			}
			VOID_C addroman(cp, func1);
			break;
		case TE_FREEROMAN:
			if (recvbuf(fd, &n, sizeof(n)) < 0) break;
			freeroman(n);
			break;
#endif	/* DEP_IME */
		case TE_INTERNAL:
			if (recvbuf(fd, &n, sizeof(n)) < 0
			|| recvstring(fd, &cp) < 0)
				break;
			if (n < 0 || n >= FUNCLISTSIZ) {
				Xfree(cp);
				break;
			}
			keywaitfunc = lastfunc;
			changewin(MAXWINDOWS, (p_id_t)-1);
			if (!(wastty = isttyiomode)) Xttyiomode(0);
			ptyinternal++;
			VOID_C dointernal(n, cp, ICM_CMDLINE, NULL);
			ptyinternal--;
			if (!wastty) Xstdiomode();
			changewin(win, (p_id_t)-1);
			rewritefile(0);
			keywaitfunc = waitpty;
			Xfree(cp);
			break;
		case TE_CHANGESTATUS:
			if (recvbuf(fd, &n, sizeof(n)) < 0) break;
			ptylist[w].status = n;
			break;
		default:
			break;
	}
}

static VOID NEAR sendkey(c)
int c;
{
	static char buf[MAXUTF8LEN * sizeof(short)];
	static int index = 0;
	static int max = 0;
#ifdef	DEP_KCONV
	int code;
#endif
	short w;

#ifdef	DEP_KCONV
	code = (inputkcode != NOCNV) ? inputkcode : DEFCODE;
#endif
	if (c >= 0) {
		w = c;
		memcpy(&(buf[index++ * sizeof(w)]), (char *)&w, sizeof(w));
	}

	if (c < 0 || c & 0xff00) /*EMPTY*/;
	else if (max && index >= max) /*EMPTY*/;
#ifdef	DEP_KCONV
	else if (code == EUC && c == C_EKANA) {
		max = 2;
		return;
	}
	else if (code >= UTF8) {
		if (index > 1) {
			if ((c & 0xc0) == 0x80) return;
		}
		else if (ismsb(c)) {
			max = ((c & 0xf0) == 0xe0) ? 3 : 2;
			return;
		}
	}
#else	/* !DEP_KCONV */
# ifdef	CODEEUC
	else if (c == C_EKANA) {
		max = 2;
		return;
	}
# endif
#endif	/* !DEP_KCONV */
	else if (isinkanji1(c, code)) {
		max = 2;
		return;
	}

	if (index) sendbuf(emufd, buf, index * sizeof(w));
	index = max = 0;
}

int frontend(VOID_A)
{
	int n, ch, wastty, status;

	ptylist[win].status = -1;
	n = sigvecset(1);
	lastfunc = keywaitfunc;
	keywaitfunc = waitpty;
	changewsize(wheader, windows);
	changewin(win, ptylist[win].pid);

	internal_status = FNC_FAIL;
	if (!(wastty = isttyiomode)) Xttyiomode(1);
	while ((ch = ptygetkey()) >= 0) sendkey(ch);
	sendkey(-1);
	if (!wastty) Xstdiomode();

	if (ch < -1) {
		status = 0;
		internal_status = FNC_CANCEL;
	}
	else {
		status = ptylist[win].status;
		if (!(ptylist[win].pid)) {
			ptylist[win].status = -1;
			recvchild(win);
			killpty(win, &status);
		}
		if (internal_status == FNC_FAIL && status < 128)
			internal_status = status;
	}

	sigvecset(n);
	keywaitfunc = lastfunc;
	if (!(ptylist[win].pid)) {
		changewin(win, (p_id_t)0);
		if (!emupid && ptytmpfile) {
			rmtmpfile(ptytmpfile);
			Xfree(ptytmpfile);
			ptytmpfile = NULL;
		}
	}
	changewin(MAXWINDOWS, (p_id_t)-1);
	setlinecol();

	return(status);
}
#endif	/* DEP_PTY */
